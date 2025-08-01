// SPDX-License-Identifier: MIT

#include "ouly/allocators/detail/platform_memory.hpp"
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace ouly::detail
{

#ifdef _WIN32
inline static auto protection_to_win32(cfg::protection prot) noexcept -> DWORD
{
  switch (prot)
  {
  case cfg::protection::none:
    return PAGE_NOACCESS;
  case cfg::protection::read:
    return PAGE_READONLY;
  case cfg::protection::write:
  case cfg::protection::read_write:
    return PAGE_READWRITE;
  // case cfg::protection::execute:
  //  return PAGE_EXECUTE;
  // case cfg::protection::read_execute:
  //  return PAGE_EXECUTE_READ;
  // case cfg::protection::write_execute:
  // case cfg::protection::read_write_execute:
  //  return PAGE_EXECUTE_READWRITE;
  default:
    return PAGE_NOACCESS;
  }
}

inline static auto map_flags_to_win32(map_flags flags) noexcept -> DWORD
{
  DWORD result = 0;

  if ((static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(map_flags::shared)) != 0)
  {
    result |= SEC_COMMIT;
  }

  return result;
}
#else

constexpr mode_t default_file_mode = 0644;

inline static auto protection_to_posix(cfg::protection prot) noexcept -> int
{
  int result = PROT_NONE;

  if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::read)) != 0)
  {
    result |= PROT_READ;
  }
  if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::write)) != 0)
  {
    result |= PROT_WRITE;
  }
  // if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::execute)) != 0)
  // {
  //   result |= PROT_EXEC;
  // }

  return result;
}

inline static auto map_flags_to_posix(map_flags flags) noexcept -> int
{
  int result = 0;

  if ((static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(map_flags::private_map)) != 0)
  {
    result |= MAP_PRIVATE;
  }
  if ((static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(map_flags::shared)) != 0)
  {
    result |= MAP_SHARED;
  }
  if ((static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(map_flags::anonymous)) != 0)
  {
    result |= MAP_ANONYMOUS;
  }
  if ((static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(map_flags::fixed)) != 0)
  {
    result |= MAP_FIXED;
  }

  return result;
}

inline static auto advice_to_posix(cfg::advice advice_type) noexcept -> int
{
  switch (advice_type)
  {
  case cfg::advice::normal:
    return MADV_NORMAL;
  case cfg::advice::random:
    return MADV_RANDOM;
  case cfg::advice::sequential:
    return MADV_SEQUENTIAL;
  case cfg::advice::will_need:
    return MADV_WILLNEED;
  case cfg::advice::dont_need:
    return MADV_DONTNEED;
  default:
    return MADV_NORMAL;
  }
}
#endif
auto get_memory_info() noexcept -> memory_info
{
  memory_info info{};

#ifdef _WIN32
  SYSTEM_INFO sys_info;
  GetSystemInfo(&sys_info);
  info.page_size_              = sys_info.dwPageSize;
  info.allocation_granularity_ = sys_info.dwAllocationGranularity;
#else
  info.page_size_              = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
  info.allocation_granularity_ = info.page_size_; // On Unix, allocation granularity == page size
#endif

  return info;
}

auto virtual_alloc(std::size_t size, cfg::protection prot, void* preferred_address) noexcept -> void*
{
#ifdef _WIN32
  DWORD allocation_type = MEM_COMMIT | MEM_RESERVE;
  DWORD protect         = protection_to_win32(prot);
  return VirtualAlloc(preferred_address, size, allocation_type, protect);
#else
  int posix_prot = protection_to_posix(prot);
  int flags      = MAP_PRIVATE | MAP_ANONYMOUS;
  if (preferred_address != nullptr)
  {
    flags |= MAP_FIXED;
  }

  void* result = mmap(preferred_address, size, posix_prot, flags, -1, 0);
  return (result == MAP_FAILED) ? nullptr : result;
#endif
}

auto virtual_free(void* ptr, std::size_t size) noexcept -> bool
{
  if (ptr == nullptr)
  {
    return true;
  }

#ifdef _WIN32
  (void)size; // Size not needed on Windows
  return VirtualFree(ptr, 0, MEM_RELEASE) != FALSE;
#else
  return munmap(ptr, size) == 0;
#endif
}

auto virtual_protect(void* ptr, std::size_t size, cfg::protection new_prot) noexcept -> bool
{
  if (ptr == nullptr)
  {
    return false;
  }

#ifdef _WIN32
  DWORD old_protect;
  DWORD protect = protection_to_win32(new_prot);
  return VirtualProtect(ptr, size, protect, &old_protect) != FALSE;
#else
  int posix_prot = protection_to_posix(new_prot);
  return mprotect(ptr, size, posix_prot) == 0;
#endif
}

auto map_file(std::filesystem::path const& filename, std::size_t size, cfg::protection prot,
              [[maybe_unused]] map_flags flags, bool create_if_missing) noexcept -> mapped_file_info
{
  if (filename.empty())
  {
    return mapped_file_info{};
  }

#ifdef _WIN32
  DWORD access   = 0;
  DWORD creation = create_if_missing ? OPEN_ALWAYS : OPEN_EXISTING;

  if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::read)) != 0)
  {
    access |= GENERIC_READ;
  }
  if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::write)) != 0)
  {
    access |= GENERIC_WRITE;
  }

  HANDLE file_handle = CreateFileW(filename.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, creation,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file_handle == INVALID_HANDLE_VALUE)
  {
    return mapped_file_info{};
  }

  if (size == 0)
  {
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size))
    {
      CloseHandle(file_handle);
      return {};
    }
    size = static_cast<std::size_t>(file_size.QuadPart);
  }

  DWORD  protect        = protection_to_win32(prot);
  HANDLE mapping_handle = CreateFileMapping(file_handle, nullptr, protect, static_cast<DWORD>(size >> 32),
                                            static_cast<DWORD>(size & 0xFFFFFFFF), nullptr);
  CloseHandle(file_handle);

  if (mapping_handle == nullptr)
  {
    return {};
  }

  DWORD map_access = 0;
  if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::read)) != 0)
  {
    map_access |= FILE_MAP_READ;
  }
  if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::write)) != 0)
  {
    map_access |= FILE_MAP_WRITE;
  }
  // if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::execute)) != 0)
  // {
  //   map_access |= FILE_MAP_EXECUTE;
  // }

  void* ptr = MapViewOfFile(mapping_handle, map_access, 0, 0, size);
  CloseHandle(mapping_handle);

  return {ptr, size, prot};
#else
  int open_flags = O_RDONLY;
  if ((static_cast<std::uint8_t>(prot) & static_cast<std::uint8_t>(cfg::protection::write)) != 0)
  {
    open_flags = O_RDWR;
  }

  if (create_if_missing)
  {
    open_flags |= O_CREAT;
  }

  // NOLINTNEXTLINE
  int fd = open(filename.c_str(), open_flags, default_file_mode);
  if (fd == -1)
  {
    return {};
  }

  if (size == 0)
  {
    struct stat st = {};
    if (fstat(fd, &st) == -1)
    {
      close(fd);
      return {};
    }
    size = static_cast<std::size_t>(st.st_size);
  }

  int posix_prot  = protection_to_posix(prot);
  int posix_flags = map_flags_to_posix(flags);

  void* result = mmap(nullptr, size, posix_prot, posix_flags, fd, 0);
  close(fd);

  return (result == MAP_FAILED) ? mapped_file_info{}
                                : mapped_file_info{.address_ = result, .size_ = size, .prot_ = prot};
#endif
}

auto map_anonymous(std::size_t size, cfg::protection prot, void* preferred_address) noexcept -> void*
{
#ifdef _WIN32
  (void)preferred_address; // Not directly supported
  return virtual_alloc(size, prot, preferred_address);
#else
  int posix_prot = protection_to_posix(prot);
  int flags      = MAP_PRIVATE | MAP_ANONYMOUS;
  if (preferred_address != nullptr)
  {
    flags |= MAP_FIXED;
  }

  void* result = mmap(preferred_address, size, posix_prot, flags, -1, 0);
  return (result == MAP_FAILED) ? nullptr : result;
#endif
}

auto unmap(void* ptr, std::size_t size) noexcept -> bool
{
  if (ptr == nullptr)
  {
    return true;
  }

#ifdef _WIN32
  (void)size; // Size not needed on Windows
  return UnmapViewOfFile(ptr) != FALSE;
#else
  return munmap(ptr, size) == 0;
#endif
}

auto sync(void* ptr, std::size_t size, bool async) noexcept -> bool
{
  if (ptr == nullptr)
  {
    return true;
  }

#ifdef _WIN32
  (void)async; // Windows FlushViewOfFile is always synchronous
  return FlushViewOfFile(ptr, size) != FALSE;
#else
  int flags = async ? MS_ASYNC : MS_SYNC;
  return msync(ptr, size, flags) == 0;
#endif
}

auto advise(void* ptr, std::size_t size, cfg::advice advice_type) noexcept -> bool
{
  if (ptr == nullptr)
  {
    return true;
  }

#ifdef _WIN32
  // Windows doesn't have direct equivalent to madvise, but we can use PrefetchVirtualMemory
  // for will_need cfg::advice on Windows 8+
  (void)size;
  (void)advice_type;
  return true; // No-op on Windows for now
#else
  int posix_advice = advice_to_posix(advice_type);
  return madvise(ptr, size, posix_advice) == 0;
#endif
}

} // namespace ouly::detail
