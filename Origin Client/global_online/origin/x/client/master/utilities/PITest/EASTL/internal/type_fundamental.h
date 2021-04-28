///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/type_fundamental.h
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_TYPE_FUNDAMENTAL_H
#define EASTL_INTERNAL_TYPE_FUNDAMENTAL_H


namespace eastl
{

    // The following properties or relations are defined here. If the given 
    // item is missing then it simply hasn't been implemented, at least not yet.
    //   is_void
    //   is_integral
    //   is_floating_point
    //   is_arithmetic
    //   is_fundamental


    ///////////////////////////////////////////////////////////////////////
    // is_void
    //
    // is_void<T>::value == true if and only if T is one of the following types:
    //    [const][volatile] void
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_is_void_CONFORMANCE 1    // is_void is conforming.

    template <typename T> struct is_void : public false_type{};

    template <> struct is_void<void> : public true_type{};
    template <> struct is_void<void const> : public true_type{};
    template <> struct is_void<void volatile> : public true_type{};
    template <> struct is_void<void const volatile> : public true_type{};



    ///////////////////////////////////////////////////////////////////////
    // is_integral
    //
    // is_integral<T>::value == true if and only if T  is one of the following types:
    //    [const] [volatile] bool
    //    [const] [volatile] char
    //    [const] [volatile] signed char
    //    [const] [volatile] unsigned char
    //    [const] [volatile] wchar_t
    //    [const] [volatile] short
    //    [const] [volatile] int
    //    [const] [volatile] long
    //    [const] [volatile] long long
    //    [const] [volatile] unsigned short
    //    [const] [volatile] unsigned int
    //    [const] [volatile] unsigned long
    //    [const] [volatile] unsigned long long
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_is_integral_CONFORMANCE 1    // is_integral is conforming.

    template <typename T> struct is_integral : public false_type{};

    // To do: Need to define volatile and const volatile versions of these.
    template <> struct is_integral<unsigned char>            : public true_type{};
    template <> struct is_integral<const unsigned char>      : public true_type{};
    template <> struct is_integral<unsigned short>           : public true_type{};
    template <> struct is_integral<const unsigned short>     : public true_type{};
    template <> struct is_integral<unsigned int>             : public true_type{};
    template <> struct is_integral<const unsigned int>       : public true_type{};
    template <> struct is_integral<unsigned long>            : public true_type{};
    template <> struct is_integral<const unsigned long>      : public true_type{};
    template <> struct is_integral<unsigned long long>       : public true_type{};
    template <> struct is_integral<const unsigned long long> : public true_type{};

    template <> struct is_integral<signed char>              : public true_type{};
    template <> struct is_integral<const signed char>        : public true_type{};
    template <> struct is_integral<signed short>             : public true_type{};
    template <> struct is_integral<const signed short>       : public true_type{};
    template <> struct is_integral<signed int>               : public true_type{};
    template <> struct is_integral<const signed int>         : public true_type{};
    template <> struct is_integral<signed long>              : public true_type{};
    template <> struct is_integral<const signed long>        : public true_type{};
    template <> struct is_integral<signed long long>         : public true_type{};
    template <> struct is_integral<const signed long long>   : public true_type{};

    template <> struct is_integral<bool>            : public true_type{};
    template <> struct is_integral<const bool>      : public true_type{};
    template <> struct is_integral<char>            : public true_type{};
    template <> struct is_integral<const char>      : public true_type{};
    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type which is already handled above...
        template <> struct is_integral<wchar_t>         : public true_type{};
        template <> struct is_integral<const wchar_t>   : public true_type{};
    #endif



    ///////////////////////////////////////////////////////////////////////
    // is_floating_point
    //
    // is_floating_point<T>::value == true if and only if T is one of the following types:
    //    [const] [volatile] float
    //    [const] [volatile] double
    //    [const] [volatile] long double
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_is_floating_point_CONFORMANCE 1    // is_floating_point is conforming.

    template <typename T> struct is_floating_point : public false_type{};

    // To do: Need to define volatile and const volatile versions of these.
    template <> struct is_floating_point<float>             : public true_type{};
    template <> struct is_floating_point<const float>       : public true_type{};
    template <> struct is_floating_point<double>            : public true_type{};
    template <> struct is_floating_point<const double>      : public true_type{};
    template <> struct is_floating_point<long double>       : public true_type{};
    template <> struct is_floating_point<const long double> : public true_type{};



    ///////////////////////////////////////////////////////////////////////
    // is_arithmetic
    //
    // is_arithmetic<T>::value == true if and only if:
    //    is_floating_point<T>::value == true, or
    //    is_integral<T>::value == true
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_is_arithmetic_CONFORMANCE 1    // is_arithmetic is conforming.

    template <typename T> 
    struct is_arithmetic : public integral_constant<bool,
        is_integral<T>::value || is_floating_point<T>::value
    >{};



    ///////////////////////////////////////////////////////////////////////
    // is_fundamental
    //
    // is_fundamental<T>::value == true if and only if:
    //    is_floating_point<T>::value == true, or
    //    is_integral<T>::value == true, or
    //    is_void<T>::value == true
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_is_fundamental_CONFORMANCE 1    // is_fundamental is conforming.

    template <typename T> 
    struct is_fundamental : public integral_constant<bool,
        is_void<T>::value || is_integral<T>::value || is_floating_point<T>::value
    >{};


} // namespace eastl


#endif // Header include guard





















