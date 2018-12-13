#pragma once

#include <assert.h>
#include <cstdio>
#include <string.h>

struct ArenaAllocator
{
    void* m_begin;
    void* m_end;
    
    // Current position
    char* m_curr;
    
    // For this ArenaAllocator, we will always align to 8 byte boundries
    static constexpr int ALIGNMENT = 8;
    
    // accept begining and ending pointers of arena
    ArenaAllocator(void* begin, void* end)
    : m_begin(begin),
    m_end(end),
    m_curr(static_cast<char*>(m_begin))
    {
        
    }
    
    // Allocates section of memory
    void* Allocate(size_t sizeBytes)
    {
        // Alignes up to the next 8 byte boundy
        m_curr = (char*) ( ((uintptr_t) m_curr + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1) );
        
        // Out of memory
        assert((m_curr + sizeBytes) < m_end);
        
        printf("Allocated %d bytes\n", (int) sizeBytes);
        void* ptr = m_curr;
        m_curr += sizeBytes;
        
        return ptr;
    }
    
    void DeAllocate(void* ptr, size_t osize)
    {
        // Burning through memory for performance reasons.
        // Memory will be freed when the Arena goes out of scope
        
        
        assert(ptr != nullptr); // Ensure we are not deallocating a nullptr
        printf("DeAllocate %d bytes\n", (int) osize);
    }
    
    void* ReAllocate(void* ptr, size_t osize, size_t nsize)
    {
        printf("ReAllocated %d bytes to %d bytes\n", (int) osize, (int) nsize);
        void* newPtr = Allocate(nsize);
        memcpy(newPtr, ptr, osize);
        DeAllocate(ptr, osize);
        return newPtr;
    }
    
    
    /*
     ud: user data
     ptr: usage pointer
     osize: old size of memory block
     nsize: new size of memory block
     */
    static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
        ArenaAllocator* pool = static_cast<ArenaAllocator*>(ud);
        
        if (nsize == 0) {
            
            if (ptr)
            {
                pool->DeAllocate(ptr, osize);
            }
            
            return NULL;
        }
        else
        {
            // Allocation
            if (ptr == nullptr)
            {
                return pool->Allocate(nsize);
            }
            // Reallocation
            else
            {
                return pool->ReAllocate(ptr, osize, nsize);
            }
            
        }
    }
};
