///////////////////////////////////////////////////////////////////////////////
// Registration.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef REGISTRATION_H
#define REGISTRATION_H

namespace Registration
{
	bool setAutoStartEnabled(bool autostart);
	bool registerUrlProtocol();
	bool registerUrlProtocol(wchar_t* protocol_name, unsigned long protocol_size, wchar_t* keyName);
};

#endif