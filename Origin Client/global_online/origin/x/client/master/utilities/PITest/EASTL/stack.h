///////////////////////////////////////////////////////////////////////////////
// EASTL/stack.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a stack that is just like the C++ std::stack adapter class.
// The only significant difference is that the stack here provides a get_container
// function to provide access to the underlying container.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_STACK_H
#define EASTL_STACK_H


#include <EASTL/internal/config.h>
#include <EASTL/vector.h>
#include <stddef.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
    #pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{

    /// EASTL_STACK_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_STACK_DEFAULT_NAME
        #define EASTL_STACK_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " stack" // Unless the user overrides something, this is "EASTL stack".
    #endif

    /// EASTL_STACK_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_STACK_DEFAULT_ALLOCATOR
        #define EASTL_STACK_DEFAULT_ALLOCATOR allocator_type(EASTL_STACK_DEFAULT_NAME)
    #endif



    /// stack
    ///
    /// stack is an adapter class provides a LIFO (last-in, first-out) interface
    /// via wrapping a sequence that provides at least the following operations:
    ///     push_back
    ///     pop_back
    ///     back
    ///
    /// In practice this means vector, deque, string, list, intrusive_list. 
    ///
    template <typename T, typename Container = eastl::vector<T> >
    class stack
    {
    public:
        typedef stack<T, Container>                  this_type;
        typedef          Container                   container_type;
        typedef typename Container::value_type       value_type;
        typedef typename Container::reference        reference;
        typedef typename Container::const_reference  const_reference;
        typedef typename Container::size_type        size_type;

    public:          // We declare public so that global comparison operators can be implemented without adding an inline level and without tripping up GCC 2.x friend declaration failures. GCC (through at least v4.0) is poor at inlining and performance wins over correctness.
        Container c; // The C++ standard specifies that you declare a protected member variable of type Container called 'c'.

    public:
        stack();
        explicit stack(const Container& x);

        bool      empty() const;
        size_type size() const;

        reference       top();
        const_reference top() const;

        void push(const value_type& value);
        void pop();

        container_type&       get_container();
        const container_type& get_container() const;

    }; // class stack





    ///////////////////////////////////////////////////////////////////////
    // stack
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Container>
    inline stack<T, Container>::stack()
        : c() // To consider: use c(EASTL_STACK_DEFAULT_ALLOCATOR) here, though that would add the requirement that the user supplied container support this.
    {
        // Empty
    }

    template <typename T, typename Container>
    inline stack<T, Container>::stack(const Container& x)
        : c(x)
    {
        // Empty
    }


    template <typename T, typename Container>
    inline bool stack<T, Container>::empty() const
    {
        return c.empty();
    }


    template <typename T, typename Container>
    inline typename stack<T, Container>::size_type
    stack<T, Container>::size() const
    {
        return c.size();
    }


    template <typename T, typename Container>
    inline typename stack<T, Container>::reference
    stack<T, Container>::top()
    {
        return c.back();
    }


    template <typename T, typename Container>
    inline typename stack<T, Container>::const_reference
    stack<T, Container>::top() const
    {
        return c.back();
    }


    template <typename T, typename Container>
    inline void stack<T, Container>::push(const value_type& value)
    {
        c.push_back(const_cast<value_type&>(value)); // const_cast so that intrusive_list can work. We may revisit this.
    }


    template <typename T, typename Container>
    inline void stack<T, Container>::pop()
    {
        c.pop_back();
    }


    template <typename T, typename Container>
    inline typename stack<T, Container>::container_type&
    stack<T, Container>::get_container()
    {
        return c;
    }


    template <typename T, typename Container>
    inline const typename stack<T, Container>::container_type&
    stack<T, Container>::get_container() const
    {
        return c;
    }



    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename T, typename Container>
    inline bool operator==(const stack<T, Container>& a, const stack<T, Container>& b)
    {
        return (a.c == b.c);
    }


    template <typename T, typename Container>
    inline bool operator!=(const stack<T, Container>& a, const stack<T, Container>& b)
    {
        return !(a.c == b.c);
    }


    template <typename T, typename Container>
    inline bool operator<(const stack<T, Container>& a, const stack<T, Container>& b)
    {
        return (a.c < b.c);
    }


    template <typename T, typename Container>
    inline bool operator>(const stack<T, Container>& a, const stack<T, Container>& b)
    {
        return (b.c < a.c);
    }


    template <typename T, typename Container>
    inline bool operator<=(const stack<T, Container>& a, const stack<T, Container>& b)
    {
        return !(b.c < a.c);
    }


    template <typename T, typename Container>
    inline bool operator>=(const stack<T, Container>& a, const stack<T, Container>& b)
    {
        return !(a.c < b.c);
    }


} // namespace eastl


#endif // Header include guard













