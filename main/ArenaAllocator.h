#pragma once

#include <assert.h>
#include <cstdio>
#include <string.h>

template <typename T>
class IAllocator {
    
public:
    virtual ~IAllocator()
    { }
    
    virtual void*   Allocate(size_t sizeBytes) = 0;
    virtual void    DeAllocate(void* ptr, size_t osize) = 0;
    virtual void*   ReAllocate(void* ptr, size_t osize, size_t nsize) = 0;
    
    /*
     ud: user data
     ptr: usage pointer
     osize: old size of memory block
     nsize: new size of memory block
     */
    static void* l_alloc (void *ud, void *ptr, size_t osize, size_t nsize)
    {
        T* allocator = static_cast<T*>(ud);
        
        if (nsize == 0) {
            
            if (ptr)
            {
                allocator->DeAllocate(ptr, osize);
            }
            
            return NULL;
        }
        else
        {
            // Allocation
            if (ptr == nullptr)
            {
                return allocator->Allocate(nsize);
            }
            // Reallocation
            else
            {
                return allocator->ReAllocate(ptr, osize, nsize);
            }
            
        }
    }
    
};

/* Allocates from global memory */
struct GlobalAllocator
    : IAllocator<GlobalAllocator>
{
    void* Allocate(size_t sizeBytes) override
    {
        return ::operator new(sizeBytes);
    }
    
    void DeAllocate(void* ptr, size_t /*osize*/) override
    {
        assert(ptr != nullptr); // Ensure we are not deallocating a nullptr
        ::operator delete(ptr);
    }
    
    void* ReAllocate(void* ptr, size_t osize, size_t nsize) override
    {
        
        // Get minimum size between osize and nsize
        size_t bytesToCopy = osize;
        if (nsize < bytesToCopy)
        {
            bytesToCopy = nsize;
        }
        
        void* newPtr = Allocate(nsize);
        memcpy(newPtr, ptr, bytesToCopy);
        DeAllocate(ptr, osize);
        return newPtr;
    }
    
};


/*
 Allocates from a fixed pool.
 Aligns all memory to 8 bytes
 Has a min allocation of 64 bytes
 Puts all free'd blocks onto free linked list
 When out of memory, falls back onto GlobalAllocator
 
 Memory alignment concerns:
 - Multi-threading safety
 - SIMD operations on specific memory boundries
 */
struct ArenaAllocator
    : public IAllocator<ArenaAllocator>
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
    GlobalAllocator m_globalAllocator;
    
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
    void* Allocate(size_t sizeBytes) override
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
            if ((m_curr + allocatedBytes) <= m_end)
            {
                
                //printf("Allocated %d bytes\n", (int) allocatedBytes);
                void* ptr = m_curr;
                m_curr += allocatedBytes;
                
                return ptr;
            }
            
            // Out of memory? Fallback on global allocator
            else
            {
                return m_globalAllocator.Allocate(sizeBytes);
            }
        
        }
    }
    
    void DeAllocate(void* ptr, size_t osize) override
    {
        size_t allocatedBytes = SizeToAllocate(osize);
        assert(ptr != nullptr); // Ensure we are not deallocating a nullptr
        
        // Dellocate from pool
        if (ptr >= m_begin && ptr <= m_end)
        {
            if (allocatedBytes >= MIN_BLOCK_SIZE)
            {
                //printf("-- deallocated to the freelist --\n");
                FreeList* newHead = static_cast<FreeList*>(ptr);
                newHead->m_next = m_freeListHead;
                m_freeListHead = newHead;
            }
        }
        
        // Dellocate memory from global
        else
        {
            m_globalAllocator.DeAllocate(ptr, osize);
        }
    }
    
    void* ReAllocate(void* ptr, size_t osize, size_t nsize) override
    {
        //printf("ReAllocated %d bytes to %d bytes\n", (int) osize, (int) nsize);
        size_t bytesToCopy = osize;
        if (nsize < bytesToCopy)
        {
            bytesToCopy = nsize;
        }
        void* newPtr = Allocate(nsize);
        memcpy(newPtr, ptr, bytesToCopy);
        DeAllocate(ptr, osize);
        return newPtr;
    }
    
};
