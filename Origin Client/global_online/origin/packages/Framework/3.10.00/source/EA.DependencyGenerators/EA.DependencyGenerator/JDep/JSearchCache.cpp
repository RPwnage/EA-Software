/*

	JSearchCache.cpp - Class to hold where I've searched for what, and whether it was sucessful or not

*/

#include "StdAfx.h"
#include <Windows.h>

#include "JSearchCache.h"
#include "JPrefilteredFile.h"
#include "bWareHooks.hpp"

int PrintTiming = 0;

#define BCHUNK_JDEP_PREFILTERED_FILE	0x00133333	// This # is arbitrary
#define BCHUNK_JDEP_ARG_CHECKSUM		0x00133334

// This structure is written to the TokenCache for each file
struct CachedTokenHeader
{
	int		FileTime;
	int		FileChecksum;
	int		UnusedCount;	// Reset to 0 if used.  Incremented by 1 every load
	int		NumTokens;
	char	Filename[160];
};

class JSearchNode : public ccHashNode
{
public:
	JSearchNode() : bFound(false), Checksum(0), FileTime(0), UnusedCount(0), pCachedTokenHeader(0), CachedTokenSize(0), pFilt(0) {;}	// Set the usage high to delay purging for a while
	~JSearchNode() {if(pFilt) delete pFilt;}

	static BOOL AlphabetSortFunction(ccMinNode *before, ccMinNode *after) {return (_stricmp(((JSearchNode *)before)->GetName(), ((JSearchNode *)after)->GetName()) >= 0);}

	bool bFound;
	int	 Checksum;
	int  FileTime;
	int  UnusedCount;
	CachedTokenHeader *pCachedTokenHeader;
	int CachedTokenSize;
	JPrefilteredFile *pFilt;
};


// Keeping track of filenames that needed reloading.
int ChangedFilenameTableLength = 0;
char ChangedFilenameTable[65536] = {0};


void JSearchCache::Reset(void)
{
	List.Purge();
}

JSearchNode *JSearchCache::GetFileNode(const char *pFilepath)
{
	JSearchNode *pNode = (JSearchNode *)List.Find(pFilepath);
	if(pNode == 0)
	{
		FILE *pFile = fopen(pFilepath, "rb");

		pNode = new JSearchNode;
		pNode->SetName(pFilepath);
		pNode->bFound = pFile != 0;

		if(pFile)
		{
			char *pBuf;
			long Len;

			fseek(pFile, 0, SEEK_END);
			Len = ftell(pFile);
			fseek(pFile, 0, SEEK_SET);
			pBuf = new char[Len + 9];		// Allow for MMX optimizations on the buffer
			fread(pBuf, 1, Len, pFile);
			fclose(pFile);
			//memset(pBuf + Len, 0, 9);		// Fill the padding with 0's to stop any out-of-bounds accesses
			pBuf[Len] = 0;

			pNode->Checksum = bCalculateCrc32(pBuf, Len);
			pNode->FileTime = FileTime(pFilepath);

			StoredFiles++;

			pNode->pFilt = new JPrefilteredFile();
			pNode->pFilt->Open(pBuf, Len);
			delete [] pBuf;

			if (ChangedFilenameTableLength < (int)sizeof(ChangedFilenameTable) - 512)
			{
				strcpy(&ChangedFilenameTable[ChangedFilenameTableLength], pFilepath);
				ChangedFilenameTableLength += (int)strlen(pFilepath) + 1;
			}
		}

		List.AddTail(pNode);
	}
	else if (pNode && pNode->pCachedTokenHeader && !pNode->pFilt)
	{
		StoredFiles++;

		pNode->pFilt = new JPrefilteredFile();

		pNode->pFilt->ReadCachedTokens((char *)(pNode->pCachedTokenHeader + 1), pNode->pCachedTokenHeader->NumTokens);
	}

	pNode->UnusedCount = 0;	// Mark as having been used.

	return pNode;
}

bool JSearchCache::FileExists(const char *pFilepath)
{
	JSearchNode *pNode = GetFileNode(pFilepath);
	return pNode->bFound;
}

JPrefilteredFile *JSearchCache::OpenFile(const char *pFilepath)
{
	JSearchNode *pNode = GetFileNode(pFilepath);
	return pNode->pFilt;
}

void JSearchCache::SortFiles()
{
	ccList list;

	JSearchNode *pNode = (JSearchNode *)List.GetHead();
	while (pNode)
	{
		JSearchNode *pNextNode = (JSearchNode *)pNode->GetNext();

		if (pNode->pFilt)
		{
			List.RemNode(pNode);
			list.AddTail(pNode);
		}

		pNode = pNextNode;
	}

	list.Sort(JSearchNode::AlphabetSortFunction);

	while (!list.IsListEmpty())
	{
		pNode = (JSearchNode *)list.RemHead();
		List.AddTail(pNode);
	}
}

void JSearchCache::ReadCachedTokens(const char *pFilename, ccList &ArgList)
{
	bInitTicker(10.0f);
	int ticks = bGetTicker();

	// Calculate arg_checksum
	ArgChecksum = 0;
	for (ccNode *pNode = ArgList.GetHead(); pNode; pNode = pNode->GetNext())
		ArgChecksum += pNode->GetHash();

	// Does file exist?
	FILE *f = fopen(pFilename, "rb");
	if (!f)
		return;
	fclose(f);

	int file_size;
	int num_read = 0;
	pTokenCacheBuf = ReadEntireFile(pFilename, &file_size);
	bChunk *chunks = (bChunk *)pTokenCacheBuf;
	for (bChunk *chunk = chunks; chunk < GetLastChunk(chunks, file_size); chunk = chunk->GetNext())
	{
		// We expect only these chunks
		// This one will be first.
		if (chunk->GetID() == BCHUNK_JDEP_ARG_CHECKSUM)
		{
			unsigned int cached_checksum = *(unsigned int *)chunk->GetData();
			if (cached_checksum != ArgChecksum)
			{
				//printf("Arg checksum has changed.\n");
				break;
			}
		}
		else if (chunk->GetID() == BCHUNK_JDEP_PREFILTERED_FILE)
		{
			CachedTokenHeader *header = (CachedTokenHeader *)chunk->GetData();

			// We need to skip loading files from the cache if the FileTime
			// has changed.  Obviously this is the key feature that makes it work.
			// FileTime() will return 0 if file not found.
			if (header->FileTime && (header->FileTime == FileTime(header->Filename)) )
			{
				int token_size = chunk->GetSize() - sizeof(*header);

				JSearchNode *pNode = new JSearchNode;
				pNode->SetName(header->Filename);
				pNode->bFound = true;
				pNode->Checksum = header->FileChecksum;
				pNode->FileTime = header->FileTime;
				pNode->UnusedCount = header->UnusedCount + 1;	// Increment the unused count
				pNode->CachedTokenSize = token_size;
				pNode->pCachedTokenHeader = header;
				pNode->pFilt = NULL;

				List.AddTail(pNode);
			}
			else
			{
				//printf("File %s has changed....\n", header->Filename);
			}

			num_read++;
		}
	}

	if (PrintTiming)
		printf("Timings:  ReadCachedTokens = %.3fs\n", bGetTickerDifference(ticks) / 1000.0f);
}

void JSearchCache::WriteCachedTokens(FILE *f)
{
	int ticks = bGetTicker();

	int magic = WriteChunkHeader(f, BCHUNK_JDEP_ARG_CHECKSUM);
	fwrite(&ArgChecksum, 1, sizeof(ArgChecksum), f);
	WriteChunkSize(f, magic);

	SortFiles();

	int num_written = 0;
	for (JSearchNode *pNode = (JSearchNode *)List.GetHead(); pNode; pNode = (JSearchNode *)pNode->GetNext())
	{
		// If our cached tokens haven't changed we can save time by writing them out in one big chunk
		if (pNode->pCachedTokenHeader && (pNode->UnusedCount == 0))
		{
			int magic = WriteChunkHeader(f, BCHUNK_JDEP_PREFILTERED_FILE);

			fwrite(pNode->pCachedTokenHeader, 1, sizeof(CachedTokenHeader) + pNode->CachedTokenSize, f);

			WriteChunkSize(f, magic);

			num_written++;
		}
		else if (pNode->pFilt)
		{
			CachedTokenHeader header;
			memset(&header, 0, sizeof(header));
			strncpy(header.Filename, pNode->GetName(), sizeof(header.Filename) - 1);
			header.FileChecksum = pNode->Checksum;
			header.FileTime = pNode->FileTime;
			header.NumTokens = pNode->pFilt->GetNumTokens();

			int magic = WriteChunkHeader(f, BCHUNK_JDEP_PREFILTERED_FILE);

			fwrite(&header, 1, sizeof(header), f);

			pNode->pFilt->WriteCachedTokens(f);

			WriteChunkSize(f, magic);

			num_written++;
		}
	}

	delete[] pTokenCacheBuf;
	pTokenCacheBuf = 0;

	if (PrintTiming)
		printf("Timings:  WriteCachedTokens = %.3fs\n", bGetTickerDifference(ticks) / 1000.0f);
}

void JSearchCache::WriteFileChecksums(FILE *f)
{
	fprintf(f, "\n\n//\n// File Checksums\n//\n\n");

	SortFiles();

	for (JSearchNode *pNode = (JSearchNode *)List.GetHead(); pNode; pNode = (JSearchNode *)pNode->GetNext())
	{
		// We only write out nodes that were used.
		if (pNode->pFilt && (pNode->UnusedCount == 0))
		{
			fprintf(f, "FILE:  0x%08x  %s\n", pNode->Checksum, pNode->GetName());
		}
	}
}
