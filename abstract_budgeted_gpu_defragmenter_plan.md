# Abstract Budgeted GPU Defragmenter Plan

The defragmenter should be independent of the concrete allocator, resource type, and graphics API. Its job is to:

1. inspect allocator state;
2. identify backing blocks worth evacuating;
3. ask the allocator whether allocations can move;
4. reserve destination allocations;
5. emit relocation operations within a copy budget;
6. track asynchronous completion;
7. release old allocations only when safe;
8. free backing blocks that become empty.

The central policy should be **incremental block evacuation**, not arbitrary hole filling.

---

## 1. Define the abstractions

Your defragmenter should not manipulate allocator internals directly. Expose an adapter interface for every allocator implementation.

```cpp
using BlockId       = uint64_t;
using AllocationId  = uint64_t;
using MoveId        = uint64_t;
using TimelineValue = uint64_t;
```

### Allocation description

```cpp
struct DefragAllocation
{
    AllocationId id;
    BlockId block;

    uint64_t offset;
    uint64_t size;
    uint64_t alignment;

    // Optional allocator-defined classification.
    uint32_t memoryClass;
    uint32_t resourceClass;

    bool movable;
    bool pinned;
};
```

`resourceClass` can distinguish things such as:

```cpp
enum class ResourceClass : uint32_t
{
    RawBufferRange,
    BufferResource,
    ImageResource,
    AccelerationStructure,
    ExternalResource
};
```

The defragmenter should treat the classification as opaque except where policy requires filtering.

### Backing-block description

```cpp
struct DefragBlock
{
    BlockId id;

    uint64_t capacity;
    uint64_t usedBytes;
    uint64_t freeBytes;

    uint32_t allocationCount;
    uint32_t movableAllocationCount;

    bool dedicated;
    bool releasable;
};
```

`releasable` means that an empty block can actually be destroyed or returned to the parent allocator.

---

## 2. Define the allocator adapter

The adapter is the boundary between the generic planner and each concrete allocator.

```cpp
class IDefragAllocator
{
public:
    virtual ~IDefragAllocator() = default;

    virtual void enumerateBlocks(
        std::function<void(DefragBlock const&)> visitor) const = 0;

    virtual void enumerateAllocations(
        BlockId block,
        std::function<void(DefragAllocation const&)> visitor) const = 0;

    virtual bool canMove(AllocationId allocation) const = 0;

    virtual std::optional<DefragAllocation> reserveDestination(
        DefragAllocation const& source,
        std::span<BlockId const> excludedBlocks) = 0;

    virtual void cancelDestination(
        AllocationId destination) = 0;

    virtual void commitMove(
        AllocationId source,
        AllocationId destination) = 0;

    virtual void releaseSource(
        AllocationId source) = 0;

    virtual bool releaseBlock(
        BlockId block) = 0;
};
```

The adapter must guarantee that `reserveDestination()`:

- observes size and alignment;
- respects memory type and usage compatibility;
- never allocates inside an evacuation source block;
- does not invalidate the source;
- returns a reserved destination that remains stable until committed or cancelled.

For images or individually created API resources, the “destination allocation” may represent a newly created resource rather than merely a new offset.

---

## 3. Separate planning from execution

The system should have three layers:

```text
Allocator adapter
    Supplies allocation and block state

Defragmentation planner
    Chooses source blocks and destination placements

Relocation executor
    Records copies, patches handles and tracks retirement
```

Avoid letting the planner submit GPU commands directly.

---

## 4. Define the budget

A byte budget alone is insufficient because thousands of small moves may be expensive.

```cpp
struct DefragBudget
{
    uint64_t maxCopyBytes;
    uint32_t maxMoveCount;

    uint32_t maxResourceCreations;
    uint32_t maxDescriptorUpdates;

    // Optional CPU planning limit.
    uint32_t maxBlocksScanned;
};
```

Track actual transfer cost:

```cpp
struct MoveCost
{
    uint64_t copyBytes;
    uint32_t moveCount;
    uint32_t resourceCreations;
    uint32_t descriptorUpdates;
};
```

For a direct move:

```text
copyBytes = allocation.size
```

For a scratch-mediated move:

```text
copyBytes = allocation.size × 2
```

The budget should normally mean **actual transfer bytes**, not merely logical allocation size.

---

## 5. Maintain persistent defragmentation state

Defragmentation spans multiple frames and submissions.

```cpp
enum class BlockDefragState
{
    Normal,
    EvacuationTarget,
    WaitingForRetirement,
    Empty
};
```

```cpp
struct BlockState
{
    BlockId block;
    BlockDefragState state;

    uint64_t originalLiveBytes;
    uint64_t remainingLiveBytes;

    uint32_t pendingMoveCount;
};
```

Once a block becomes an evacuation target:

- never place new allocations into it;
- continue moving allocations from it across frames;
- do not abandon it unless evacuation becomes impossible;
- release it after all source allocations retire.

This persistence is critical. Without it, the allocator may repeatedly improve many blocks but never empty one.

---

## 6. Represent a move explicitly

```cpp
enum class MoveState
{
    Reserved,
    CopyScheduled,
    CopyCompleted,
    ReferencesSwitched,
    SourceRetired,
    Cancelled
};
```

```cpp
struct PlannedMove
{
    MoveId id;

    AllocationId source;
    AllocationId destination;

    BlockId sourceBlock;
    BlockId destinationBlock;

    uint64_t size;
    uint64_t transferBytes;

    MoveState state;

    TimelineValue copyCompletion;
    TimelineValue sourceRetirement;
};
```

The source and destination must coexist until all older GPU uses of the source have completed.

---

## 7. Choose evacuation candidates

For each block, compute:

```cpp
struct BlockCandidate
{
    BlockId block;

    uint64_t capacity;
    uint64_t liveBytes;
    uint64_t movableBytes;
    uint64_t pinnedBytes;

    uint32_t movableCount;

    double score;
};
```

Reject blocks that:

- are dedicated allocations;
- cannot be released when empty;
- contain pinned allocations;
- contain unsupported resource types;
- are already destination-only blocks;
- cannot be completely evacuated according to a placement simulation.

A simple candidate score is:

```cpp
score =
    static_cast<double>(capacity) /
    static_cast<double>(std::max<uint64_t>(movableBytes, 1));
```

This rewards blocks that release substantial capacity for little copy traffic.

A more practical score:

```cpp
score =
      releaseWeight * capacity
    - copyWeight * movableBytes
    - moveWeight * movableCount
    - placementRiskPenalty;
```

Do not rely exclusively on occupancy percentage. A 256 MB block with 16 MB live is usually a better target than a 32 MB block with 1 MB live, depending on the objective.

---

## 8. Simulate complete evacuation before starting

Before marking a block as an evacuation target, ensure every allocation has somewhere to go.

Create a snapshot of destination free ranges and perform a dry-run packing pass.

Recommended ordering:

```text
1. Highest alignment first
2. Largest size first
3. Most restricted resource class first
```

Conceptually:

```cpp
std::ranges::sort(
    allocations,
    [](auto const& lhs, auto const& rhs)
    {
        if (lhs.alignment != rhs.alignment)
            return lhs.alignment > rhs.alignment;

        return lhs.size > rhs.size;
    });
```

For each allocation, select a destination using best fit:

```text
smallest valid free range that satisfies:
    size
    alignment
    memory compatibility
    resource compatibility
```

The simulation should not mutate the real allocator.

```cpp
struct SimulatedPlacement
{
    AllocationId source;
    BlockId destinationBlock;
    uint64_t destinationOffset;
};
```

Only select a block when all movable allocations can be placed.

This prevents spending most of the budget on a source block and discovering that the last large allocation cannot move.

---

## 9. Destination-block policy

Destination selection should consolidate allocations into blocks that you intend to keep.

Prefer:

1. already-dense destination blocks;
2. best-fit holes;
3. blocks with stable long-term residency;
4. blocks that are not evacuation candidates;
5. blocks with enough space for the entire planned evacuation.

Avoid:

- allocating into the source block;
- scattering moves across empty blocks;
- creating a new backing block merely to move a tiny amount, unless necessary;
- placing allocations into another fragmented block that you intend to evacuate soon.

A destination score can be:

```cpp
destinationScore =
      remainingHoleWaste
    + resultingFragmentationPenalty
    + sparseBlockPenalty
    + newBlockPenalty;
```

Choose the lowest score.

---

## 10. Frame-level planning algorithm

At the start of a defrag pass:

```cpp
void Defragmenter::run(DefragBudget budget)
{
    retireCompletedMoves();

    DefragBudget remaining = budget;

    while (hasBudget(remaining))
    {
        BlockId source = selectSourceBlock();

        if (source == InvalidBlock)
            break;

        auto move = selectNextMove(source, remaining);

        if (!move)
            break;

        scheduleMove(*move);
        consumeBudget(remaining, move->cost);
    }
}
```

Source selection priority:

```text
1. Continue an existing evacuation target.
2. Finish a nearly evacuated block.
3. Start a fully feasible block evacuation.
4. Optionally perform preparatory moves.
```

Do not start a new source block while an existing source block can still make progress, unless there is a strong reason such as destination incompatibility.

---

## 11. Choosing the next allocation within a source block

The execution order does not have to match source address order.

A good move priority is:

```text
1. Allocation with only one valid destination
2. Highest alignment
3. Largest allocation
4. Best destination-hole match
5. Lowest relocation overhead
```

You can assign each allocation a difficulty score:

```cpp
difficulty =
      alignmentPenalty
    + sizePenalty
    + resourceCreationPenalty
    + destinationScarcityPenalty;
```

Move difficult allocations early so that they do not block final evacuation.

However, budget constraints matter. If the remaining frame budget is smaller than the next large allocation, you may move a smaller allocation provided it does not invalidate the complete evacuation plan.

---

## 12. Support budget rollover across frames

Suppose the current source block has:

```text
12 MB
20 MB
40 MB
```

and the budget is 32 MB.

Frame 1:

```text
move 20 MB
move 12 MB
```

Frame 2:

```text
move 40 MB
```

Do not require the complete block to fit within one frame’s budget.

Maintain the planned destination map across frames, or revalidate it every frame before submitting additional moves.

A reserved destination can either be:

- reserved immediately for the full evacuation;
- reserved lazily per move.

Full reservation gives stronger guarantees but temporarily consumes more free space. Lazy reservation reduces temporary pressure but risks later placement failure.

A sensible compromise is:

- simulate the entire evacuation;
- reserve destinations lazily;
- revalidate the remaining plan after every allocator mutation.

---

## 13. GPU copy execution

The executor should expose a resource-type-independent operation:

```cpp
struct RelocationCommand
{
    AllocationId source;
    AllocationId destination;

    uint64_t sourceOffset;
    uint64_t destinationOffset;
    uint64_t size;

    ResourceClass resourceClass;
};
```

Then route it to API-specific implementations:

```cpp
class IRelocationExecutor
{
public:
    virtual TimelineValue scheduleCopy(
        RelocationCommand const& command) = 0;

    virtual bool isComplete(
        TimelineValue value) const = 0;

    virtual void switchReferences(
        PlannedMove const& move) = 0;

    virtual TimelineValue lastSourceUse(
        PlannedMove const& move) const = 0;
};
```

For large suballocated buffers:

```text
vkCmdCopyBuffer
MTLBlitCommandEncoder copyFromBuffer
```

For images:

```text
create compatible destination resource
transition states/layouts
copy every required subresource
patch descriptors and handles
retire old resource
```

---

## 14. Handle resource references through indirection

Defragmentation is substantially easier when external systems do not retain raw allocator offsets.

Prefer:

```cpp
struct BufferHandle
{
    uint32_t index;
    uint32_t generation;
};
```

with a relocatable table:

```cpp
struct BufferLocation
{
    GpuBufferHandle buffer;
    uint64_t offset;
    uint64_t size;
};
```

After relocation:

```cpp
locationTable[handle.index] = newLocation;
```

Existing CPU-side handles remain stable.

Avoid exposing permanently stable:

- raw `VkBuffer` handles when recreation may be needed;
- raw `MTLBuffer*`;
- physical allocation offsets;
- device addresses;
- descriptor indices that cannot be patched.

Resources exposing device addresses or referenced from acceleration structures may need to be pinned or rebuilt after movement.

---

## 15. Synchronization and retirement

A move is not complete merely because the copy has been submitted.

Typical sequence:

```text
1. Reserve destination
2. Record copy
3. Submit copy
4. Wait for copy completion for new users
5. Switch future references
6. Wait for all old source users to retire
7. Release source
8. Coalesce old free range
9. Release source block if empty
```

You need two safety points:

```cpp
copyCompletion
sourceRetirement
```

These may be the same only when you globally synchronize and guarantee no older work references the source.

A timeline-based design:

```cpp
void Defragmenter::processPendingMoves()
{
    for (auto& move : pendingMoves)
    {
        if (move.state == MoveState::CopyScheduled &&
            executor.isComplete(move.copyCompletion))
        {
            executor.switchReferences(move);
            move.sourceRetirement = executor.lastSourceUse(move);
            move.state = MoveState::ReferencesSwitched;
        }

        if (move.state == MoveState::ReferencesSwitched &&
            executor.isComplete(move.sourceRetirement))
        {
            allocator.releaseSource(move.source);
            move.state = MoveState::SourceRetired;
        }
    }
}
```

In a frame-buffered renderer, retirement may simply use the frame completion fence associated with the last frame that could reference the old resource.

---

## 16. Prevent allocations from fighting the defragmenter

When a block becomes an evacuation target:

```cpp
allocator.setAllocationEnabled(block, false);
```

Otherwise normal allocation traffic can refill the holes being created.

You may need allocator block roles:

```cpp
enum class BlockRole
{
    Normal,
    DestinationOnly,
    EvacuationSource,
    Retiring
};
```

`EvacuationSource` blocks reject all new allocations.

---

## 17. Handle allocator mutations

Normal allocation and deallocation can invalidate plans.

Use a generation counter:

```cpp
struct AllocatorSnapshot
{
    uint64_t generation;
};
```

Increment it whenever allocator topology changes.

Each evacuation plan records:

```cpp
uint64_t allocatorGeneration;
```

Before scheduling a move:

```cpp
if (plan.allocatorGeneration != allocator.currentGeneration())
    revalidatePlan();
```

Revalidation should confirm:

- source still exists;
- source allocation is still movable;
- destination reservation remains valid;
- block is still an evacuation target;
- destination block has not become an evacuation target.

---

## 18. Failure handling

A relocation may fail because:

- destination allocation fails;
- resource creation fails;
- the resource becomes pinned;
- resource state changes;
- copy submission fails;
- allocator topology changes;
- the source is destroyed normally before relocation.

Each reservation must be reversible:

```cpp
void cancelMove(PlannedMove& move)
{
    allocator.cancelDestination(move.destination);
    move.state = MoveState::Cancelled;
}
```

Do not release the source until the move has fully committed.

If one allocation in an evacuation target becomes permanently unmovable, abort that target:

```text
cancel unsubmitted destination reservations
allow normal allocation only after deliberate policy decision
mark block temporarily ineligible
```

Already completed relocations do not need to be rolled back; the block simply remains partially compacted.

---

## 19. Preparatory hole-filling mode

This should be secondary, not the main algorithm.

Use it only when:

- no whole block is currently evacuable;
- the budget would otherwise go unused;
- the move increases the chance of future block evacuation.

A productive preparatory move should accomplish at least one of:

```text
empty a small source block;
merge two free ranges;
move a difficult allocation into a stable dense block;
create a destination range needed by a future evacuation;
reduce live bytes in a likely future source block.
```

Avoid generic “move any allocation into any hole” behavior.

That tends to create allocator churn without releasing backing memory.

---

## 20. Suggested public interface

```cpp
class Defragmenter
{
public:
    struct Config
    {
        uint32_t maxActiveEvacuationTargets = 1;
        uint32_t candidateScanLimit = 64;

        double copyByteWeight = 1.0;
        double moveCountWeight = 64.0 * 1024.0;
        double newBlockPenalty = 16.0 * 1024.0 * 1024.0;

        bool allowPreparatoryMoves = false;
    };

    Defragmenter(
        IDefragAllocator& allocator,
        IRelocationExecutor& executor,
        Config config);

    void run(DefragBudget budget);
    void processCompletions();

    [[nodiscard]]
    DefragStatistics statistics() const;
};
```

Statistics:

```cpp
struct DefragStatistics
{
    uint64_t bytesScheduled;
    uint64_t bytesCopied;
    uint64_t bytesReleased;

    uint64_t blocksReleased;

    uint32_t movesScheduled;
    uint32_t movesCompleted;
    uint32_t movesPending;

    uint32_t activeEvacuationTargets;
};
```

The key efficiency measurement is:

```cpp
releaseEfficiency =
    bytesReleased / max(bytesCopied, 1)
```

---

## 21. Minimal implementation phases

### Phase 1: Synchronous prototype

Implement:

- block enumeration;
- candidate scoring;
- full evacuation simulation;
- destination reservation;
- direct buffer copies;
- full GPU idle wait;
- source release;
- block release.

This validates the allocator policy before asynchronous complexity.

### Phase 2: Budgeted incremental moves

Add:

- per-pass byte budget;
- move-count budget;
- persistent evacuation targets;
- multi-frame evacuation;
- destination exclusion rules.

### Phase 3: Asynchronous retirement

Add:

- timeline or fence tracking;
- separate copy-complete and source-retired states;
- deferred source release;
- descriptor or handle patching.

### Phase 4: Multiple resource classes

Add:

- individually created buffers;
- images and subresources;
- alignment and memory-class restrictions;
- resource recreation callbacks;
- pinned-resource handling.

### Phase 5: Policy refinement

Add:

- preparatory moves;
- better packing heuristics;
- limited knapsack selection across candidate blocks;
- telemetry-driven scoring;
- copy-queue scheduling.

---

## 22. Core implementation skeleton

```cpp
void Defragmenter::run(DefragBudget budget)
{
    processCompletions();

    auto remaining = budget;

    while (remaining.maxCopyBytes > 0 &&
           remaining.maxMoveCount > 0)
    {
        auto* target = findActiveTarget();

        if (!target)
        {
            auto candidate = chooseCandidate();

            if (!candidate)
                break;

            auto plan = simulateEvacuation(candidate->block);

            if (!plan)
            {
                markTemporarilyIneligible(candidate->block);
                continue;
            }

            target = activateEvacuation(std::move(*plan));
        }

        auto next = chooseNextMove(*target, remaining);

        if (!next)
        {
            if (target->remainingAllocationCount == 0)
                beginBlockRetirement(*target);

            break;
        }

        auto destination =
            allocator.reserveDestination(
                next->source,
                evacuationSourceBlocks());

        if (!destination)
        {
            invalidateAndReplan(*target);
            continue;
        }

        PlannedMove move{
            .id = allocateMoveId(),
            .source = next->source.id,
            .destination = destination->id,
            .sourceBlock = next->source.block,
            .destinationBlock = destination->block,
            .size = next->source.size,
            .transferBytes = calculateTransferBytes(*next),
            .state = MoveState::Reserved,
        };

        move.copyCompletion =
            executor.scheduleCopy(makeRelocationCommand(move));

        move.state = MoveState::CopyScheduled;

        pendingMoves.push_back(move);

        remaining.maxCopyBytes -= move.transferBytes;
        remaining.maxMoveCount -= 1;
    }
}
```

Be careful with unsigned subtraction. Test the cost before consuming:

```cpp
if (move.transferBytes > remaining.maxCopyBytes)
    break;
```

---

## 23. Recommended initial policy

Start with these constraints:

```text
One active evacuation source block
No preparatory moves
No overlapping copies
No scratch moves
No movement of dedicated resources
No movement of device-address resources
Best-fit destination placement
Alignment-descending, size-descending planning
Actual copy bytes charged to the budget
Move-count limit in addition to byte limit
```

This will already provide a robust and useful defragmenter.

Only add more complex strategies after telemetry shows they are needed.

---

## 24. Principal invariants

The implementation should assert these continuously:

```text
A source allocation remains valid until retirement.

A destination reservation is unique and non-overlapping.

No new allocation enters an evacuation source block.

A block is released only when:
    it contains no live allocations;
    it contains no pending source allocations;
    it contains no reserved destinations;
    all required GPU timeline values have completed.

A move never changes the public logical allocation handle.

A failed move never destroys or modifies the source.

Budget consumption reflects actual scheduled transfer work.
```

The design can be summarized as:

```text
Select a sparsely occupied releasable block.
Prove that all its allocations can be placed elsewhere.
Freeze it against new allocations.
Move its allocations incrementally under the frame budget.
Patch future references.
Retire old resource instances asynchronously.
Release the entire backing block when empty.
```

That gives you a defragmenter that is allocator-agnostic, budgeted, asynchronous, and capable of producing concrete memory recovery rather than merely rearranging holes.
