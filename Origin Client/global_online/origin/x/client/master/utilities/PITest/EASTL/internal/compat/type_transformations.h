///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/type_transformations.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_TYPE_TRANFORMATIONS_H
#define EASTL_INTERNAL_TYPE_TRANFORMATIONS_H


namespace eastl
{


    // The following transformations are defined here. If the given item 
    // is missing then it simply hasn't been implemented, at least not yet.
    //    remove_const
    //    remove_volatile
    //    remove_cv
    //    add_const
    //    add_volatile
    //    add_cv
    //    remove_reference
    //    add_reference
    //    remove_extent
    //    remove_all_extents
    //    remove_pointer
    //    add_pointer
    //    aligned_storage


    ///////////////////////////////////////////////////////////////////////
    // add_signed
    //
    // Adds signed-ness to the given type. 
    // Modifies only integral values; has no effect on others.
    // add_signed<int>::type is int
    // add_signed<unsigned int>::type is int
    //
    ///////////////////////////////////////////////////////////////////////

    template<class T>
    struct add_signed
    { typedef T type; };

    template<>
    struct add_signed<unsigned char>
    { typedef signed char type; };

    #if (defined(CHAR_MAX) && defined(UCHAR_MAX) && (CHAR_MAX == UCHAR_MAX)) // If char is unsigned (which is usually not the case)...
        template<>
        struct add_signed<char>
        { typedef signed char type; };
    #endif

    template<>
    struct add_signed<unsigned short>
    { typedef short type; };

    template<>
    struct add_signed<unsigned int>
    { typedef int type; };

    template<>
    struct add_signed<unsigned long>
    { typedef long type; };

    template<>
    struct add_signed<unsigned long long>
    { typedef long long type; };

    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 4294967295U)) // If wchar_t is a 32 bit unsigned value...
            template<>
            struct add_signed<wchar_t>
            { typedef int32_t type; };
        #elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 65535)) // If wchar_t is a 16 bit unsigned value...
            template<>
            struct add_signed<wchar_t>
            { typedef int16_t type; };
        #endif
    #endif



    ///////////////////////////////////////////////////////////////////////
    // add_unsigned
    //
    // Adds unsigned-ness to the given type. 
    // Modifies only integral values; has no effect on others.
    // add_unsigned<int>::type is unsigned int
    // add_unsigned<unsigned int>::type is unsigned int
    //
    ///////////////////////////////////////////////////////////////////////

    template<class T>
    struct add_unsigned
    { typedef T type; };

    template<>
    struct add_unsigned<signed char>
    { typedef unsigned char type; };

    #if (defined(CHAR_MAX) && defined(SCHAR_MAX) && (CHAR_MAX == SCHAR_MAX)) // If char is signed (which is usually so)...
        template<>
        struct add_unsigned<char>
        { typedef unsigned char type; };
    #endif

    template<>
    struct add_unsigned<short>
    { typedef unsigned short type; };

    template<>
    struct add_unsigned<int>
    { typedef unsigned int type; };

    template<>
    struct add_unsigned<long>
    { typedef unsigned long type; };

    template<>
    struct add_unsigned<long long>
    { typedef unsigned long long type; };

    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 2147483647)) // If wchar_t is a 32 bit signed value...
            template<>
            struct add_unsigned<wchar_t>
            { typedef uint32_t type; };
        #elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 32767)) // If wchar_t is a 16 bit signed value...
            template<>
            struct add_unsigned<wchar_t>
            { typedef uint16_t type; };
        #endif
    #endif

    ///////////////////////////////////////////////////////////////////////
    // remove_const
    //
    // Remove const from a type.
    //
    // The remove_const transformation trait removes top-level const 
    // qualification (if any) from the type to which it is applied. For a 
    // given type T, remove_const<T const>::type is equivalent to the type T. 
    // For example, remove_const<char*>::type is equivalent to char* while 
    // remove_const<const char*>::type is equivalent to const char*. 
    // In the latter case, the const qualifier modifies char, not *, and is 
    // therefore not at the top level.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_volatile
    //
    // Remove volatile from a type.
    //
    // The remove_volatile transformation trait removes top-level volatile 
    // qualification (if any) from the type to which it is applied. 
    // For a given type T, the type remove_volatile <T volatile>::T is equivalent 
    // to the type T. For example, remove_volatile <char* volatile>::type is 
    // equivalent to char* while remove_volatile <volatile char*>::type is 
    // equivalent to volatile char*. In the latter case, the volatile qualifier 
    // modifies char, not *, and is therefore not at the top level.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_cv
    //
    // Remove const and volatile from a type.
    //
    // The remove_cv transformation trait removes top-level const and/or volatile 
    // qualification (if any) from the type to which it is applied. For a given type T, 
    // remove_cv<T const volatile>::type is equivalent to T. For example, 
    // remove_cv<char* volatile>::type is equivalent to char*, while remove_cv<const char*>::type 
    // is equivalent to const char*. In the latter case, the const qualifier modifies 
    // char, not *, and is therefore not at the top level.
    //
    ///////////////////////////////////////////////////////////////////////
    template <typename T> struct remove_cv_imp{};
    template <typename T> struct remove_cv_imp<T*>                { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<const T*>          { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<volatile T*>       { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<const volatile T*> { typedef T unqualified_type; };

    template <typename T> struct remove_cv{ typedef typename remove_cv_imp<T*>::unqualified_type type; };
    template <typename T> struct remove_cv<T&>{ typedef T& type; }; // References are automatically not const nor volatile. See section 8.3.2p1 of the C++ standard.

    template <typename T, size_t N> struct remove_cv<T const[N]>         { typedef T type[N]; };
    template <typename T, size_t N> struct remove_cv<T volatile[N]>      { typedef T type[N]; };
    template <typename T, size_t N> struct remove_cv<T const volatile[N]>{ typedef T type[N]; };



    ///////////////////////////////////////////////////////////////////////
    // add_const
    //
    // Add const to a type.
    //
    // Tor a given type T, add_const<T>::type is equivalent to T 
    // const if is_const<T>::value == false, and
    //    - is_void<T>::value == true, or
    //    - is_object<T>::value == true.
    //
    // Otherwise, add_const<T>::type is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // add_volatile
    //
    // Add volatile to a type.
    // 
    // For a given type T, add_volatile<T>::type is equivalent to T volatile 
    // if is_volatile<T>::value == false, and
    //   - is_void<T>::value == true, or
    //   - is_object<T>::value == true.
    //
    // Otherwise, add_volatile<T>::type is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // add_cv
    //
    // The add_cv transformation trait adds const and volatile qualification 
    // to the type to which it is applied. For a given type T, 
    // add_volatile<T>::type is equivalent to add_const<add_volatile<T>::type>::type.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // add_reference
    //
    // Add reference to a type.
    //
    // The add_reference transformation trait adds a level of indirection 
    // by reference to the type to which it is applied. For a given type T, 
    // add_reference<T>::type is equivalent to T& if is_reference<T>::value == false, 
    // and T otherwise.
    //
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct add_reference_impl{ typedef T& type; };

    template <typename T>
    struct add_reference_impl<T&>{ typedef T& type; };

    template <>
    struct add_reference_impl<void>{ typedef void type; };

    template <typename T>
    struct add_reference { typedef typename add_reference_impl<T>::type type; };



    ///////////////////////////////////////////////////////////////////////
    // remove_reference
    //
    // Remove reference from a type.
    //
    // The remove_reference transformation trait removes top-level of 
    // indirection by reference (if any) from the type to which it is applied. 
    // For a given type T, remove_reference<T&>::type is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_pointer
    //
    // Remove pointer from a type.
    //
    // The remove_pointer transformation trait removes top-level indirection 
    // by pointer (if any) from the type to which it is applied. Pointers to 
    // members are not affected. For a given type T, remove_pointer<T*>::type 
    // is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_extent
    //
    // The remove_extent transformation trait removes a dimension from an array.
    // For a given non-array type T, remove_extent<T>::type is equivalent to T.
    // For a given array type T[N], remove_extent<T[N]>::type is equivalent to T.
    // For a given array type const T[N], remove_extent<const T[N]>::type is equivalent to const T.
    // For example, given a multi-dimensional array type T[M][N], remove_extent<T[M][N]>::type is equivalent to T[N].
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_all_extents
    //
    // The remove_all_extents transformation trait removes all dimensions from an array.
    // For a given non-array type T, remove_all_extents<T>::type is equivalent to T.
    // For a given array type T[N], remove_all_extents<T[N]>::type is equivalent to T.
    // For a given array type const T[N], remove_all_extents<const T[N]>::type is equivalent to const T.
    // For example, given a multi-dimensional array type T[M][N], remove_all_extents<T[M][N]>::type is equivalent to T.
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // aligned_storage
    //
    // The aligned_storage transformation trait provides a type that is 
    // suitably aligned to store an object whose size is does not exceed length 
    // and whose alignment is a divisor of alignment. When using aligned_storage, 
    // length must be non-zero, and alignment must be equal to alignment_of<T>::value 
    // for some type T.
    //
    ///////////////////////////////////////////////////////////////////////

    // template <size_t length, size_t alignment> struct aligned_storage;
    // Not implemented yet.


} // namespace eastl


#endif // Header include guard





















