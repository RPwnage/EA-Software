///////////////////////////////////////////////////////////////////////////////
// Unpacker.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Unpacker.h"
#include "Common.h"
#include "LogService_win.h"

#include "libarchive\archive.h"
#include "libarchive\archive_entry.h"

Unpacker::Unpacker() {}
Unpacker::~Unpacker() {}

bool Unpacker::Extract(const wchar_t* filename)
{
	int flags;
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r;

	flags = ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;

	char mbFileName[MAX_PATH];
	size_t convertedChars = 0;
	wcstombs_s(&convertedChars, mbFileName, sizeof(mbFileName), filename, _TRUNCATE);
	mbFileName[convertedChars] = 0;

	// If we're not on XP then we don't want to extract winhttp.dll
	bool isXP = false;
	OSVERSIONINFO	osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osvi))
	{
		if (osvi.dwMajorVersion == 5 && (osvi.dwMinorVersion == 1 || osvi.dwMinorVersion == 2))
		{
			isXP = true;
		}
	}

	a = archive_read_new();
	archive_read_support_format_zip(a);
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	r = archive_read_open_filename(a, mbFileName, 10240);
	if (r != ARCHIVE_OK)
	{
        ORIGINBS_LOG_ERROR << "Unpacker: Unable to open packed file";
		Cleanup(a, ext);
		PostMessage(hUnpackDialog, UNPACK_FAILED, 0, 0);
		return false;
	}
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
	{
		// If this is Origin, rename it since it will fail updating
		if (strncmp(archive_entry_pathname(entry), "Origin.exe", 10) == 0)
		{
			archive_entry_set_pathname(entry, "OriginTMP.exe");
		}

		if (strncmp(archive_entry_pathname(entry), "winhttp.dll", 9) == 0 && !isXP)
		{
			continue;
		}

		// Remove the file attributes from the target files since they could be read only (And they're not supposed to be)
		SetFileAttributesA(archive_entry_pathname(entry), FILE_ATTRIBUTE_NORMAL);

		r = archive_write_header(ext, entry);
		if (r != ARCHIVE_OK)
		{
			if (strncmp(archive_entry_pathname(entry), "winhttp.dll", 9) == 0)
			{
				continue;
			}
            ORIGINBS_LOG_ERROR << "Unpacker: unable to write header";
			Cleanup(a, ext);
			PostMessage(hUnpackDialog, UNPACK_FAILED, 0, 0);
			return false;
		}
		else
		{
			if (CopyData(a, ext) != ARCHIVE_OK)
			{
				// File is not valid
                ORIGINBS_LOG_ERROR << "Unpacker: invalid file";
				Cleanup(a, ext);
				PostMessage(hUnpackDialog, UNPACK_FAILED, 0, 0);
				return false;
			}
			r = archive_write_finish_entry(ext);
			if (r != ARCHIVE_OK)
			{
                ORIGINBS_LOG_ERROR << "Unpacker: unable to finish write";
				Cleanup(a, ext);
				PostMessage(hUnpackDialog, UNPACK_FAILED, 0, 0);
				return false;
			}
		}
	}
	Cleanup(a, ext);
	PostMessage(hUnpackDialog, UNPACK_SUCCEEDED, 0, 0);
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
	const void *buff;
	size_t size;
	off_t offset;

	for (;;) 
	{
		int r = archive_read_data_block(ar, &buff, &size, &offset);
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