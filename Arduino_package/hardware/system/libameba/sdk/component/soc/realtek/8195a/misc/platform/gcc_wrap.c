/**************************************************
 * malloc/free/realloc wrap for gcc compiler
 *
 **************************************************/
#if defined(__GNUC__)
#include "FreeRTOS.h"

void* __wrap_malloc( size_t size )
{
    return pvPortMalloc(size);
}

void* __wrap_realloc( void *p, size_t size )
{
    return pvPortReAlloc(p, size);
}

void __wrap_free( void *p )
{
    vPortFree(p);
}

/* For GCC stdlib */
void* __wrap__malloc_r( void * reent, size_t size )
{
    return pvPortMalloc(size);
}

void* __wrap__realloc_r( void * reent, void *p, size_t size )
{
    return pvPortReAlloc(p, size);
}

void __wrap__free_r( void * reent, void *p )
{
    vPortFree(p);
}

#endif