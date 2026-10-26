#pragma once

#include <utility>

namespace learn::test
{

/** Conta as alocações de memória feitas *nesta thread* enquanto o objeto existe.

    Em Debug com MSVC usa o hook do heap de debug da CRT (_CrtSetAllocHook), que apanha
    new, malloc, realloc, etc. Noutras configurações substitui o operator new global
    (apanha new/delete, mas não malloc direto).

    Uso típico num teste:

        learn::test::ScopedAllocationCounter counter;
        processor.processBlock (buffer);
        REQUIRE (counter.getCount() == 0);
*/
class ScopedAllocationCounter
{
public:
    ScopedAllocationCounter() noexcept;
    ~ScopedAllocationCounter() noexcept;

    ScopedAllocationCounter (const ScopedAllocationCounter&) = delete;
    ScopedAllocationCounter& operator= (const ScopedAllocationCounter&) = delete;

    /** Número de alocações desde a construção. */
    int getCount() const noexcept;

private:
    int startCount = 0;
};

/** Executa fn() e devolve o número de alocações que fez. */
template <typename Fn>
int countAllocations (Fn&& fn)
{
    ScopedAllocationCounter counter;
    std::forward<Fn> (fn)();
    return counter.getCount();
}

/** true se a deteção apanha também malloc/realloc (e não só new). */
bool allocationTrackingCoversMalloc() noexcept;

} // namespace learn::test
