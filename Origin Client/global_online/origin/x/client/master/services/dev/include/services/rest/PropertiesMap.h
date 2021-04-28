///////////////////////////////////////////////////////////////////////////////
// PropertiesMap.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __PROPERTIES_MAP_H_INCLUDED__
#define __PROPERTIES_MAP_H_INCLUDED__

#include <QHash>
#include <QString>

#include "services/plugin/PluginAPI.h"

// helper class to get the static QHash
// properly constructed
template <typename T, typename U = quint64, typename V = QString>
class ORIGIN_PLUGIN_API PropertiesMap
{
protected:
	///
	/// Hash to store the array type values: id and name
	///
	static QHash<U, V> m_Hash;
public: 
	///
	/// Accessor: returns array type name given its id.
	///
	static const V name(U index)
	{
		if (exists(index))
			return m_Hash[index];
		else
			return V();
	}

	/// \brief Adds new element to map.
	///
	static void add(U index, const V& name) 
	{
		m_Hash[index] = name;
	}

    /// \brief Adds new element to map.
	///
	static void add(U index, const char* name) 
	{
        add(index, (const V&)QString::fromLatin1(name, -1).toUtf8().constData());
	}

    /// 
	/// \brief Tests if the passed key exists in the container.
	/// \param index to test
	///	\return true if it exists
	///
	static bool exists( const U& index)
	{
		return m_Hash.contains(index);
	}

	/// \brief Tests if the passed value exists in the container
	/// \param val The value to test.
	///	\return true if it exists
	///
	static bool valExists( const V& val)
	{
		return m_Hash.values().contains(val);
	}

	/// \brief Returns the first key U mapped to value V; if it does not exist, it returns a default-constructed key.
	/// \param val to search the key from
	/// \return Returns the key attached to value
	///
	static U keyForValue(const V& val)
	{
		return m_Hash.key(val);
	}

	/// \brief Returns the hash size.
	///
	static size_t size() { return m_Hash.size();}
};

#endif