#pragma once

#include <assert.h>
#include <cstdio>
#include <string.h>

/*
 Memory alignment concerns:
 - Multi-threading safety
 - SIMD operations on specific memory boundries
 
 */
struct ArenaAllocator
{
    void* m_begin;
    void* m_end;
    
    // Current position
    char* m_curr;
    
    // For this ArenaAllocator, we will always align to 8 byte boundries
    static constexpr int ALIGNMENT = 8;
    
    // Minimum size to allocate
    static constexpr int MIN_BLOCK_SIZE = ALIGNMENT * 8;
    
    // Linkedlist of free memory blocks (MIN_BLOCK_SIZE)
    struct FreeList
    {
        FreeList* m_next;
    };
    
    FreeList* m_freeListHead;
    
    // accept begining and ending pointers of arena
    ArenaAllocator(void* begin, void* end)
    : m_begin(begin),
    m_end(end)
    {
        Reset();
    }
    
    void Reset()
    {
        m_freeListHead = nullptr;
        m_curr = static_cast<char*>(m_begin);
    }
    
    size_t SizeToAllocate(size_t size)
    {
        size_t allocatedSize = size;
        if (allocatedSize < MIN_BLOCK_SIZE)
        {
            allocatedSize = MIN_BLOCK_SIZE;
        }
        return allocatedSize;
    }
    
    // Allocates section of memory
    void* Allocate(size_t sizeBytes)
    {
        size_t allocatedBytes = SizeToAllocate(sizeBytes);
        
        // Allocate memory from free list
        if (allocatedBytes <= MIN_BLOCK_SIZE && m_freeListHead)
        {
            //printf("-- allocated from the freelist --\n");
            void* ptr = m_freeListHead;
            m_freeListHead = m_freeListHead->m_next;
            return ptr;
        }
        
        // Allocate memory from pool
        else
        {
            // Alignes up to the next 8 byte boundry
            m_curr = (char*) ( ((uintptr_t) m_curr + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1) );
            
            // Out of memory
            assert((m_curr + allocatedBytes) < m_end);
            
            //printf("Allocated %d bytes\n", (int) allocatedBytes);
            void* ptr = m_curr;
            m_curr += allocatedBytes;
            
            return ptr;
        }
    }
    
    void DeAllocate(void* ptr, size_t osize)
    {
        size_t allocatedBytes = SizeToAllocate(osize);
        assert(ptr != nullptr); // Ensure we are not deallocating a nullptr
        //printf("DeAllocate %d bytes\n", (int) allocatedBytes);
        
        if (allocatedBytes >= MIN_BLOCK_SIZE)
        {
            //printf("-- deallocated to the freelist --\n");
            FreeList* newHead = static_cast<FreeList*>(ptr);
            newHead->m_next = m_freeListHead;
            m_freeListHead = newHead;
        }
        else
        {
            // Burn memory
        }
    }
    
    void* ReAllocate(void* ptr, size_t osize, size_t nsize)
    {
        //printf("ReAllocated %d bytes to %d bytes\n", (int) osize, (int) nsize);
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
    static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize)
    {
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
