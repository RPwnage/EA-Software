// ***************************************************************
//  CCollectCache.h           version: 1.0.0  ·  date: 2010-01-10
//  -------------------------------------------------------------
//  cache class to optimize hooking performance
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

#ifndef _CCOLLECTCACHE_H
#define _CCOLLECTCACHE_H

//#ifndef _CCOLLECTION_H
  #include "CCollection.h"
//#endif
#include <EASTL/hash_map.h>

typedef struct tagCollectCacheRecord
{
  DWORD ThreadId;
  int ReferenceCount;

  // in RAM a lot of patching can happen
  // sometimes madCodeHook has to make sure that it can get original unpatched information
  // for such purposes it opens the original exe/dll files on harddisk
  // in order to speed things up, we're caching the last opened file here
  HMODULE moduleOrg;   // which loaded exe/dll module is currently open?
  LPVOID moduleFile;   // where is the file mapped into memory?
} COLLECT_CACHE_RECORD;


#define COLLECT_RELOCATION_CACHE_RECORD_BUFFER_SIZE 32
typedef struct tagCollectRelocationCacheRecord
{
    CHAR buffer[COLLECT_RELOCATION_CACHE_RECORD_BUFFER_SIZE];
} COLLECT_RELOCATION_CACHE_RECORD;


class CCollectCache
{
  private:
    static CCollectCache *pInstance;

  public:
    static void AddReference(void);
    static void ReleaseReference(void);
    static void Initialize(void);
    static void Finalize(void);
    static LPVOID CacheModuleFileMap(HMODULE hModule);

  protected:
    CCollectCache();
    CCollectCache(const CCollectCache&);
    CCollectCache& operator = (const CCollectCache&);
    ~CCollectCache();

    static CCollectCache& Instance();

    int Open(HANDLE *hMutex, BOOL force);
    void Close(HANDLE hMutex);

    void AddReferenceInternal(void);
    void ReleaseReferenceInternal(void);

    LPVOID CacheModuleFileMapInternal(HMODULE hModule);

    // Add a record to the collection for given ThreadId
    int AddThread(DWORD threadId);

    // Collection of structures
    CCollection<COLLECT_CACHE_RECORD, CStructureEqualHelper<COLLECT_CACHE_RECORD>> mCollectCacheRecords;
};


// a simple cache structure for caching module files

class CCollectCache2
{
private:
    static CCollectCache2 *pInstance;
    CRITICAL_SECTION mCS;
public:
    static void Initialize(void);
    static void Finalize(void);
    static LPVOID CacheModuleFileMap(HMODULE hModule);
    
    static CHAR* CacheRelocationBuffer(LPVOID pCode);
    static void AddCacheRelocationBuffer(LPVOID pCode, CHAR relocationBuffer[COLLECT_RELOCATION_CACHE_RECORD_BUFFER_SIZE]);

    static MEMORY_BASIC_INFORMATION* CacheMemoryQueryBuffer(LPVOID pCode);
    static void AddCacheMemoryQueryBuffer(LPVOID pCode, MEMORY_BASIC_INFORMATION *mbi);

protected:
    CCollectCache2();
    CCollectCache2(const CCollectCache2&);
    CCollectCache2& operator = (const CCollectCache2&);
    ~CCollectCache2();

    static CCollectCache2& Instance();

    int Find(HMODULE module);

    LPVOID CacheModuleFileMapInternal(HMODULE hModule);

    CHAR* CacheRelocationBufferInternal(LPVOID pCode);
    void AddCacheRelocationBufferInternal(LPVOID pCode, CHAR relocationBuffer[COLLECT_RELOCATION_CACHE_RECORD_BUFFER_SIZE]);

    MEMORY_BASIC_INFORMATION* CacheMemoryQueryBufferInternal(LPVOID pCode);
    void AddCacheMemoryQueryBufferInternal(LPVOID pCode, MEMORY_BASIC_INFORMATION *mbi);

    // Collection of structures
    CCollection<COLLECT_CACHE_RECORD, CStructureEqualHelper<COLLECT_CACHE_RECORD>> mCollectCacheRecords;

    // Collection of structures

    eastl::hash_map<LPVOID, COLLECT_RELOCATION_CACHE_RECORD> mCollectRelocationCacheRecords;
    eastl::hash_map<LPVOID, MEMORY_BASIC_INFORMATION> mCollectMemoryQuery;
};

#endif