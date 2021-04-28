/*************************************************************************************************/
/*!
    \file   fifa_hmac.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_HMAC_H
#define BLAZE_CUSTOM_FIFA_HMAC_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

class FifaHMAC
{
	// singleton class
public:
	static void Create(GameReportingSlaveImpl&);
	static FifaHMAC* Instance();
	static void Destroy();

	void GenerateHMACFromGameReport(GameReport&, char8_t*);
	void GenerateHMACFromGameReport(GameReport&, char8_t*, char8_t*);

private:
	FifaHMAC() : mNumKeys(0), mEntryLen(0), mKeyIndexOverride(-1), mFilename(""), mKeys(NULL), mPeppers(NULL), mLoadKeysInMemory(true) {};
	~FifaHMAC();

	void SetHMACKeyConfig(GameReportingSlaveImpl&);
	void LoadKeys();
	bool GetKey(uint64_t, uint64_t, uint64_t, char*, uint64_t&);

	uint32_t mNumKeys;
	uint32_t mEntryLen;
	int32_t mKeyIndexOverride;
	const char8_t* mFilename;

	char8_t** mKeys;
	uint64_t* mPeppers;
	bool mLoadKeysInMemory;

	static FifaHMAC * mSingleton;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_HMAC_H

