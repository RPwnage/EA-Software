/*************************************************************************************************/
/*!
    \file   fifa_hmac.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "EAStdC/EAMemory.h"
#include "EAStdC/EAString.h"

#include "fifa_hmac.h"
#include "gamereporting/fifa/fifa_encryption/fifacrypto.h"
#include "framework/util/hashutil.h"

#include "fifa/tdf/fifah2hreport.h"
#include "fifa/tdf/fifasoloreport.h"

#include "framework/sampleLarge.h"
#include "framework/sampleSmall.h"
#include "framework/sampleTiny.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

const char* GetMatchResultString(uint16_t result)
{
	enum FUTMatchResultType
	{
		// from fifaonline.h
		FUT_MATCHRESULT_INVALID = 0, // "", should never happen as defaulted to DNF

		FUT_MATCHRESULT_WIN, // "WIN", all caps
		FUT_MATCHRESULT_DRAW, // "DRAW"
		FUT_MATCHRESULT_LOSS, // "LOSS"

		FUT_MATCHRESULT_DNF, // "DNF"
		FUT_MATCHRESULT_QUIT, // "FORFEIT"
		FUT_MATCHRESULT_NO_CONTEST, // "NO_CONTEST"
	};

	switch(result)
	{
	default:
	case FUT_MATCHRESULT_INVALID: return "";
	case FUT_MATCHRESULT_WIN: return "WIN";
	case FUT_MATCHRESULT_DRAW: return "DRAW";
	case FUT_MATCHRESULT_LOSS: return "LOSS";
	case FUT_MATCHRESULT_DNF: return "DNF";
	case FUT_MATCHRESULT_QUIT: return "FORFEIT";
	case FUT_MATCHRESULT_NO_CONTEST: return "NO_CONTEST";
	}
}

//OSDKPlayerReport& GetHomeOrUserPlayerReport(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportMap, bool isSolo, bool isHomeOrUser, uint64_t& personaId)
//{
//	// solo : isHomeOrUser? user : ai
//	// h2h: isHomeOrUser? home : away
//	unsigned int idx = 0;
//
//	if(isSolo)
//	{
//		// playerReportMap with key 0 (OSDK::USER_ID_INVALID) indicates AI report
//		idx = (isHomeOrUser)? ((playerReportMap.at(0).first > 0)? 0 : 1) : ((playerReportMap.at(0).first > 0)? 1 : 0);
//	}
//	else
//	{
//		idx = (isHomeOrUser)? ((playerReportMap.at(0).second->getHome())? 0 : 1) : ((playerReportMap.at(0).second->getHome())? 1 : 0);
//	}
//	personaId = playerReportMap.at(idx).first;
//	return *(playerReportMap.at(idx).second);
//}

struct SideReport
{
	SideReport(OSDKGameReportBase::OSDKReport& osdkReport, bool isSolo, bool isHomeOrUser)
	{
		isValid = false;
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportMap = osdkReport.getPlayerReports();

		// solo : isHomeOrUser? user : ai
		// h2h: isHomeOrUser? home : away
		unsigned int idx = 0;

		if(playerReportMap.size() == 0)
		{
			TRACE_LOG(__FUNCTION__ << " Empty playerReportMap for user " << gCurrentUserSession->getBlazeId());
		}
		else
		{
			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
			playerIter = playerReportMap.begin();
			playerEnd = playerReportMap.end();

			if (isSolo)
			{
				idx = 0;
				OSDKPlayerReport* playerReport = playerReportMap.at(idx).second;

				if (playerReport != NULL)
				{
					Fifa::SoloPlayerReport* soloPlayerReport = static_cast<Fifa::SoloPlayerReport*>(playerReport->getCustomPlayerReport());
	
					goals = (soloPlayerReport != NULL) ? (soloPlayerReport->getCommonPlayerReport().getGoals() + soloPlayerReport->getCommonPlayerReport().getPkGoals()) : 0;
					oppGoals = (soloPlayerReport != NULL) ? soloPlayerReport->getSoloCustomPlayerData().getOppGoal() : 0;
					matchTime = static_cast<uint64_t>(playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap()["futMatchTime"]);
					result = GetMatchResultString(static_cast<uint16_t>(playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap()["futMatchResult"]));
					personaId = playerReportMap.at(idx).first;
					nucleusId = playerReport->getNucleusId();
					isValid = true;		
				}
				else
				{
					TRACE_LOG(__FUNCTION__ << " Solo report NULL playerReport encountered for user " << gCurrentUserSession->getBlazeId());
				}
			}
			else  
			{
				idx = (isHomeOrUser)? ((playerReportMap.at(0).second->getHome())? 0 : 1) : ((playerReportMap.at(0).second->getHome())? 1 : 0);

				if (playerReportMap.size() > idx)
				{
					OSDKPlayerReport* playerReport = playerReportMap.at(idx).second;

					if (playerReport != NULL)
					{
						Fifa::H2HPlayerReport* h2hPlayerReport = static_cast<Fifa::H2HPlayerReport*>(playerReport->getCustomPlayerReport());

						goals = (h2hPlayerReport != NULL) ? h2hPlayerReport->getCommonPlayerReport().getUnadjustedScore() : 0;
						matchTime = static_cast<uint64_t>(playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap()["futMatchTime"]);
						result = GetMatchResultString(static_cast<uint16_t>(playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap()["futMatchResult"]));
						personaId = playerReportMap.at(idx).first;
						nucleusId = playerReport->getNucleusId();
						isValid = true;
					}
					else
					{
						TRACE_LOG(__FUNCTION__ << " H2H report NULL playerReport encountered for user " << gCurrentUserSession->getBlazeId());
					}
				}
				else
				{
					TRACE_LOG(__FUNCTION__ << " H2H report with playerReport index " << idx << " larger than playerReportMap size " << playerReportMap.size() << ". User id " << gCurrentUserSession->getBlazeId() << ", is HOME = " << isHomeOrUser);
				}
			}
		}
	}

	uint64_t matchTime;
	uint64_t personaId;
	uint64_t nucleusId;
	const char* result;
	uint32_t goals;
	uint32_t oppGoals;
	bool isValid;
};

FifaHMAC* FifaHMAC::mSingleton = NULL;

static const int MAX_LEN = 256;
static const int KEY_LEN = Blaze::HashUtil::SHA512_HEXSTRING_OUT;

void FifaHMAC::Create(GameReportingSlaveImpl& component)
{
	if(mSingleton == NULL)
	{
		mSingleton = BLAZE_NEW_NAMED ("FifaHMAC") FifaHMAC();
		mSingleton->SetHMACKeyConfig(component);
		mSingleton->LoadKeys();
	}
}

FifaHMAC* FifaHMAC::Instance()
{
	return mSingleton;
}

void FifaHMAC::Destroy()
{
	if(mSingleton != NULL)
	{
		delete mSingleton;
		mSingleton = NULL;
	}
}

FifaHMAC::~FifaHMAC()
{
	for(unsigned int i = 0; i < mNumKeys; ++i) 
	{
		delete [] mKeys[i];
		mKeys[i] = NULL;
	}

	delete [] mKeys;
	mKeys = NULL;
	delete [] mPeppers;
	mPeppers = NULL;
}

void FifaHMAC::SetHMACKeyConfig(GameReportingSlaveImpl& component)
{
	// defined in gamereporting.cfg
	mNumKeys = component.getConfig().getHmacConfig().getHmacNumKeys();
	mEntryLen = component.getConfig().getHmacConfig().getHmacKeyLineWidth();
	mFilename = component.getConfig().getHmacConfig().getHmacKeyFile();
	mKeyIndexOverride = component.getConfig().getHmacConfig().getHmacKeyIndexOverride();

	TRACE_LOG("[" << __FUNCTION__ << "] File: " << mFilename << " Num keys: " << mNumKeys << " entry length: " << mEntryLen << " override index: " << mKeyIndexOverride);
}

void ReadKey(char* line, char8_t* key, uint64_t& pepper)
{
	uint64_t len = 0;
	const char8_t* tok = EA::StdC::Strtok2(line, ",", static_cast<size_t*>(&len), true);
	tok = EA::StdC::Strtok2(tok, ",", static_cast<size_t*>(&len), false);
	EA::StdC::Strncpy(key, tok, len);

	tok = EA::StdC::Strtok2(tok, ",", static_cast<size_t*>(&len), false);
	pepper = EA::StdC::AtoU64(tok);
}

void FifaHMAC::LoadKeys()
{
	TRACE_LOG(__FUNCTION__ << " begin");
	mKeys = BLAZE_NEW_ARRAY(char8_t*, mNumKeys);
	for(unsigned int i = 0; i < mNumKeys; ++i)
	{
		mKeys[i] = BLAZE_NEW_ARRAY(char8_t, KEY_LEN);
		EA::StdC::Memclear(mKeys[i], KEY_LEN);
	}

	mPeppers = BLAZE_NEW_ARRAY(uint64_t, mNumKeys);

	FifaOnline::Security::FifaCrypto crypto;
	FifaOnline::Security::FifaDecryptedPayload payload(Blaze::GameReporting::SampleData_large_bin_len);

	int regStrSize = Blaze::GameReporting::SampleData_small_bin_len + 1 ;
	int strSize = regStrSize + (16 - (regStrSize % 16));
	char cipher[] = "SampleData_Small";		

	char *sampleDataSmall = BLAZE_NEW_ARRAY(char, strSize);
	char *sampleDataSmallBin = (char*)(&Blaze::GameReporting::SampleData_small_bin[0]);
	crypto.ObfuscateString(cipher, sampleDataSmallBin, Blaze::GameReporting::SampleData_small_bin_len, sampleDataSmall, strSize);

	int tinyStrSize = Blaze::GameReporting::SampleData_tiny_bin_len + 1 ;
	int tinySize = tinyStrSize + (16 - (tinyStrSize % 16));
	char *sampleDataTiny = BLAZE_NEW_ARRAY(char, tinySize);
	char *sampleDataTinyBin = (char*)(&Blaze::GameReporting::SampleData_tiny_bin[0]);
	crypto.ObfuscateString(cipher, sampleDataTinyBin, Blaze::GameReporting::SampleData_tiny_bin_len, sampleDataTiny, tinySize);
	
	int32_t result = crypto.Decrypt(sampleDataTiny, sampleDataSmall, Blaze::GameReporting::SampleData_large_bin, Blaze::GameReporting::SampleData_large_bin_len, &payload);

	memset(sampleDataTiny, 0, tinyStrSize);
	delete[] sampleDataTiny;
	
	memset(sampleDataSmall, 0, strSize);
	delete[] sampleDataSmall;		

	memset(Blaze::GameReporting::SampleData_small_bin, 0, Blaze::GameReporting::SampleData_small_bin_len);
	memset(Blaze::GameReporting::SampleData_tiny_bin, 0, Blaze::GameReporting::SampleData_tiny_bin_len);
	memset(Blaze::GameReporting::SampleData_large_bin , 0, Blaze::GameReporting::SampleData_large_bin_len);

	if(result == ERR_OK)
	{
		char8_t* pContext = NULL;
		char8_t* pResult;
		unsigned int idx = 0;
		while((pResult = EA::StdC::Strtok(pContext ? NULL : (char8_t*)payload.mDataPayload, "\r\n", &pContext)) != NULL)
		{
			ReadKey(pResult, mKeys[idx], mPeppers[idx]);
			if(EA::StdC::Strlen(mKeys[idx]) == (KEY_LEN - 1)) // needs to be 128
			{
				//TRACE_LOG("Add Key " << idx << ": " << mKeys[idx] << " Pepper: " << mPeppers[idx]);
				idx++;
			}
		}
	}
	else{
		ERR_LOG(__FUNCTION__ <<" Sample Data failed");
	}	
	
	TRACE_LOG(__FUNCTION__ << " end");
}

bool FifaHMAC::GetKey(uint64_t time, uint64_t personaid, uint64_t matchreportid, char* key, uint64_t& pepper)
{
	bool success = false;
	
	if(time > 0 && personaid > 0 && matchreportid > 0 && mNumKeys > 0)
	{
		unsigned int index = static_cast<unsigned int>((time ^ personaid ^ matchreportid) % mNumKeys);

		TRACE_LOG("Index returned: " << index);

		if(mKeyIndexOverride >= 0)
		{
			index = static_cast<unsigned int>(mKeyIndexOverride);
			TRACE_LOG("Index override: " << index);
		}

		if(index < mNumKeys)
		{
			key = EA::StdC::Strncpy(key, mKeys[index], KEY_LEN);
			pepper = mPeppers[index];
			success = true;

			TRACE_LOG("[M]Key " << index << ": " << key << " len " << EA::StdC::Strlen(key));
			TRACE_LOG("[M]Pepper: " << pepper);
		}
	}

	return success;
}

void FifaHMAC::GenerateHMACFromGameReport(Blaze::GameReporting::GameReport& report, char8_t* buffer)
{
	// for solo matches
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*report.getReport());

	const char8_t* gameTypeName = osdkReport.getGameReport().getGameType() + strlen("gameType");
	int32_t gameModeId = atoi(gameTypeName);
	uint64_t gameReportId = report.getGameReportingId();

	SideReport user(osdkReport, true, true);

	if (user.isValid == true)
	{
		char8_t key[KEY_LEN] = "";
		uint64_t pepper = 0;

		bool valid = GetKey(user.matchTime, user.personaId, gameReportId, key, pepper);
		if(valid)
		{
			size_t keylen = EA::StdC::Strlen(key);
			eastl::string payload;
			payload.sprintf("%llu|%llu|%llu|%010llu|%llu", user.personaId, user.nucleusId, user.matchTime, pepper, 0ull);
			payload.append_sprintf("|%d|%s|%llu|%u|%u", gameModeId, user.result, gameReportId, user.oppGoals, user.goals);

			Blaze::HashUtil::generateSHA512HMACHash(key, keylen, payload.c_str(), payload.length(), buffer, Blaze::HashUtil::SHA512_HEXSTRING_OUT+1);

			TRACE_LOG("User payload: " << payload.c_str() << " len " << payload.length());
			TRACE_LOG("User hash: " << buffer);
		}
	}
}

void FifaHMAC::GenerateHMACFromGameReport(Blaze::GameReporting::GameReport& report, char8_t* homeBuf, char8_t* awayBuf)
{
	// for H2H matches

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*report.getReport());

	const char8_t* gameTypeName = osdkReport.getGameReport().getGameType() + strlen("gameType");
	int32_t gameModeId = atoi(gameTypeName);
	uint64_t gameReportId = report.getGameReportingId();

	SideReport home(osdkReport, false, true);
	SideReport away(osdkReport, false, false);

	if (home.isValid == true && away.isValid == true)
	{
		char8_t key[KEY_LEN] = "";
		uint64_t pepper = 0;
		size_t keylen = 0;
		eastl::string payload;

		bool valid = GetKey(home.matchTime, home.personaId, gameReportId, key, pepper);
		if(valid)
		{
			keylen = EA::StdC::Strlen(key);
			payload.sprintf("%llu|%llu|%llu|%010llu|%llu", home.personaId, home.nucleusId, home.matchTime, pepper, away.personaId);
			payload.append_sprintf("|%d|%s|%llu|%u|%u", gameModeId, home.result, gameReportId, away.goals, home.goals);

			Blaze::HashUtil::generateSHA512HMACHash(key, keylen, payload.c_str(), payload.length(), homeBuf, Blaze::HashUtil::SHA512_HEXSTRING_OUT+1);

			TRACE_LOG("Home payload: " << payload.c_str() << " len " << payload.length());
			TRACE_LOG("Home hash: " << homeBuf);
		}

		valid = GetKey(away.matchTime, away.personaId, gameReportId, key, pepper);
		if(valid)
		{
			keylen = EA::StdC::Strlen(key);
			payload.sprintf("%llu|%llu|%llu|%010llu|%llu", away.personaId, away.nucleusId, away.matchTime, pepper, home.personaId);
			payload.append_sprintf("|%d|%s|%llu|%u|%u", gameModeId, away.result, gameReportId, home.goals, away.goals);

			Blaze::HashUtil::generateSHA512HMACHash(key, keylen, payload.c_str(), payload.length(), awayBuf, Blaze::HashUtil::SHA512_HEXSTRING_OUT+1);

			TRACE_LOG("Away payload: " << payload.c_str() << " len " << payload.length());
			TRACE_LOG("Away hash: " << awayBuf);
		}
	}
}

}   // namespace GameReporting
}   // namespace Blaze
