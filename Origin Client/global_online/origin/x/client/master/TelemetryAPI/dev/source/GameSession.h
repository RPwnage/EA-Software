///////////////////////////////////////////////////////////////////////////////
// GameSession.h
//
// Implements a class that hold session information for a game session
//
// Created by Thomas Bruckschlegel
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef GAMESESSION_H
#define GAMESESSION_H

#include "PersistentSession.h"

class GameSession : public PersistentSession{
public:	
	GameSession(const char8_t* productId) : PersistentSession(productId)
	{
	}

	~GameSession() {};
};

#endif // GAMESESSION_H