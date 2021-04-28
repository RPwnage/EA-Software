///////////////////////////////////////////////////////////////////////////////
// HELPERS_H.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef HELPERS_H
#define HELPERS_H

#ifdef ORIGIN_PC

#include <windows.h>
#include "IGOTelemetry.h"
#include "eathread/eathread_futex.h"

namespace OriginIGO
{
	// Check whether a specific directory exists
	bool DirectoryExists(LPCWSTR path);

	// Get the common data folder for this OS
	bool GetCommonDataPath(WCHAR buffer[MAX_PATH]);

	// Read the entire content of a file
	bool ReadFileContent(LPCWSTR fileName, char** buffer, size_t* bufferSize);

	// Compute the MD5 of the image file corresponding to a specific module
	bool ComputeFileMd5(HMODULE module, char md5[33]);

	// Check that the offset in the module points to a method
	void* ValidateMethod(HMODULE module, uint64_t offset, OriginIGO::TelemetryRenderer renderer);

   	// Save raw RGBA picture to disk in TGA format
	void SaveRawRGBAToTGA(LPCWSTR fileName, size_t width, size_t height, const unsigned char* data, bool overwrite = false);

    // Missing API we're missing while using older Windows SDK
    struct NotXPSupportedAPIs
    {
        // TMP while using older Windows SDK
        typedef HANDLE (WINAPI *CreateEventExFn)(LPSECURITY_ATTRIBUTES lpEventAttributes, LPCTSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess);
        CreateEventExFn createEventEx;
    };
    const NotXPSupportedAPIs* GetNotXPSupportedAPIs();

    // Search the loaded modules for a specific API
    void* FindModuleAPI(LPCWSTR filter, LPCSTR apiName);

    // Return the content/size of an embedded resource
    bool LoadEmbeddedResource(int resourceID, const char** content, DWORD* contentSize);

	// Class used to lookup for a method in a loaded image.
	class CodePatternFinder
	{
	public:
		struct PatternSignature
		{
			IMAGE_FILE_HEADER moduleID;
			unsigned char patternBytes[128];
			unsigned char maskBytes[128];
			size_t patternSize;
		};

	public:
		CodePatternFinder(const PatternSignature* const patterns, size_t patternsCount);

		enum FindInstanceResult
		{
			FindInstanceResult_NONE
			, FindInstanceResult_ONE
			, FindInstanceResult_MULTIPLE
		};

		FindInstanceResult GetMethod(HMODULE module, void** method);

	private:
		size_t mPatternsCount;
		const PatternSignature* const mPatterns;
	};

    // This class has the same functionality as the original EA class, except that it tries to lock the futex instead of waiting for the lock forever
    class AutoTryFutex
    {
    public:
        inline AutoTryFutex(EA::Thread::Futex& futex, int timeout = 0/*ms*/)
            : mFutex(futex), mTimeout(timeout)
        {
            mLocked = mFutex.TryLock();
            while (!mLocked && mTimeout > 0)
            {
                Sleep(1);
                mTimeout--;
                mLocked = mFutex.TryLock();
            }

        }
        inline ~AutoTryFutex()
        {
            if (mLocked)
                mFutex.Unlock();
        }

        inline bool Locked() const { return mLocked; }

    protected:
        EA::Thread::Futex& mFutex;

        // Prevent copying by default, as copying is dangerous.
        AutoTryFutex(const AutoTryFutex&);
        const AutoTryFutex operator=(const AutoTryFutex&);

    private:
        bool mLocked;
        int mTimeout;
    };

////////////////////////////////////////////////////////////////////////////////////////////

    struct D3DXVECTOR4
    {
    public:
        D3DXVECTOR4() {};
        D3DXVECTOR4(FLOAT x, FLOAT y, FLOAT z, FLOAT w) { this->x = x; this->y = y; this->z = z; this->w = w; }

    public:
        FLOAT x, y, z, w;
    };

    struct D3DXVECTOR2
    {
    public:
        D3DXVECTOR2() {};
        D3DXVECTOR2(FLOAT x, FLOAT y) { this->x = x; this->y = y; }

    public:
        FLOAT x, y;
    };
} // OriginIGO

#else // ORIGIN_MAC

#include <MacTypes.h>

#endif // ORIGIN_PC

// Supported by both platforms

namespace OriginIGO
{

    // Helper to keep track of deltaT
    class IntervalKeeper
    {
    public:
        IntervalKeeper();
        ~IntervalKeeper();

        void Update();

        float deltaInMS() const { return mDeltaInMS; }

    private:
#if defined(ORIGIN_MAC)
        UInt64 mRefTime;
#elif defined(ORIGIN_PC)
        LONGLONG mRefTime;
        double mRefFrequency;
#endif
        float mDeltaInMS;
    };

} // OriginIGO

#endif