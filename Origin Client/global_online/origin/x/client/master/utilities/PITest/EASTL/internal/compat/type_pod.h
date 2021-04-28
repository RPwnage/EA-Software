///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/type_pod.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_TYPE_POD_H
#define EASTL_INTERNAL_TYPE_POD_H


#include <limits.h>


namespace eastl
{


    // The following properties or relations are defined here. If the given 
    // item is missing then it simply hasn't been implemented, at least not yet.
    //    is_empty
    //    is_pod
    //    has_trivial_constructor
    //    has_trivial_copy
    //    has_trivial_assign
    //    has_trivial_destructor
    //    has_trivial_relocate       -- EA extension to the C++ standard proposal.
    //    has_nothrow_constructor
    //    has_nothrow_copy
    //    has_nothrow_assign
    //    has_virtual_destructor




    ///////////////////////////////////////////////////////////////////////
    // is_empty
    // 
    // is_empty<T>::value == true if and only if T is an empty class or struct. 
    // is_empty may only be applied to complete types.
    //
    // is_empty cannot be used with union types until is_union can be made to work.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct empty_helper_t1 : public T{ char m[64]; };
    struct empty_helper_t2           { char m[64]; };

    template <typename T, bool is_a_class = false>
    struct empty_helper{ static const bool value = false; };

    template <typename T>
    struct empty_helper<T, true>{ static const bool value = sizeof(empty_helper_t1<T>) == sizeof(empty_helper_t2); };

    template <typename T>
    struct is_empty_value{
        typedef typename remove_cv<T>::type t;
        static const bool value = empty_helper<t, is_class<T>::value>::value;
    };

    template <> struct is_empty_value<void>{ static const bool value = false; };

    template <typename T> 
    struct is_empty : public integral_constant<bool, is_empty_value<T>::value>{};


    ///////////////////////////////////////////////////////////////////////
    // is_pod
    // 
    // is_pod<T>::value == true if and only if, for a given type T:
    //    - is_scalar<T>::value == true, or
    //    - T is a class or struct that has no user-defined copy 
    //      assignment operator or destructor, and T has no non-static 
    //      data members M for which is_pod<M>::value == false, and no 
    //      members of reference type, or
    //    - T is a class or struct that has no user-defined copy assignment 
    //      operator or destructor, and T has no non-static data members M for 
    //      which is_pod<M>::value == false, and no members of reference type, or
    //    - T is the type of an array of objects E for which is_pod<E>::value == true
    //
    // is_pod may only be applied to complete types.
    //
    // Without some help from the compiler or user, is_pod will not report 
    // that a struct or class is a POD, but will correctly report that 
    // built-in types such as int are PODs. The user can help the compiler
    // by using the EASTL_DECLARE_POD macro on a class.
    ///////////////////////////////////////////////////////////////////////
    template <typename T> // There's not much we can do here without some compiler extension.
    struct is_pod_value{ static const bool value = type_or< is_scalar<T>::value, is_void<T>::value >::value; };

    template <typename T, size_t N>
    struct is_pod_value<T[N]> : is_pod_value<T>{};

    template <>
    struct is_pod_value<void>{ static const bool value = true; };

    template <typename T>
    struct is_POD;

    template <typename T> 
    struct is_POD : public integral_constant<bool, is_pod_value<T>::value>{};

    template <typename T> 
    struct is_pod : public integral_constant<bool, is_POD<T>::value>{};

    #define EASTL_DECLARE_POD(T) namespace eastl{ template <> struct is_pod<T> : public true_type{}; template <> struct is_pod<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_constructor
    //
    // has_trivial_constructor<T>::value == true if and only if T is a class 
    // or struct that has a trivial constructor. A constructor is trivial if
    //    - it is implicitly defined by the compiler, and
    //    - is_polymorphic<T>::value == false, and
    //    - T has no virtual base classes, and
    //    - for every direct base class of T, has_trivial_constructor<B>::value == true, 
    //      where B is the type of the base class, and
    //    - for every nonstatic data member of T that has class type or array 
    //      of class type, has_trivial_constructor<M>::value == true, 
    //      where M is the type of the data member
    //
    // has_trivial_constructor may only be applied to complete types.
    //
    // Without from the compiler or user, has_trivial_constructor will not 
    // report that a class or struct has a trivial constructor. 
    // The user can use EASTL_DECLARE_TRIVIAL_CONSTRUCTOR to help the compiler.
    //
    // A default constructor for a class X is a constructor of class X that 
    // can be called without an argument.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct has_trivial_constructor_value{ static const bool value = is_pod<T>::value; }; // With current compilers, this is all we can do.

    template <typename T> 
    struct has_trivial_constructor : public integral_constant<bool, has_trivial_constructor_value<T>::value>{};

    #define EASTL_DECLARE_TRIVIAL_CONSTRUCTOR(T) namespace eastl{ template <> struct has_trivial_constructor<T> : public true_type{}; template <> struct has_trivial_constructor<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_copy
    //
    // has_trivial_copy<T>::value == true if and only if T is a class or 
    // struct that has a trivial copy constructor. A copy constructor is 
    // trivial if
    //   - it is implicitly defined by the compiler, and
    //   - is_polymorphic<T>::value == false, and
    //   - T has no virtual base classes, and
    //   - for every direct base class of T, has_trivial_copy<B>::value == true, 
    //     where B is the type of the base class, and
    //   - for every nonstatic data member of T that has class type or array 
    //     of class type, has_trivial_copy<M>::value == true, where M is the 
    //     type of the data member
    //
    // has_trivial_copy may only be applied to complete types.
    //
    // Another way of looking at this is:
    // A copy constructor for class X is trivial if it is implicitly 
    // declared and if all the following are true:
    //    - Class X has no virtual functions (10.3) and no virtual base classes (10.1).
    //    - Each direct base class of X has a trivial copy constructor.
    //    - For all the nonstatic data members of X that are of class type 
    //      (or array thereof), each such class type has a trivial copy constructor;
    //      otherwise the copy constructor is nontrivial.
    //
    // Without from the compiler or user, has_trivial_copy will not report 
    // that a class or struct has a trivial copy constructor. The user can 
    // use EASTL_DECLARE_TRIVIAL_COPY to help the compiler.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct has_trivial_copy_value{
        static const bool value = type_and<is_pod<T>::value, type_not<is_volatile<T>::value>::value>::value;
    };

    template <typename T> 
    struct has_trivial_copy : public integral_constant<bool, has_trivial_copy_value<T>::value>{};

    #define EASTL_DECLARE_TRIVIAL_COPY(T) namespace eastl{ template <> struct has_trivial_copy<T> : public true_type{}; template <> struct has_trivial_copy<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_assign
    //
    // has_trivial_assign<T>::value == true if and only if T is a class or 
    // struct that has a trivial copy assignment operator. A copy assignment 
    // operator is trivial if:
    //    - it is implicitly defined by the compiler, and
    //    - is_polymorphic<T>::value == false, and
    //    - T has no virtual base classes, and
    //    - for every direct base class of T, has_trivial_assign<B>::value == true, 
    //      where B is the type of the base class, and
    //    - for every nonstatic data member of T that has class type or array 
    //      of class type, has_trivial_assign<M>::value == true, where M is 
    //      the type of the data member.
    //
    // has_trivial_assign may only be applied to complete types.
    //
    // Without  from the compiler or user, has_trivial_assign will not 
    // report that a class or struct has trivial assignment. The user 
    // can use EASTL_DECLARE_TRIVIAL_ASSIGN to help the compiler.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct has_trivial_assign_value{
        static const bool value = type_and<is_pod<T>::value, type_not<is_const<T>::value>::value, type_not<is_volatile<T>::value>::value>::value;
    };

    template <typename T> 
    struct has_trivial_assign : public integral_constant<bool, has_trivial_assign_value<T>::value>{};

    #define EASTL_DECLARE_TRIVIAL_ASSIGN(T) namespace eastl{ template <> struct has_trivial_assign<T> : public true_type{}; template <> struct has_trivial_assign<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_destructor
    //
    // has_trivial_destructor<T>::value == true if and only if T is a class 
    // or struct that has a trivial destructor. A destructor is trivial if
    //    - it is implicitly defined by the compiler, and
    //    - for every direct base class of T, has_trivial_destructor<B>::value == true, 
    //      where B is the type of the base class, and
    //    - for every nonstatic data member of T that has class type or 
    //      array of class type, has_trivial_destructor<M>::value == true, 
    //      where M is the type of the data member
    //
    // has_trivial_destructor may only be applied to complete types.
    //
    // Without from the compiler or user, has_trivial_destructor will not 
    // report that a class or struct has a trivial destructor. 
    // The user can use EASTL_DECLARE_TRIVIAL_DESTRUCTOR to help the compiler.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct has_trivial_destructor_value{ static const bool value = is_pod<T>::value; }; // With current compilers, this is all we can do.

    template <typename T> 
    struct has_trivial_destructor : public integral_constant<bool, has_trivial_destructor_value<T>::value>{};

    #define EASTL_DECLARE_TRIVIAL_DESTRUCTOR(T) namespace eastl{ template <> struct has_trivial_destructor<T> : public true_type{}; template <> struct has_trivial_destructor<const T> : public true_type{}; }




    ///////////////////////////////////////////////////////////////////////
    // has_trivial_relocate
    //
    // This is an EA extension to the type traits standard.
    //
    // A trivially relocatable object is one that can be safely memmove'd 
    // to uninitialized memory. construction, assignment, and destruction 
    // properties are not addressed by this trait. A type that has the 
    // is_fundamental trait would always have the has_trivial_relocate trait. 
    // A type that has the has_trivial_constructor, has_trivial_copy or 
    // has_trivial_assign traits would usally have the has_trivial_relocate 
    // trait, but this is not strictly guaranteed.
    //
    // The point of has_trivial_relocate is that you can move an object around 
    // with nothing more than memcpy. A class might have a non-trivial constructor, 
    // destructor, operator=, but still be relocatable with memcpy. 
    //
    // The user can use EASTL_DECLARE_TRIVIAL_RELOCATE to help the compiler.
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct has_trivial_relocate_value{ static const bool value = type_and<is_pod<T>::value, type_not<is_volatile<T>::value>::value>::value; }; // With current compilers, this is all we can do.

    template <typename T> 
    struct has_trivial_relocate : public integral_constant<bool, has_trivial_relocate_value<T>::value>{};

    #define EASTL_DECLARE_TRIVIAL_RELOCATE(T) namespace eastl{ template <> struct has_trivial_relocate<T> : public true_type{}; template <> struct has_trivial_relocate<const T> : public true_type{}; }



    ///////////////////////////////////////////////////////////////////////
    // has_nothrow_constructor
    //
    // has_nothrow_constructor<T>::value == true if and only if T is a 
    // class or struct whose default constructor has an empty throw specification.
    // 
    // has_nothrow_constructor may only be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // has_nothrow_copy
    //
    // has_nothrow_copy<T>::value == true if and only if T is a class or 
    // struct whose copy constructor has an empty throw specification.
    //
    // has_nothrow_copy may only be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // has_nothrow_assign
    //
    // has_nothrow_assign<T>::value == true if and only if T is a class or 
    // struct whose copy assignment operator has an empty throw specification.
    // 
    // has_nothrow_assign may only be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // has_virtual_destructor
    //
    // has_virtual_destructor<T>::value == true if and only if T is a class 
    // or struct with a virtual destructor.
    //
    // has_virtual_destructor may only be applied to complete types.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



} // namespace eastl


#endif // Header include guard






















