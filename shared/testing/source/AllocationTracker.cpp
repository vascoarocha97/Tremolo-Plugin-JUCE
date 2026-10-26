#include <learn_testing/RealtimeChecks.h>

#include <cstdlib>
#include <new>

#if defined(_MSC_VER) && defined(_DEBUG)
 #include <crtdbg.h>
 #define LEARN_USE_CRT_ALLOC_HOOK 1
#else
 #define LEARN_USE_CRT_ALLOC_HOOK 0
#endif

namespace learn::test
{
namespace
{
    // thread_local triviais: aceder-lhes não aloca memória.
    thread_local int trackingDepth = 0;
    thread_local int allocationCount = 0;

    inline void noteAllocation() noexcept
    {
        if (trackingDepth > 0)
            ++allocationCount;
    }
}

ScopedAllocationCounter::ScopedAllocationCounter() noexcept
{
    ++trackingDepth;
    startCount = allocationCount;
}

ScopedAllocationCounter::~ScopedAllocationCounter() noexcept
{
    --trackingDepth;
}

int ScopedAllocationCounter::getCount() const noexcept
{
    return allocationCount - startCount;
}

bool allocationTrackingCoversMalloc() noexcept
{
    return LEARN_USE_CRT_ALLOC_HOOK != 0;
}

} // namespace learn::test

#if LEARN_USE_CRT_ALLOC_HOOK

//==============================================================================
// MSVC Debug: o heap de debug chama este hook em cada alocação (new, malloc, realloc...).
namespace
{
    int __cdecl learnAllocHook (int allocType, void*, size_t, int blockType, long,
                                const unsigned char*, int)
    {
        if (blockType != _CRT_BLOCK && (allocType == _HOOK_ALLOC || allocType == _HOOK_REALLOC))
            learn::test::noteAllocation();

        return 1; // TRUE: deixa a alocação prosseguir
    }

    const bool hookInstalled = []
    {
        _CrtSetAllocHook (learnAllocHook);
        return true;
    }();
}

#else

//==============================================================================
// Outras configurações: substituir o operator new global (forma portável).
namespace
{
    void* allocateOrThrow (std::size_t size)
    {
        learn::test::noteAllocation();

        if (auto* p = std::malloc (size == 0 ? 1 : size))
            return p;

        throw std::bad_alloc();
    }

    void* allocateAligned (std::size_t size, std::align_val_t alignment) noexcept
    {
        learn::test::noteAllocation();
        const auto align = static_cast<std::size_t> (alignment);
       #if defined(_MSC_VER)
        return _aligned_malloc (size == 0 ? 1 : size, align);
       #else
        void* p = nullptr;
        return posix_memalign (&p, align, size == 0 ? 1 : size) == 0 ? p : nullptr;
       #endif
    }

    void freeAligned (void* p) noexcept
    {
       #if defined(_MSC_VER)
        _aligned_free (p);
       #else
        std::free (p);
       #endif
    }
}

void* operator new (std::size_t size)                                   { return allocateOrThrow (size); }
void* operator new[] (std::size_t size)                                 { return allocateOrThrow (size); }
void* operator new (std::size_t size, const std::nothrow_t&) noexcept   { learn::test::noteAllocation(); return std::malloc (size == 0 ? 1 : size); }
void* operator new[] (std::size_t size, const std::nothrow_t&) noexcept { learn::test::noteAllocation(); return std::malloc (size == 0 ? 1 : size); }

void operator delete (void* p) noexcept                                 { std::free (p); }
void operator delete[] (void* p) noexcept                               { std::free (p); }
void operator delete (void* p, std::size_t) noexcept                    { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept                  { std::free (p); }
void operator delete (void* p, const std::nothrow_t&) noexcept          { std::free (p); }
void operator delete[] (void* p, const std::nothrow_t&) noexcept        { std::free (p); }

void* operator new (std::size_t size, std::align_val_t a)
{
    if (auto* p = allocateAligned (size, a))
        return p;

    throw std::bad_alloc();
}

void* operator new[] (std::size_t size, std::align_val_t a)                          { return operator new (size, a); }
void* operator new (std::size_t size, std::align_val_t a, const std::nothrow_t&) noexcept   { return allocateAligned (size, a); }
void* operator new[] (std::size_t size, std::align_val_t a, const std::nothrow_t&) noexcept { return allocateAligned (size, a); }

void operator delete (void* p, std::align_val_t) noexcept                            { freeAligned (p); }
void operator delete[] (void* p, std::align_val_t) noexcept                          { freeAligned (p); }
void operator delete (void* p, std::size_t, std::align_val_t) noexcept               { freeAligned (p); }
void operator delete[] (void* p, std::size_t, std::align_val_t) noexcept             { freeAligned (p); }
void operator delete (void* p, std::align_val_t, const std::nothrow_t&) noexcept     { freeAligned (p); }
void operator delete[] (void* p, std::align_val_t, const std::nothrow_t&) noexcept   { freeAligned (p); }

#endif
