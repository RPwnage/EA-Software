///////////////////////////////////////////////////////////////////////////////
// Unpacker.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef UNPACKER_H
#define UNPACKER_H

struct archive;

// This class unpacks the update zip file into the specified directory, usually
// the bundle directory.  It's almost a direct copy from the Windows bootstrapper.
class Unpacker
{
public:
	Unpacker();
	~Unpacker();

	bool Extract(const char* filename, const char* baseDirectory);
    
private:
	int CopyData(struct archive *ar, struct archive *aw);
	void Cleanup(struct archive *ar, struct archive *aw);
};

#endif //UNPACKER_H