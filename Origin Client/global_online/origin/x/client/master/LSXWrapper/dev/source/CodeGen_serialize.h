/////////////////////////////////////////////////////////////////////////////
// CodeGen_Serialize.h
//
// Copyright (c) 2010, Electronic Arts Inc. All rights reserved.
//
// Serialization templates for codegen. Include in sourcelist before 
// any codegen files.
//
// Created by Hans van Veenendaal
/////////////////////////////////////////////////////////////////////////////

#ifndef __CODEGEN_SERIALIZE_H__
#define __CODEGEN_SERIALIZE_H__

template<typename T>
std::string Serialize(const T& obj)
{
	return obj.Serialize();
}


template<typename T>
std::string SerializeServerObject(const T& obj)
{
	size_t size = 65535;
	std::string buf;
	buf.resize(size);
	if( !Writer(const_cast<T&>(obj), &buf[0], size) )
	{
		size++;	// need to increment size, since Writer needs size+1 to write the string and a null
		buf.resize(size);
		Writer(const_cast<T&>(obj), &buf[0], size);
	}
	buf.resize(size);
	return buf;
}

#endif