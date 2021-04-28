/*************************************************************************************************/
/*!
    \file   fifa_vpro.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "fifa_vpro.h"
#include "fifa_vproextensions.h"

#include "xmshd/tdf/xmshdtypes.h"
#include "xmshd/xmshdslaveimpl.h" 
#include "vprogrowthunlocks/vprogrowthunlocksslaveimpl.h"
#include "EAStdC/EATextUtil.h"

namespace Blaze
{
namespace GameReporting
{

#define		KEYSCOPE_NAME_POS		"pos"
#define		KEYSCOPE_NAME_CLUBID	"club"
#define		KEYSCOPE_NAME_VPRO_ATTRIBUTE_TYPE	"attributetype"
#define		STAT_VPRO_NAME			"proName"
#define		STAT_VPRO_POS			"proPos"
#define		STAT_VPRO_STYLE			"proStyle"
#define		STAT_VPRO_HEIGHT		"proHeight"
#define		STAT_VPRO_NATIONALITY	"proNationality"
#define		STAT_VPRO_OVERALL		"proOverall"
#define		STAT_VPRO_OVERALL_STR	"proOverallStr"
#define		STAT_VPRO_CLUB_GAMES	"clubgames"
#define		STAT_VPRO_OTP_GAMES		"otpgames"

#define		ATTR_VPRO_NAME			"VProName"
#define		ATTR_VPRO_POS			"VProPos"
#define		ATTR_VPRO_STYLE			"VProStyle"
#define		ATTR_VPRO_HEIGHT		"VProHeight"
#define		ATTR_VPRO_NATIONALITY	"VProNationality"
#define		ATTR_VPRO_OVERALL		"VProOverall"

#define		CLUB_STATS_TAG_SUFFIX	"CLUBS"
#define		CUP_STATS_TAG_SUFFIX	"CLUBSCUP"
#define		DROPIN_STATS_TAG_SUFFIX "DROPIN"
#define		STAT_KEY_FORMAT			"vprostat_savedstatrole%dc%d"

#define		CLUB_ID_XML_FORMAT	"\t<CLUBID>%s</CLUBID>\r\n"
#define		CLUB_ID_KEY			"clubId"

//Different from GKROLESTATS,DEFROLESTATS, etc. so as to not confuse already present parsing logic
const char* FifaVpro::mXmlRoleTagMapping[VPRO_POS_NUM] = {
	"<%sGKROLESTATS%s>", "<%sDEFROLESTATS%s>", "<%sMIDROLESTATS%s>", "<%sFWDROLESTATS%s>"
};

void FifaVpro::initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport)
{
	mBuilder = builder;
	mProcessedReport = processedReport;
	mUpdateStatsHelper = updateStatsHelper;
}

void FifaVpro::setExtension(IFifaVproExtension* vproExtension)
{
	mVproExtension = vproExtension;
}

void FifaVpro::updateVproStats(CategoryInfoVector& categoryInfoVector)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		OSDKGameReportBase::OSDKPlayerReport* playerReport = playerIter->second;

		CategoryInfoVector::iterator catIter, catEnd;
		catIter = categoryInfoVector.begin();
		catEnd = categoryInfoVector.end();

		Collections::AttributeMap& map = playerReport->getPrivatePlayerReport().getPrivateAttributeMap();

		for (; catIter != catEnd; ++catIter)
		{
			Stats::ScopeNameValueMap scopeNamevalueMap;

			CategoryInfo* categoryInfo = catIter;
			switch(categoryInfo->keyScopeType)
			{
			case KEYSCOPE_POS:
				scopeNamevalueMap.insert(eastl::make_pair(KEYSCOPE_NAME_POS, mVproExtension->getPosForPlayer(playerReport)));
				break;
			case KEYSCOPE_CLUBID:
				scopeNamevalueMap.insert(eastl::make_pair(KEYSCOPE_NAME_CLUBID, mVproExtension->getClubForPlayer(playerReport)));
				break;
			case KEYSCOPE_NONE:
			default:
				break;

			}

			mBuilder->startStatRow(categoryInfo->statCategory, playerIter->first, scopeNamevalueMap.size() > 0? &scopeNamevalueMap : NULL);

			Collections::AttributeMap::iterator iter = map.find(ATTR_VPRO_NAME);
			if (iter != map.end() && (NULL != iter->second) && (0 != iter->second[0]))
			{
				mBuilder->assignStat(STAT_VPRO_NAME, iter->second);
			}

			iter = map.find(ATTR_VPRO_POS);
			if (iter != map.end() && (NULL != iter->second) && (0 != iter->second[0]))
			{
				mBuilder->assignStat(STAT_VPRO_POS, iter->second);
			}

			iter = map.find(ATTR_VPRO_STYLE);
			if (iter != map.end() && (NULL != iter->second) && (0 != iter->second[0]))
			{
				mBuilder->assignStat(STAT_VPRO_STYLE, iter->second);
			}

			iter = map.find(ATTR_VPRO_HEIGHT);
			if (iter != map.end() && (NULL != iter->second) && (0 != iter->second[0]))
			{
				mBuilder->assignStat(STAT_VPRO_HEIGHT, iter->second);
			}

			iter = map.find(ATTR_VPRO_NATIONALITY);
			if (iter != map.end() && (NULL != iter->second) && (0 != iter->second[0]))
			{
				mBuilder->assignStat(STAT_VPRO_NATIONALITY, iter->second);
			}

			iter = map.find(ATTR_VPRO_OVERALL);
			if (iter != map.end() && (NULL != iter->second) && (0 != iter->second[0]))
			{
				mBuilder->assignStat(STAT_VPRO_OVERALL_STR, iter->second);
				mBuilder->assignStat(STAT_VPRO_OVERALL,     atoi(iter->second));
			}


			mBuilder->completeStatRow();
		}
	}
}

bool FifaVpro::CompareStatValues(const char* sourceStr, const char* targetStr)
{
	if ((sourceStr != NULL) && (targetStr != NULL))
	{
		char16_t srcValue = 0;
		char16_t tgtValue = 0;
		do
		{
			srcValue = EA::StdC::UTF8ReadChar(sourceStr, const_cast<const char8_t**>(&sourceStr));
			tgtValue = EA::StdC::UTF8ReadChar(targetStr, const_cast<const char8_t**>(&targetStr));

			if (tgtValue < srcValue)
			{
				return false;
			}
		}
		while ((srcValue != 0) && (tgtValue != 0));

		return true;
	}
	return false;
}


int FifaVpro::CountVProBits(const char* sourceStr)
{
	const int MAX_ACCOMPLISHMENTS = 302;

	const unsigned char oneBits[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
	const unsigned char lastMask[] = { 0x00, 0x01, 0x03, 0x7, 0x0f, 0x1f };

	unsigned int sourceBits = 0;

	for (int i = 0; (i <= (MAX_ACCOMPLISHMENTS / 6)) && (sourceStr != NULL) && (*sourceStr != 0); ++i, ++sourceStr)
	{
		char c = *sourceStr & 0x3f; // We only use the last 6 bits
		sourceBits += oneBits[c & 0x0f];
		sourceBits += oneBits[c >> 4];
	}

	if ((sourceStr != NULL) && (*sourceStr != 0))
	{
		char c = *sourceStr & lastMask[MAX_ACCOMPLISHMENTS % 6];

		sourceBits += oneBits[c & 0x0f];
		sourceBits += oneBits[c >> 4];
	}

	return sourceBits;
}

bool FifaVpro::CompareBitCount(const char* sourceStr, const char* targetStr)
{
	if(targetStr == NULL)
	{
		ERR_LOG("[FifaVpro::CompareBitCount] - targetStr is NULL");
		return false;
	}
	
	// this number comes from accomplishments_online.ini from the client perforce depot
	const int MAX_ACCOMPLISHMENTS = 313; 

	unsigned char* tempSourceStr = (unsigned char*)sourceStr;
	unsigned char* tempTargetStr = (unsigned char*)targetStr;

	// accomplishments are stored in bits in a char.  6 right most bits are for accomplishments.
	// 2 left most bits are set as 01 and should not be tampered with.
	int numOfChars = (MAX_ACCOMPLISHMENTS / 6) + ((MAX_ACCOMPLISHMENTS % 6) > 0 ? 1 : 0);

	// iterate through accomplishments, char by char
	bool retVal = true;
	for(int i = 0; i < numOfChars; ++i)
	{
		// detect if first two bits in char have been modified
		if((*tempTargetStr & 0xC0) != 0x40)
		{
			TRACE_LOG("[FifaVpro::CompareBitCount] - Marker bits were modifed");
			retVal = false;
			break;
		}

		if(tempSourceStr != NULL && *tempSourceStr != '\0')
		{
			// detect bitwise equivalence and verify against target string to check if any source bits flipped from 1 to 0
			unsigned char bitwiseEquivResult = *tempSourceStr ^ *tempTargetStr; 
			unsigned char detectSourceBitsZeroed = *tempSourceStr & bitwiseEquivResult;
			if(detectSourceBitsZeroed > 0)
			{
				TRACE_LOG("[FifaVpro::CompareBitCount] - Bits were zeroed from the source - Indicates either cheating or error with client data");
				retVal = false;
				break;
			}

			++tempSourceStr;
			++tempTargetStr;
		}
		else
		{
			TRACE_LOG("[FifaVpro::CompareBitCount] - Source string is empty.  Overwrite with target.");
			break;
		}
		
	}

	return retVal;
}

bool FifaVpro::updateStatIfPresent(Stats::UpdateRowKey* key, Collections::AttributeMap& privateStringMap, const char *toStat, const char *fromStat, ValidateFunction validateFunc)
{
	Collections::AttributeMap::iterator it = privateStringMap.find(fromStat);
	if (it != privateStringMap.end() && it->second.c_str() != NULL && it->second.c_str()[0] != 0)
	{
		if (validateFunc != NULL)
		{
			const char * sourceStr = mUpdateStatsHelper->getValueString(key, toStat, Stats::STAT_PERIOD_ALL_TIME, false);
			
			if (!validateFunc(sourceStr, it->second.c_str()))
			{
				return false;
			}
		}

		mUpdateStatsHelper->setValueString(key, toStat, Blaze::Stats::STAT_PERIOD_ALL_TIME, it->second.c_str());
		return true;
	}
	return false;
}


void FifaVpro::selectStats(CategoryInfoVector& categoryInfoVector)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		OSDKGameReportBase::OSDKPlayerReport* playerReport = playerIter->second;
		OSDKGameReportBase::IntegerAttributeMap& privateIntMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();

		OSDKGameReportBase::IntegerAttributeMap::iterator it = privateIntMap.find("vpro_isvalid");

		if ((it != privateIntMap.end()) && (it->second == 1))
		{
			CategoryInfoVector::iterator catIter, catEnd;
			catIter = categoryInfoVector.begin();
			catEnd = categoryInfoVector.end();

			for (; catIter != catEnd; ++catIter)
			{
				CategoryInfo* categoryInfo = catIter;
				mBuilder->startStatRow(categoryInfo->statCategory,	playerIter->first, NULL);

				// No need to do the selectStat calls - we'll get the whole row either way
				mBuilder->completeStatRow();
			}
		}
	}
}



void FifaVpro::updateVproGameStats(CategoryInfoVector& categoryInfoVector)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		OSDKGameReportBase::OSDKPlayerReport* playerReport = playerIter->second;
		Collections::AttributeMap& privateStringMap = playerReport->getPrivatePlayerReport().getPrivateAttributeMap();
		OSDKGameReportBase::IntegerAttributeMap& privateIntMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();

		OSDKGameReportBase::IntegerAttributeMap::iterator it = privateIntMap.find("vpro_isvalid");

		if ((it != privateIntMap.end()) && (it->second == 1))
		{
			CategoryInfoVector::iterator catIter, catEnd;
			catIter = categoryInfoVector.begin();
			catEnd = categoryInfoVector.end();
			
			for (; catIter != catEnd; ++catIter)
			{
				CategoryInfo* categoryInfo = catIter;
				Stats::UpdateRowKey* key = mBuilder->getUpdateRowKey(categoryInfo->statCategory, playerIter->first);

				mBuilder->completeStatRow();
				if ((NULL != key) && updateStatIfPresent(key, privateStringMap, "accomplishments",		"vprostat_accomplishment", CompareBitCount))
				{
					updateStatIfPresent(key, privateStringMap, "gkRoleStats",                "vprostat_savedstatrole0c0");
					updateStatIfPresent(key, privateStringMap, "defRoleStats",				"vprostat_savedstatrole1c0");
					updateStatIfPresent(key, privateStringMap, "midRoleStats",				"vprostat_savedstatrole2c0");
					updateStatIfPresent(key, privateStringMap, "fwdRoleStats",				"vprostat_savedstatrole3c0");
					updateStatIfPresent(key, privateStringMap, "gkRoleStatsClubs",			"vprostat_savedstatrole0c1");
					updateStatIfPresent(key, privateStringMap, "defRoleStatsClubs",			"vprostat_savedstatrole1c1");
					updateStatIfPresent(key, privateStringMap, "midRoleStatsClubs",			"vprostat_savedstatrole2c1");
					updateStatIfPresent(key, privateStringMap, "fwdRoleStatsClubs",			"vprostat_savedstatrole3c1");
					updateStatIfPresent(key, privateStringMap, "gkRoleStatsClubsCup",		"vprostat_savedstatrole0c2");
					updateStatIfPresent(key, privateStringMap, "defRoleStatsClubsCup",		"vprostat_savedstatrole1c2");
					updateStatIfPresent(key, privateStringMap, "midRoleStatsClubsCup",		"vprostat_savedstatrole2c2");
					updateStatIfPresent(key, privateStringMap, "fwdRoleStatsClubsCup",		"vprostat_savedstatrole3c2");
					updateStatIfPresent(key, privateStringMap, "gkRoleStatsDropIn",			"vprostat_savedstatrole0c3");
					updateStatIfPresent(key, privateStringMap, "defRoleStatsDropIn",		"vprostat_savedstatrole1c3");
					updateStatIfPresent(key, privateStringMap, "midRoleStatsDropIn",		"vprostat_savedstatrole2c3");
					updateStatIfPresent(key, privateStringMap, "fwdRoleStatsDropIn",		"vprostat_savedstatrole3c3");
					updateStatIfPresent(key, privateStringMap, "consecutiveInGameStats",	"vprostat_ingame");
				}
				else
				{
					ERR_LOG("FifaVpro::updateVproGameStats() Did not update accomplishments user["<<playerIter->first << "] key["<<(key != NULL)<<"]");
				}
			}
			
			Blaze::XmsHd::XmsHdSlave *xmsHdComponent = static_cast<Blaze::XmsHd::XmsHdSlave*>(gController->getComponent(Blaze::XmsHd::XmsHdSlave::COMPONENT_ID, false));

			if (xmsHdComponent != NULL)
			{
				Blaze::XmsHd::PublishDataRequest request;

				uint64_t nucleusId = playerReport->getNucleusId();

				eastl::string vproStatXmlData;

				vproStatXmlData.sprintf("<VPRO_STAT>\r\n");

				vproStatXmlData.append_sprintf("\t<GKROLESTATS>%s</GKROLESTATS>\r\n",								privateStringMap["vprostat_savedstatrole0"].c_str());
				vproStatXmlData.append_sprintf("\t<DEFROLESTATS>%s</DEFROLESTATS>\r\n",								privateStringMap["vprostat_savedstatrole1"].c_str());
				vproStatXmlData.append_sprintf("\t<MIDROLESTATS>%s</MIDROLESTATS>\r\n",								privateStringMap["vprostat_savedstatrole2"].c_str());
				vproStatXmlData.append_sprintf("\t<FWDROLESTATS>%s</FWDROLESTATS>\r\n",								privateStringMap["vprostat_savedstatrole3"].c_str());
				vproStatXmlData.append_sprintf("\t<ACCOMPLISHMENTS>%s</ACCOMPLISHMENTS>\r\n",						privateStringMap["vprostat_accomplishment"].c_str());
				vproStatXmlData.append_sprintf("\t<CONSECUTIVEINGAMESTATS>%s</CONSECUTIVEINGAMESTATS>\r\n",			privateStringMap["vprostat_ingame"].c_str());

				const char* strClubId = "";
				Collections::AttributeMap::iterator psmIt = privateStringMap.find(CLUB_ID_KEY);
				if (psmIt != privateStringMap.end())
				{
					strClubId = psmIt->second.c_str();
				}
				vproStatXmlData.append_sprintf(CLUB_ID_XML_FORMAT, strClubId);

				for (int i = VPRO_COMPETITION_CLUBS; i < VPRO_COMPETITION_NUM; ++i)
				{
					const char* xmlTagSuffix = NULL;
					switch(i)
					{
					case VPRO_COMPETITION_CLUBS:
						xmlTagSuffix = CLUB_STATS_TAG_SUFFIX;
						break;
					case VPRO_COMPETITION_CLUBSCUP:
						xmlTagSuffix = CUP_STATS_TAG_SUFFIX;
						break;
					case VPRO_COMPETITION_DROPIN:
					default:
						xmlTagSuffix = DROPIN_STATS_TAG_SUFFIX;
						break;
					}

					for (int j = 0; j < VPRO_POS_NUM; j++)
					{
						vproStatXmlData.append_sprintf(mXmlRoleTagMapping[j], "", xmlTagSuffix);
						eastl::string key;
						key.append_sprintf(STAT_KEY_FORMAT, j, i);
						Collections::AttributeMap::iterator privateStrIt = privateStringMap.find(key.c_str());
						if (privateStrIt != privateStringMap.end())
						{
							vproStatXmlData.append(privateStrIt->second);
						}
						vproStatXmlData.append_sprintf(mXmlRoleTagMapping[j], "/", xmlTagSuffix);
						vproStatXmlData.append_sprintf("\r\n");
					}
					vproStatXmlData.append_sprintf("\r\n");
				}

				vproStatXmlData.append_sprintf("</VPRO_STAT>");

				TRACE_LOG("xmlPayload: " << vproStatXmlData.c_str());

				// Push the XmsHD RPC call to another thread to free the game reporting thread up.
				gSelector->scheduleFiberCall<FifaVpro, uint64_t, const char*, eastl::string>(
					this, 
					&FifaVpro::sendDataToXmsFiberCall,
					nucleusId,
					privateStringMap[ATTR_VPRO_NAME].c_str(),
					vproStatXmlData,
					"FifaVpro::updateVproGameStats");

			}			
		}

		//clear out string data in case they result in bad chars for the xml event feed
		for (int i = 0; i < VPRO_POS_NUM; i++)
		{
			for (int j = 0; j < VPRO_COMPETITION_NUM; j++)
			{
				eastl::string key;
				key.sprintf(STAT_KEY_FORMAT, i, j);
				privateStringMap[key.c_str()] = "";
			}
		}
		privateStringMap["vprostat_accomplishment"] = "";
		privateStringMap["vprostat_ingame"] = "";
	}
}

void FifaVpro::updatePlayerGrowth()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	eastl::string fieldName;

	for (; playerIter != playerEnd; ++playerIter)
	{
		OSDKGameReportBase::OSDKPlayerReport* playerReport = playerIter->second;
		OSDKGameReportBase::IntegerAttributeMap& privateIntMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();

		int64_t vproIsValid = privateIntMap["vpro_isvalid"];
		int64_t eventCount = privateIntMap["match_event_count"];
		int64_t gameEndWithEnoughPlayer = privateIntMap["game_end_w_enough_player"];

		if (vproIsValid != 0 && gameEndWithEnoughPlayer != 0)
		{
			Blaze::VProGrowthUnlocks::VProGrowthUnlocksSlave *vproGrowthUnlocksComponent = static_cast<Blaze::VProGrowthUnlocks::VProGrowthUnlocksSlave*>(gController->getComponent(Blaze::VProGrowthUnlocks::VProGrowthUnlocksSlave::COMPONENT_ID, false));
			if (vproGrowthUnlocksComponent != NULL)
			{
				Blaze::VProGrowthUnlocks::UpdatePlayerGrowthRequest request;
				Blaze::VProGrowthUnlocks::UpdatePlayerGrowthResponse response;
				request.setUserId(playerIter->first);

				auto iter = privateIntMap.lower_bound("matchevent_0");
				for (int i = 0; i < eventCount && iter != privateIntMap.end(); i++, iter++)
				{
					const char *key = iter->first.c_str();
					uint32_t eventId = 0;
					uint32_t eventOccurences = static_cast<uint32_t>(iter->second);

					if (EA::StdC::Sscanf(key, "matchevent_%u", &eventId) == 1 && eventOccurences > 0)
					{
						request.getMatchEventsMap().insert(eastl::make_pair(eventId, eventOccurences));
					}
				}

				UserSession::pushSuperUserPrivilege();
				vproGrowthUnlocksComponent->updatePlayerGrowth(request, response);
				UserSession::popSuperUserPrivilege();
			}
		}
	}
}

void FifaVpro::sendDataToXmsFiberCall(uint64_t nucleusId, const char* vproName, eastl::string vProStatsDataXml)
{
	Blaze::XmsHd::XmsHdSlave *xmsHdComponent = static_cast<Blaze::XmsHd::XmsHdSlave*>(gController->getComponent(Blaze::XmsHd::XmsHdSlave::COMPONENT_ID, false));

	if (xmsHdComponent != NULL)
	{
		Blaze::XmsHd::PublishDataRequest request;

		request.setNucleusId(nucleusId);
		request.setXmsAssetName("fifa_online_vpro_stat.bin");

		Blaze::XmsHd::XmsAttributes* metaData = BLAZE_NEW Blaze::XmsHd::XmsAttributes;
		metaData->setName(ATTR_VPRO_NAME);
		metaData->setType("string");
		metaData->setValue("FIFA14_Online_VPro");

		request.getAttributes().push_back(metaData);

		Blaze::XmsHd::XmsBinary* binaryData = BLAZE_NEW Blaze::XmsHd::XmsBinary;
		binaryData->setName("fifa_online_vpro_stat.bin");
		binaryData->setData(vProStatsDataXml.c_str());
		binaryData->setDataSize(strlen(vProStatsDataXml.c_str()));

		request.getBinaries().push_back(binaryData);

		UserSession::pushSuperUserPrivilege();
		xmsHdComponent->publishData(request);
		UserSession::popSuperUserPrivilege();
	}
}

} //namespace GameReporting
} //namespace Blaze
