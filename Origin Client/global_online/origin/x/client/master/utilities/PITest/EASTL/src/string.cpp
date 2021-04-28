///////////////////////////////////////////////////////////////////////////////
// EASTL/string.cpp
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////



#include <EASTL/internal/config.h>
#include <EASTL/string.h>
#include <EABase/eabase.h>


namespace eastl
{

    /// gEmptyString
    ///
    /// gEmptyString is used for empty strings. This allows us to avoid allocating
    /// memory for empty strings without having to add null pointer checks. 
    /// The downside of this technique is that all empty strings share this same
    /// value and if any code makes an error and writes to this value with non-zero,
    /// then all existing empty strings will be wrecked and not just the one that
    /// was incorrectly overwritten.
    EASTL_API EmptyString gEmptyString = { 0 };


} // namespace eastl














