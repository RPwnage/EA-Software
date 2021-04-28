#ifndef __ORIGINSDK_MEMORY_H
#define __ORIGINSDK_MEMORY_H

#include <string>

#define TYPE_NEW(type, length) (type *)Origin::AllocFunc(sizeof(type) * (length))
#define TYPE_DELETE(data) Origin::FreeFunc(data)

namespace Origin
{
    // implemented in OriginMemoryStub.cpp
    void FreezeMemoryAllocators ();
    void ThawMemoryAllocators ();
    void* AllocFunc(size_t);
    void FreeFunc(void*);
    void* AllocAlignFunc(size_t, size_t);
    void FreeAlignFunc(void*);
    
    template <class T>
    class Allocator 
    {
    public:
        typedef T              value_type;

        typedef std::size_t    size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T*             pointer;
        typedef const T*       const_pointer;
        typedef T&             reference;
        typedef const T&       const_reference;

        pointer address(reference r) const             { return &r; }
        const_pointer address(const_reference r) const { return &r; }

        Allocator() throw() {}

        template <class U>
        Allocator(const Allocator<U>&) throw() {}

        ~Allocator() throw() {}

        pointer allocate(size_type n, void * = 0) 
        {
            return (pointer)Origin::AllocFunc(n * sizeof(T));
        }

        void deallocate(pointer p, size_type /*n*/) 
        {
            Origin::FreeFunc(p);
        }

        void construct(pointer p, const T& val) { new(p) T(val); }
        void destroy(pointer p) { (void)p; p->~T(); }

        size_type max_size() const throw() { return 0xffffffff; }

        template <class U>
        struct rebind { typedef Allocator<U> other; };
    };

    template <class T1, class T2>
    bool operator!=(const Allocator<T1>& lhs, const Allocator<T2>& rhs) throw()
    {
        return &lhs != &rhs;
    }

    template <class T1, class T2>
    bool operator==(const Allocator<T1>& lhs, const Allocator<T2>& rhs) throw()
    {
        return &lhs == &rhs;
    }
        
    typedef std::basic_string<char, std::char_traits<char>, Allocator<char> > AllocString;
}

#endif //__ORIGINSDK_MEMORY_H
