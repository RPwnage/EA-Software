/*

	JSearchCache.h - Class to hold where I've searched for what, and whether it was sucessful or not

*/

#ifndef JSEARCHCACHE_H__
#define JSEARCHCACHE_H__

#include "ccList.h"

class JSearchNode;
class JPrefilteredFile;

class JSearchCache
{
public:
	JSearchCache() : List(4091), StoredFiles(0), ArgChecksum(0), pTokenCacheBuf(0) {;}
	void Reset(void);
	void Clean(void);

	bool FileExists(const char *pFilepath);
	JPrefilteredFile *OpenFile(const char *pFilepath);

	long Count(void) const {return List.GetNumElements();}
	long Stored(void) const {return StoredFiles;}

	void SortFiles();
	void ReadCachedTokens(const char *pFilename, ccList &ArgList);
	void WriteCachedTokens(FILE *f);

	void WriteFileChecksums(FILE *f);

private:
	JSearchNode *GetFileNode(const char *pFilepath);
	long StoredFiles;

	unsigned int ArgChecksum;
	void *pTokenCacheBuf;

	ccHashList	List;
};

#endif
