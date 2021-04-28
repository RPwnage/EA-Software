///////////////////////////////////////////////////////////////////////////////
// Unpacker.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Unpacker.h"
#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

#import "LogServiceWrapper.h"

Unpacker::Unpacker() {}
Unpacker::~Unpacker() {}

// What's reasonable here
#define UNPACKER_PATH_MAX 2048

bool Unpacker::Extract(const char* filename, const char* baseDirectory)
{
	int flags;
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r;

	flags = ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;
	
    char pathname[UNPACKER_PATH_MAX+1];
    
	a = archive_read_new();
	archive_read_support_format_zip(a);
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	r = archive_read_open_filename(a, filename, 10240);
	if (r != ARCHIVE_OK)
	{
        ORIGIN_LOG_ERROR(@"Unpacker: Unable to open packed file %s", filename);
		Cleanup(a, ext);
		return false;
	}
    
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
	{
        // Change the pathname to be rooted within the bundle path.
        const char* relativePath = archive_entry_pathname( entry );
        snprintf( pathname, UNPACKER_PATH_MAX, "%s/%s", baseDirectory, relativePath );
        archive_entry_set_pathname( entry, pathname );
        
        // Change the permissions for this header entry to be writable (by all).
        mode_t perms = archive_entry_perm( entry ) | 00666; // read+write bit for user+group+others.
        archive_entry_set_perm( entry, perms );
        
        // Ensure any pre-existing files at this pathname are writable (by all).
//        chmod( pathname, 00777 );

		r = archive_write_header(ext, entry);
		if (r != ARCHIVE_OK)
		{
            ORIGIN_LOG_ERROR(@"Unpacker: Failed to write header");
			Cleanup(a, ext);
			return false;
		}
		else
		{
			if (CopyData(a, ext) != ARCHIVE_OK)
			{
                ORIGIN_LOG_ERROR(@"Unpacker: invalid file");
				// File is not valid
				Cleanup(a, ext);
				return false;
			}
			r = archive_write_finish_entry(ext);
			if (r != ARCHIVE_OK)
			{
                ORIGIN_LOG_ERROR(@"Unpacker: unable to finish write");
				Cleanup(a, ext);
				return false;
			}
		}
	}
	Cleanup(a, ext);
	return true;
}

void Unpacker::Cleanup(struct archive *ar, struct archive *aw)
{
	archive_read_close(ar);
	archive_read_finish(ar);
	archive_write_close(aw);
	archive_write_finish(aw);
}

int Unpacker::CopyData(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
	off_t offset;

	for (;;) 
	{
		r = archive_read_data_block(ar, &buff, &size, &offset);
		if (r == ARCHIVE_EOF)
			return (ARCHIVE_OK);
		if (r != ARCHIVE_OK)
		{
			return (r);
		}
		r = archive_write_data_block(aw, buff, size, offset);
		if (r != ARCHIVE_OK) 
		{
			return (r);
		}
	}
}