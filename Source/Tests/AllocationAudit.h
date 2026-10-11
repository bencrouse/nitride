#pragma once
#include <cstdint>

namespace Nitride::Audit
{
#if NITRIDE_ALLOCATION_AUDIT
extern thread_local bool renderScope;
extern thread_local std::uint64_t allocations,deallocations;
#else
inline thread_local bool renderScope=false;
inline thread_local std::uint64_t allocations=0,deallocations=0;
#endif
struct Scope
{
    Scope(){renderScope=true;}
    ~Scope(){renderScope=false;}
};
}
