///////////////////////////////////////////////////////////////////////////////
// InstallSession.h
//
// Implements a class that hold session information for a game installation
//
// Created by Thomas Bruckschlegel
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef INSTALLSESSION_H
#define INSTALLSESSION_H

#include "EASTL/string.h"
#include "PersistentSession.h"

class InstallSession : public PersistentSession{
public:	
	InstallSession(const char8_t* productId) : PersistentSession(productId)
	{
	}

	~InstallSession() {};

};

#endif //INSTALLSESSION_H