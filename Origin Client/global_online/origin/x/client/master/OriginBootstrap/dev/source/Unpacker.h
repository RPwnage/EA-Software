///////////////////////////////////////////////////////////////////////////////
// Unpacker.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef UNPACKER_H
#define UNPACKER_H

#include <windows.h>

#define UNPACK_FAILED  WM_USER + 12
#define UNPACK_SUCCEEDED WM_USER + 13

class Unpacker
{
public:
	Unpacker();
	~Unpacker();

	bool Extract(const wchar_t* filename);
private:
	int CopyData(struct archive *ar, struct archive *aw);
	void Cleanup(struct archive *ar, struct archive *aw);
};

#endif //UNPACKER_H