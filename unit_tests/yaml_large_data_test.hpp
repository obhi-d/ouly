#include <string_view>

constexpr std::string_view large_yml = R"(
- name: RaiderAlpha
  root:
    name: RaidCore
    type: ReactiveSelector
    children:
      - name: AssaultPlayer
        type: Sequence
        children:
          - name: MarkTarget
            type: Condition
            method: AcquireTarget
          - name: CheckLine
            type: Condition
            method: HasLineOfSight
          - name: ChargeStrike
            type: Action
            method: LungeAttack
      - name: BurnZone
        type: Sequence
        children:
          - name: SweepArea
            type: Action
            method: RaidPerimeter
          - name: HoldPosition
            type: Wait
            duration: 1.8
      - name: CirclePatrol
        type: Sequence
        children:
          - name: PatrolRing
            type: Action
            method: PatrolArea
          - name: PauseScan
            type: Wait
            duration: 2.7

- name: ForagerCrew
  root:
    name: GatherLoop
    type: Selector
    children:
      - name: SearchResource
        type: Sequence
        children:
          - name: LocateNode
            type: Action
            method: FindResource
          - name: MoveToNode
            type: Action
            method: MoveTo
          - name: ExtractNode
            type: Action
            method: HarvestResource
      - name: StoreSupplies
        type: Sequence
        children:
          - name: FindDepot
            type: Condition
            method: FindStorage
          - name: MoveToDepot
            type: Action
            method: MoveTo
          - name: Deposit
            type: Wait
            duration: 1.4
      - name: RestoreEnergy
        type: Sequence
        children:
          - name: IfTired
            type: Condition
            method: IsTired
          - name: TakeBreak
            type: Action
            method: RestInPlace

- name: SoulAscend
  root:
    name: ReleaseSpirit
    type: Sequence
    children:
      - name: LiftOff
        type: Wait
        duration: 2.2
      - name: Despawn
        type: Action
        method: DespawnSelf

- name: SurveyDrifter
  root:
    name: ScanLoop
    type: Selector
    children:
      - name: HungerCycle
        type: Sequence
        children:
          - name: SenseHunger
            type: Condition
            method: IsHungry
          - name: SeekFood
            type: Action
            method: FindFood
          - name: ApproachFood
            type: Action
            method: MoveTo
          - name: Consume
            type: Action
            method: ConsumeFood
      - name: RestCycle
        type: Sequence
        children:
          - name: SenseFatigue
            type: Condition
            method: IsTired
          - name: Pause
            type: Action
            method: RestInPlace
      - name: Roam
        type: Sequence
        children:
          - name: Wander
            type: Action
            method: WanderNearby
          - name: ShortWait
            type: Wait
            duration: 2.6

- name: StalkerSearch
  root:
    name: HuntLoop
    type: Selector
    children:
      - name: CheckLastSpot
        type: Sequence
        children:
          - name: MoveToMark
            type: Action
            method: MoveToLastKnownTargetPosition
          - name: ScanAround
            type: Wait
            duration: 2.4
          - name: SpotTarget
            type: Condition
            method: FindPlayer
      - name: PatrolHunt
        type: Sequence
        children:
          - name: DriftSearch
            type: Action
            method: WanderNearby
          - name: PauseLook
            type: Wait
            duration: 1.7
          - name: TrySpot
            type: Condition
            method: FindPlayer

- name: GateArrival
  root:
    name: EntryPause
    type: Wait
    duration: 1.9

- name: TotemLoiter
  root:
    name: TotemIdle
    type: ReactiveSelector
    children:
      - name: GatherByTotem
        type: Sequence
        children:
          - name: LocateTotem
            type: Condition
            method: FindTotem
          - name: MoveToTotem
            type: Action
            method: GatherAroundTotem
          - name: Linger
            type: Wait
            duration: 4.6
      - name: HungerCheck
        type: Sequence
        children:
          - name: SenseHunger
            type: Condition
            method: IsHungry
          - name: SeekFood
            type: Action
            method: FindFood
          - name: ApproachFood
            type: Action
            method: MoveTo
          - name: Eat
            type: Action
            method: ConsumeFood
      - name: FatigueCheck
        type: Sequence
        children:
          - name: SenseFatigue
            type: Condition
            method: IsTired
          - name: Rest
            type: Action
            method: RestInPlace
      - name: DriftNear
        type: Sequence
        children:
          - name: SlowWander
            type: Action
            method: WanderNearby
          - name: MicroPause
            type: Wait
            duration: 2.8

- name: HunterAssault
  root:
    name: EngageTarget
    type: ReactiveSelector
    children:
      - name: CloseCombat
        type: Sequence
        children:
          - name: LockTarget
            type: Condition
            method: SetPlayerAsTarget
          - name: CheckSight
            type: Condition
            method: IsTargetVisible
          - name: Strike
            type: Action
            method: AttackTarget
      - name: RangedCombat
        type: Sequence
        children:
          - name: LockTarget
            type: Condition
            method: SetPlayerAsTarget
          - name: CheckSight
            type: Condition
            method: IsTargetVisible
          - name: Fire
            type: Action
            method: RangeAttackTarget
      - name: Pursue
        type: Sequence
        children:
          - name: MoveToEcho
            type: Action
            method: MoveToLastKnownTargetPosition
          - name: BriefSearch
            type: Wait
            duration: 1.6

- name: RefugeRunner
  root:
    name: SafeHaven
    type: ReactiveSelector
    children:
      - name: CriticalNeeds
        type: Sequence
        children:
          - name: SenseHunger
            type: Condition
            method: IsHungry
          - name: SeekFood
            type: Action
            method: FindFood
          - name: ApproachFood
            type: Action
            method: MoveTo
          - name: Consume
            type: Action
            method: ConsumeFood
      - name: RestNeeds
        type: Sequence
        children:
          - name: SenseFatigue
            type: Condition
            method: IsTired
          - name: FindShelter
            type: Action
            method: FindBed
          - name: MoveToShelter
            type: Action
            method: MoveTo
          - name: RestNow
            type: Action
            method: RestAtRefuge
      - name: WaitAtShelter
        type: Sequence
        children:
          - name: IdleWait
            type: Wait
            duration: 4.2

- name: LastBreath
  root:
    name: Finalize
    type: Sequence
    children:
      - name: DeathAnim
        type: Action
        method: PlayDeathAnimation
      - name: FadeOut
        type: Wait
        duration: 2.9
      - name: Cleanup
        type: Action
        method: DespawnSelf

- name: WardSentinel
  root:
    name: DefendPost
    type: ReactiveSelector
    children:
      - name: EngageEnemy
        type: Sequence
        children:
          - name: SpotEnemy
            type: Condition
            method: FindClosestTarget
          - name: AttackEnemy
            type: Action
            method: AttackTarget
      - name: Patrol
        type: Sequence
        children:
          - name: PatrolArea
            type: Action
            method: PatrolArea
          - name: WatchWait
            type: Wait
            duration: 3.3

- name: RaidScout
  root:
    name: TargetVillage
    type: ReactiveSelector
    children:
      - name: AttackIfSeen
        type: Sequence
        children:
          - name: SpotPlayer
            type: Condition
            method: FindPlayer
          - name: AttackPlayer
            type: Action
            method: AttackTarget
      - name: LocateVillage
        type: Sequence
        children:
          - name: FindVillagePOI
            type: Condition
            method: FindVillage
          - name: MoveToVillage
            type: Action
            method: MoveToVillage
      - name: SearchForTargets
        type: Sequence
        children:
          - name: PatrolSearch
            type: Action
            method: PatrolArea
          - name: LookAround
            type: Wait
            duration: 1.9

- name: LabWorker
  root:
    name: ResearchLoop
    type: Selector
    children:
      - name: DoResearchWork
        type: Sequence
        children:
          - name: FindStation
            type: Condition
            method: FindResearchStation
          - name: MoveToStation
            type: Action
            method: MoveTo
          - name: PerformResearch
            type: Action
            method: DoResearch
      - name: TakeBreak
        type: Sequence
        children:
          - name: SenseFatigue
            type: Condition
            method: IsTired
          - name: Rest
            type: Action
            method: RestInPlace
      - name: IdleThink
        type: Wait
        duration: 1.3

- name: VillageSeeker
  root:
    name: SeekVillage
    type: Selector
    children:
      - name: LocateVillage
        type: Sequence
        children:
          - name: FindVillagePOI
            type: Condition
            method: FindVillage
          - name: MoveToVillage
            type: Action
            method: MoveToVillage
      - name: WanderToFind
        type: Sequence
        children:
          - name: WanderSearch
            type: Action
            method: WanderNearby
          - name: PauseAndLook
            type: Wait
            duration: 2.1

- name: CompanionTrail
  root:
    name: FollowLoop
    type: ReactiveSelector
    children:
      - name: StayClose
        type: Sequence
        children:
          - name: FollowPlayer
            type: Action
            method: FollowPlayer
          - name: WaitForPlayer
            type: Wait
            duration: 1.2
      - name: HandleNeeds
        type: Sequence
        children:
          - name: SenseHunger
            type: Condition
            method: IsHungry
          - name: FindFood
            type: Action
            method: FindFood
          - name: MoveToFood
            type: Action
            method: MoveTo
          - name: Eat
            type: Action
            method: ConsumeFood

)";
