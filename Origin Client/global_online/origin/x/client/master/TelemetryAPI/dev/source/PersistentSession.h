///////////////////////////////////////////////////////////////////////////////
// PersistentSession.h
//
// Implements a class that hold session information
// TODO: store & load is currently not needed
// Created by Thomas Bruckschlegel
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef PERSISTENTSESSION_H
#define PERSISTENTSESSION_H

#include "EASTL/string.h"

class PersistentSession
{
public:
	PersistentSession( const char8_t* productId );
	~PersistentSession();
	uint64_t startTimeUTC() const;
	uint32_t sessionLength() const;
	uint64_t sessionId() const;
	const char8_t *productId() const;

private:
	uint64_t m_numericSessionId;
	uint32_t m_startTimeUTC;
	eastl::string8 m_productId;
};

#endif // PERSISTENTSESSION_H