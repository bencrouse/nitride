#include "AllocationAudit.h"
#include <malloc/malloc.h>
#include <cstdlib>
#include <cerrno>

namespace Nitride::Audit
{
thread_local bool renderScope=false;
thread_local std::uint64_t allocations=0,deallocations=0;
}

namespace
{
void allocation(){if(Nitride::Audit::renderScope)++Nitride::Audit::allocations;}
void deallocation(){if(Nitride::Audit::renderScope)++Nitride::Audit::deallocations;}
void* checkedMalloc(size_t size){allocation();return malloc_zone_malloc(malloc_default_zone(),size);}
void* checkedCalloc(size_t count,size_t size){allocation();return malloc_zone_calloc(malloc_default_zone(),count,size);}
void* checkedRealloc(void* data,size_t size){allocation();if(data)deallocation();return malloc_zone_realloc(data?malloc_zone_from_ptr(data):malloc_default_zone(),data,size);}
void checkedFree(void* data){if(data){deallocation();malloc_zone_free(malloc_zone_from_ptr(data),data);}}
int checkedAligned(void** data,size_t alignment,size_t size)
{
    if(alignment<sizeof(void*)||(alignment&(alignment-1))!=0)return EINVAL;
    allocation();*data=malloc_zone_memalign(malloc_default_zone(),alignment,size);return *data?0:ENOMEM;
}
#define NITRIDE_INTERPOSE(replacement,original) \
    __attribute__((used)) static const struct {const void* substitute;const void* source;} interpose_##original \
    __attribute__((section("__DATA,__interpose")))={reinterpret_cast<const void*>(&replacement),reinterpret_cast<const void*>(&original)};
NITRIDE_INTERPOSE(checkedMalloc,malloc)
NITRIDE_INTERPOSE(checkedCalloc,calloc)
NITRIDE_INTERPOSE(checkedRealloc,realloc)
NITRIDE_INTERPOSE(checkedFree,free)
NITRIDE_INTERPOSE(checkedAligned,posix_memalign)
}
