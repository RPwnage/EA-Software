// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

/*!
 *  @def EADPSDK_USE_STD_STL
 *  @brief Used to switch between EASTL and std STL. Defaults to using EASTL.
 */
#ifndef EADPSDK_USE_STD_STL
#define EADPSDK_USE_STD_STL 0
#endif

/*!
 * @def EADPSDK_USE_DEFAULT_ALLOCATOR
 * @brief Used if a default allocator should be implemented
 */
#ifndef EADPSDK_USE_DEFAULT_ALLOCATOR
#define EADPSDK_USE_DEFAULT_ALLOCATOR 0
#endif 

/*!
 * @def EADPSDK_INIT_SEMD
 * @brief Used to create/destroy a default SystemEventMessageDispatcher instance. 
 */
#ifndef EADPSDK_INIT_SEMD
#define EADPSDK_INIT_SEMD 0
#endif

#include <EABase/eabase.h>
#include <coreallocator/icoreallocator_interface.h>

#if EADPSDK_USE_STD_STL
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>
#include <set>
#include <list>
#else
#include <EASTL/core_allocator_adapter.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/string.h>
#include <EASTL/map.h>
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <EASTL/deque.h>
#include <EASTL/set.h>

EA_DISABLE_VC_WARNING(4625 4626)
// suppress the warnings caused by list<UniquePtr> copy constructor
#include <EASTL/list.h>
EA_RESTORE_VC_WARNING()

#include <EASTL/utility.h>
#include <IEAUser/IEAUser.h>
#endif

#include <EAStdC/EADateTime.h>

/*!
 * @def EADPSDK_API
 * @brief This is used to label functions as DLL exports.
 */
#ifndef EADPSDK_API
#if defined(EA_DLL) // If the build file hasn't already defined this to be dllexport...
#if defined(EA_PLATFORM_MICROSOFT)
#define EADPSDK_API      __declspec(dllimport)
#else
 // rely on the dll building project to define dllexport
#define EADPSDK_API
#endif
#else
#define EADPSDK_API
#endif
#endif

 /*!
  * @def EADPGENERATED_API
  * @brief This is used to label functions as DLL exports.
  */
#ifndef EADPGENERATED_API
#if defined(EA_DLL) // If the build file hasn't already defined this to be dllexport...
#if defined(EA_PLATFORM_MICROSOFT)
#define EADPGENERATED_API      __declspec(dllimport)
#else
  // rely on the dll building project to define dllexport
#define EADPGENERATED_API
#endif
#else
#define EADPGENERATED_API
#endif
#endif

#if EADPSDK_USE_STD_STL
namespace eadp
{
namespace foundation
{
    /*!
     *  @brief Unique pointer type for the Foundation module
     */
    template <typename T> using UniquePtr = std::unique_ptr<T>;
}
}
#else
#include <eadp/foundation/UniquePtr.h>
#endif

namespace eadp
{
/*!
 * @brief Root namespace for the Foundation package
 */
namespace foundation
{

/*!
 *  @brief Allocated String type for the Foundation module
 */
#if EADPSDK_USE_STD_STL
    using String = std::string;
#else
    using String = eastl::basic_string<char8_t, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
    /*!
 *  @brief Allocated 16-bit String type for the Foundation module
 */
#if EADPSDK_USE_STD_STL
    using String16 = std::basic_string<char16_t>;
#else
    using String16 = eastl::basic_string<char16_t, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
    /*!
 *  @brief Allocated wchar String type for the Foundation module
 */
#if EADPSDK_USE_STD_STL
    using WString = std::wstring;
#else
    using WString = eastl::basic_string<wchar_t, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
    /*!
 *  @brief Time and date type for the Foundation module
 */
using Timestamp = EA::StdC::DateTime;

/*!
 *  @brief Shared pointer type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename T> using SharedPtr = std::shared_ptr<T>;
#else
template <typename T> using SharedPtr = eastl::shared_ptr<T>;
#endif
/*!
 *  @brief Weak pointer type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename T> using WeakPtr = std::weak_ptr<T>;
#else
template <typename T> using WeakPtr = eastl::weak_ptr<T>;
#endif
/*!
 *  @brief Vector container type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename T> using Vector = std::vector<T>;
#else
template <typename T> using Vector = eastl::vector<T, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
/*!
 *  @brief Tree-based container type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename K, typename V> using Map = std::map<K, V>;
#else
template <typename K, typename V> using Map = eastl::map<K, V, eastl::less<K>, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
/*!
 *  @brief Tree-based container type with redundant key support for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename K, typename V> using MultiMap = std::multimap<K, V>;
#else
template <typename K, typename V> using MultiMap = eastl::multimap<K, V, eastl::less<K>, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
/*!
 *  @brief Hashed container type with redundant key support for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename K>
using HashStringMap = std::unordered_map<String, K>;
#else
template <typename K>
using HashStringMap = eastl::hash_map<String, K, eastl::string_hash<String>, eastl::equal_to<String>, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
/*!
 *  @brief Queue container type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename T> using Queue = std::deque<T>;
#else
template <typename T> using Queue = eastl::deque<T, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
/*!
 *  @brief Unordered set container type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename T> using Set = std::set<T>;
#else
template <typename T> using Set = eastl::set<T, eastl::less<T>, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
/*!
 *  @def EADPSDK_MOVE
 *  @brief Abstracts STL provider for move semantics.
 */
#if EADPSDK_USE_STD_STL
#define EADPSDK_STL std
#define EADPSDK_SHARED_FROM_THIS std::shared_from_this
#define EADPSDK_ENABLE_SHARED_FROM_THIS std::enable_shared_from_this
#define EADPSDK_MOVE std::move
#define EADPSDK_SWAP std::swap
#define EADPSDK_FORWARD std::forward
#define EADPSDK_INDEX_SEQUENCE_FOR std::index_sequence_for
#define EADPSDK_SIZE_T std::size_t
#else
#define EADPSDK_STL eastl
#define EADPSDK_SHARED_FROM_THIS eastl::shared_from_this
#define EADPSDK_ENABLE_SHARED_FROM_THIS eastl::enable_shared_from_this
#define EADPSDK_MOVE eastl::move
#define EADPSDK_SWAP eastl::swap
#define EADPSDK_FORWARD eastl::forward
#define EADPSDK_INDEX_SEQUENCE_FOR eastl::index_sequence_for
#define EADPSDK_SIZE_T EASTL_SIZE_T
#endif
/*!
 *  @brief Hash map container type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename K, typename V> using HashMap = std::unordered_map<K, V>;
#else
template <typename K, typename V>
using HashMap = eastl::hash_map<K, V, eastl::hash<K>, eastl::equal_to<K>, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif
/*!
 *  @brief List container type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
template <typename T> using List = std::list<T>;
#else
template <typename T>
using List = eastl::list<T, EA::Allocator::EASTLICoreAllocatorAdapter>;
#endif

/*!
 *  @brief User type for the Foundation module.
 */
#if EADPSDK_USE_STD_STL
using User = void;
#else
using User = EA::User::IEAUser;
#endif

/*!
 *  @brief Package name for the module
 */
EADPSDK_API extern const char8_t* kPackageName;

}
}


