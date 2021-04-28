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
    template <typename T> struct is_void : public false_type{};

    template <> struct is_void<void>       : public true_type{};
    template <> struct is_void<const void> : public true_type{};



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
    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
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
    template <typename T>
    struct is_arithmetic_value{ static const bool value = (type_or<is_integral<T>::value, is_floating_point<T>::value>::value); };

    template <typename T> 
    struct is_arithmetic : public integral_constant<bool, is_arithmetic_value<T>::value>{};



    ///////////////////////////////////////////////////////////////////////
    // is_fundamental
    //
    // is_fundamental<T>::value == true if and only if:
    //    is_floating_point<T>::value == true, or
    //    is_integral<T>::value == true, or
    //    is_void<T>::value == true
    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_fundamental_value{ static const bool value = (type_or<is_void<T>::value, is_integral<T>::value, is_floating_point<T>::value>::value); };

    template <typename T> 
    struct is_fundamental : public integral_constant<bool, is_fundamental_value<T>::value>{};


} // namespace eastl


#endif // Header include guard





















