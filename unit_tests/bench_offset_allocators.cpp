// Offset / arena allocator benchmark: every allocator runs the same deterministic trace.
//
// Warm up to N live allocations, then repeatedly free a random victim and allocate into its slot.
// Reports ns per (free + alloc) pair, fragmentation (peak arena capacity / peak live bytes, or
// high-water mark / peak live for a single range) and the cost of one full defragmentation after
// freeing a random half. Run with `validate` to check placements for overlap and alignment instead.
// NOLINTBEGIN
#include "ouly/allocators/coalescing_allocator.hpp"
#include "ouly/allocators/coalescing_arena_allocator.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using u32 = uint32_t;
using u64 = uint64_t;

static bool g_validate = false;

struct trace
{
  std::string      name;
  u32              live;
  std::vector<u32> sizes;
  std::vector<u32> victim;
};

static trace make_trace(std::string name, u32 live, u32 steps, u32 lo, u32 hi, u32 round)
{
  trace                                  t{std::move(name), live, {}, {}};
  std::mt19937_64                        rng(1234);
  std::uniform_real_distribution<double> d(std::log2(lo), std::log2(hi));
  for (u32 i = 0; i < live + steps; ++i)
  {
    auto s = static_cast<u32>(std::exp2(d(rng)));
    t.sizes.push_back((s + round - 1) / round * round);
  }
  for (u32 i = 0; i < steps; ++i)
    t.victim.push_back(static_cast<u32>(rng() % live));
  return t;
}

// ------------------------------------------------------------------------------------------------
// Managers
// ------------------------------------------------------------------------------------------------
struct arena_counter
{
  std::vector<u64> caps;
  u64              cap = 0, peak_cap = 0;
  u32              active = 0, peak_active = 0;
  void             on_add(u32 id, u64 size)
  {
    if (caps.size() <= id)
      caps.resize(id + 1, 0);
    caps[id] = size;
    cap += size;
    ++active;
    peak_cap    = std::max(peak_cap, cap);
    peak_active = std::max(peak_active, active);
  }
  void on_remove(u32 id)
  {
    cap -= caps[id];
    caps[id] = 0;
    --active;
  }
};

struct arena_mgr : arena_counter
{
  void add(ouly::arena_id id, ouly::allocation_size_type size, uint32_t)
  {
    on_add(id.get(), size);
  }
  void remove(ouly::arena_id id)
  {
    on_remove(id.get());
  }
};

struct sync_exec
{
  u64  copied = 0;
  u64  tl     = 1;
  auto schedule_copy(ouly::ca_relocation const& r) -> std::optional<ouly::ca_timeline_value>
  {
    copied += r.size_;
    return tl++;
  }
  auto switch_references(ouly::ca_relocation const&) -> std::optional<ouly::ca_timeline_value>
  {
    return tl++;
  }
  bool is_complete(ouly::ca_timeline_value) const
  {
    return true;
  }
};

// ------------------------------------------------------------------------------------------------
// Adapters: alloc(size) -> {handle, arena, offset}; free(handle); defrag() -> {bytes moved}
// ------------------------------------------------------------------------------------------------
struct placed
{
  u32 h, arena, offset;
};

template <u32 Align>
struct a_arena
{
  static constexpr u32             align = Align;
  static constexpr const char*     name  = "coalescing_arena_allocator";
  arena_mgr                        m;
  ouly::coalescing_arena_allocator a;
  explicit a_arena(u32 arena) : a(arena) {}
  placed alloc(u32 s, u32)
  {
    auto r = a.allocate(s, m, ouly::ca_allocation_options{.alignment_ = Align});
    return {r.get_allocation_id().get(), r.get_arena_id().get(), r.get_offset()};
  }
  void free(u32 h)
  {
    a.deallocate(ouly::allocation_id{h}, m);
  }
  static constexpr bool has_defrag = true;
  u64                   defrag()
  {
    sync_exec e;
    for (int i = 0; i < 100000; ++i)
    {
      auto r = a.defragment(m, e);
      if (!r.evacuation_active_ && r.moves_scheduled_ == 0)
        break;
    }
    return e.copied;
  }
};

// ------------------------------------------------------------------------------------------------
struct live_t
{
  u32 h = 0, arena = 0, offset = 0, size = 0;
};

static void check_overlap(std::vector<live_t> const& live, const char* name)
{
  std::vector<live_t> v = live;
  std::sort(v.begin(), v.end(),
            [](auto& a, auto& b)
            {
              return a.arena != b.arena ? a.arena < b.arena : a.offset < b.offset;
            });
  for (size_t i = 1; i < v.size(); ++i)
    if (v[i - 1].arena == v[i].arena && v[i - 1].offset + v[i - 1].size > v[i].offset)
    {
      std::printf("OVERLAP in %s: arena %u [%u,+%u) vs [%u,+%u)\n", name, v[i].arena, v[i - 1].offset, v[i - 1].size,
                  v[i].offset, v[i].size);
      std::abort();
    }
}

struct row
{
  double ns, frag;
  u32    arenas;
  double defrag_ms  = -1;
  u64    defrag_mv  = 0;
  u32    arenas_pre = 0, arenas_post = 0;
};

template <typename A>
static row run(trace const& t, u32 arena_size, std::string const& label)
{
  row r{1e30, 0, 0};
  for (int rep = 0; rep < (g_validate ? 1 : 3); ++rep)
  {
    auto                a = std::make_unique<A>(arena_size);
    std::vector<live_t> live(t.live);
    u64                 live_bytes = 0, peak_live = 0;
    size_t              si       = 0;
    auto                do_alloc = [&](u32 slot)
    {
      u32  s     = t.sizes[si++];
      auto p     = a->alloc(s, slot);
      live[slot] = {p.h, p.arena, p.offset, s};
      if (g_validate && p.offset % A::align != 0)
      {
        std::printf("MISALIGNED in %s: offset %u\n", label.c_str(), p.offset);
        std::abort();
      }
      live_bytes += s;
      peak_live = std::max(peak_live, live_bytes);
    };
    for (u32 i = 0; i < t.live; ++i)
      do_alloc(i);
    auto t0 = std::chrono::steady_clock::now();
    for (auto v : t.victim)
    {
      a->free(live[v].h);
      live_bytes -= live[v].size;
      do_alloc(v);
      if (g_validate)
        check_overlap(live, label.c_str());
    }
    auto t1  = std::chrono::steady_clock::now();
    r.ns     = std::min(r.ns, std::chrono::duration<double, std::nano>(t1 - t0).count() / t.victim.size());
    r.frag   = double(a->m.peak_cap) / double(peak_live);
    r.arenas = a->m.peak_active;

    if constexpr (A::has_defrag)
    {
      if (rep == 0)
      {
        // free a random half, then defragment once
        std::mt19937 rng(77);
        for (u32 i = 0; i < t.live; ++i)
          if (rng() & 1)
            a->free(live[i].h);
        r.arenas_pre  = a->m.active;
        auto d0       = std::chrono::steady_clock::now();
        r.defrag_mv   = a->defrag();
        auto d1       = std::chrono::steady_clock::now();
        r.defrag_ms   = std::chrono::duration<double, std::milli>(d1 - d0).count();
        r.arenas_post = a->m.active;
      }
    }
  }
  return r;
}

template <typename A>
static void print(trace const& t, u32 arena_size, std::string label)
{
  auto r = run<A>(t, arena_size, label);
  if (r.defrag_ms >= 0)
    std::printf("| %-38s | %9.1f | %6.3f | %5u | %9.2f | %8.1f | %4u -> %-4u |\n", label.c_str(), r.ns, r.frag,
                r.arenas, r.defrag_ms, r.defrag_mv / 1048576.0, r.arenas_pre, r.arenas_post);
  else
    std::printf("| %-38s | %9.1f | %6.3f | %5u | %9s | %8s | %11s |\n", label.c_str(), r.ns, r.frag, r.arenas, "-", "-",
                "-");
  std::fflush(stdout);
}

template <u32 Align>
static void multi_arena(trace const& t, u32 arena_size)
{
  std::printf("\n### %s, align=%u, arena=%uMB\n", t.name.c_str(), Align, arena_size >> 20);
  std::printf("| %-38s | %9s | %6s | %5s | %9s | %8s | %11s |\n", "allocator", "ns/pair", "frag", "arenas", "defrag ms",
              "moved MB", "arenas pre->post");
  std::printf("|---|---:|---:|---:|---:|---:|---|\n");
  print<a_arena<Align>>(t, arena_size, "coalescing_arena_allocator");
}

// ---------------------------------------------------------------- single range
static void single_range(trace const& t)
{
  std::printf("\n### single range: %s\n", t.name.c_str());
  std::printf("| %-30s | %9s | %8s | %10s | %8s |\n", "allocator", "ns/pair", "HWM/peak", "compact ms", "moved MB");
  std::printf("|---|---:|---:|---:|---:|\n");
  {
    double best = 1e30, hwm = 0, cms = 0;
    u64    moved = 0;
    for (int rep = 0; rep < (g_validate ? 1 : 3); ++rep)
    {
      ouly::coalescing_allocator a;
      std::vector<live_t>        live(t.live);
      u64                        lb = 0, pl = 0, high = 0;
      size_t                     si = 0;
      auto                       da = [&](u32 slot)
      {
        u32  s     = t.sizes[si++];
        auto id    = a.allocate(s);
        u32  o     = a.get_offset(id);
        live[slot] = {id.get(), 0, o, s};
        lb += s;
        pl   = std::max(pl, lb);
        high = std::max<u64>(high, u64{o} + s);
      };
      for (u32 i = 0; i < t.live; ++i)
        da(i);
      auto t0 = std::chrono::steady_clock::now();
      for (auto v : t.victim)
      {
        a.deallocate(ouly::allocation_id{live[v].h});
        lb -= live[v].size;
        da(v);
        if (g_validate)
        {
          for (auto& l : live)
            l.offset = a.get_offset(ouly::allocation_id{l.h});
          check_overlap(live, "coalescing_allocator");
        }
      }
      auto t1 = std::chrono::steady_clock::now();
      best    = std::min(best, std::chrono::duration<double, std::nano>(t1 - t0).count() / t.victim.size());
      hwm     = double(high) / pl;
      if (rep == 0 && !g_validate)
      {
        std::mt19937 rng(77);
        for (u32 i = 0; i < t.live; ++i)
          if (rng() & 1)
            a.deallocate(ouly::allocation_id{live[i].h});
        auto c0 = std::chrono::steady_clock::now();
        auto r  = a.compact([](u32, u32, u32) {});
        auto c1 = std::chrono::steady_clock::now();
        cms     = std::chrono::duration<double, std::milli>(c1 - c0).count();
        moved   = r.bytes_moved_;
      }
    }
    std::printf("| %-30s | %9.1f | %8.3f | %10.2f | %8.1f |\n", "coalescing_allocator", best, hwm, cms,
                moved / 1048576.0);
  }
}

int main(int argc, char** argv)
{
  g_validate = argc > 1 && std::string(argv[1]) == "validate";
  if (g_validate)
  {
    auto t = make_trace("validate", 400, 3000, 256, 1 << 20, 256);
    multi_arena<1>(t, 16u << 20);
    multi_arena<256>(t, 16u << 20);
    auto s = make_trace("validate single", 400, 3000, 16, 65536, 16);
    single_range(s);
    std::printf("\nvalidation OK\n");
    return 0;
  }
  u32 steps = 100000;
  for (u32 live : {1000u, 10000u, 50000u})
  {
    auto t = make_trace("256B-1MB log-uniform, live=" + std::to_string(live), live, steps, 256, 1 << 20, 256);
    multi_arena<1>(t, 64u << 20);
    multi_arena<256>(t, 64u << 20);
  }
  for (u32 live : {1000u, 10000u, 50000u})
    single_range(make_trace("16B-64KB, live=" + std::to_string(live), live, steps, 16, 65536, 16));
}
// NOLINTEND
