/*************************************************************************************************/
/*!
    \file   fifacupsmasterimpl.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/fifacupsmasterimpl.cpp#2 $
    $Change: 289164 $
    $DateTime: 2011/03/08 12:47:46 $

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaCupsMasterImpl

    FifaCups Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacupsmasterimpl.h"

// global includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicator.h"
#include "framework/util/random.h"

// fifacups includes
#include "fifacups/tdf/fifacupstypes.h"
#include "fifacups/tdf/fifacupstypes_server.h"
#include "fifacups/fifacupsdb.h"
#include "fifacupstimeutils.h"

namespace Blaze
{
namespace FifaCups
{

// static
FifaCupsMaster* FifaCupsMaster::createImpl()
{
    return BLAZE_NEW FifaCupsMasterImpl();
}

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief  constructor

    

*/
/*************************************************************************************************/
FifaCupsMasterImpl::FifaCupsMasterImpl() : 
    mDbId(DbScheduler::INVALID_DB_ID),
    mProcessingEndOfSeasonDaily(false),
    mProcessingEndOfSeasonWeekly(false),
    mProcessingEndOfSeasonMonthly(false),
    mMetrics()
{
}


/*************************************************************************************************/
/*!
    \brief  destructor

    

*/
/*************************************************************************************************/    
FifaCupsMasterImpl::~FifaCupsMasterImpl()
{
}


/*************************************************************************************************/
/*!
    \brief  processStartEndOfSeasonProcessing

    receive notification from a slave that a stat period has rolled over and that end of season
    processing should be done for that period. If not already in progress, pick a slave to do the
    end of season processing for the stat period

*/
/*************************************************************************************************/
StartEndOfSeasonProcessingError::Error FifaCupsMasterImpl::processStartEndOfSeasonProcessing(const EndOfSeasonRolloverData &rolloverData, const Message* /*message*/)
{
    if (mSlaveList.empty())
    {
        ERR_LOG("[FifaCupsMasterImpl].processStartEndOfSeasonProcessing: "
            "no slaves found to run end of season processing");
        return StartEndOfSeasonProcessingError::ERR_SYSTEM;
    }

    // use a pointer to the "in progress" bool for the period type so that we can 
    // update it later without another switch statement.
    bool8_t *pAlreadyInProgress = NULL;

    // find the "in progress" bool for the period type.
    switch (rolloverData.getStatPeriodType())
    {
        case Stats::STAT_PERIOD_DAILY:
            pAlreadyInProgress = &mProcessingEndOfSeasonDaily;
            break;
        case Stats::STAT_PERIOD_WEEKLY:
            pAlreadyInProgress = &mProcessingEndOfSeasonWeekly;
            break;
        case Stats::STAT_PERIOD_MONTHLY:
            pAlreadyInProgress = &mProcessingEndOfSeasonMonthly;
            break;
        default:
            // not processing this type of period. Leave pAlreadyInProgress as NULL
            break;
    }

    // want to initiate end of season processing if it's not already in progress
    if (NULL != pAlreadyInProgress && false == *pAlreadyInProgress)
    {
        // find a random slave to do the end of season processing
        uint32_t randomSlave = Blaze::Random::getRandomNumber(static_cast<int32_t>(mSlaveList.size()));

        TRACE_LOG("[FifaCupsMasterImpl].processStartEndOfSeasonProcessing: finding slave "<<randomSlave<<" to run end of season processing for stats period "<<Stats::StatPeriodTypeToString(rolloverData.getStatPeriodType())<<" ");

        SlaveSessionList::iterator slaveIter =  mSlaveList.begin();
        SlaveSessionList::iterator slaveIter_end =  mSlaveList.end();
        for(uint32_t counter = 0; slaveIter != slaveIter_end; ++slaveIter, ++counter) 
        {
            if(randomSlave == counter)
            {
                INFO_LOG("[FifaCupsMasterImpl].processStartEndOfSeasonProcessing: Telling slave "<<slaveIter->second->getId()<<" to run end of season processing for stats period "<<Stats::StatPeriodTypeToString(rolloverData.getStatPeriodType())<<" ");
                
                sendProcessEndOfSeasonNotificationToSlaveSession(slaveIter->second, &rolloverData);

                // indicate that end of season processing is in progress. 
                *pAlreadyInProgress = true;

                // record the end of season processing start time for metrics
                mEndOfSeasonProcStartTime = TimeValue::getTimeOfDay();

                break; // no need to check the other slaves.
            }
        }

        if (false == *pAlreadyInProgress)
        {
            ERR_LOG("[FifaCupsMasterImpl].processStartEndOfSeasonProcessing: Did not find slave "<<randomSlave<<" to run end of season processing for stats period "<<Stats::StatPeriodTypeToString(rolloverData.getStatPeriodType())<<" ");
        }
    }
    else if (NULL != pAlreadyInProgress && true == *pAlreadyInProgress)
    {
        INFO_LOG("[FifaCupsMasterImpl].processStartEndOfSeasonProcessing: end of season processing already in progress for stats period "<<Stats::StatPeriodTypeToString(rolloverData.getStatPeriodType())<<" ");
    }

    return StartEndOfSeasonProcessingError::ERR_OK;
}


/*************************************************************************************************/
/*!
    \brief  processFinishedEndOfSeasonProcessing

    receive notification from a slave that end of season processing has completed

*/
/*************************************************************************************************/
FinishedEndOfSeasonProcessingError::Error FifaCupsMasterImpl::processFinishedEndOfSeasonProcessing(const EndOfSeasonRolloverData &rolloverData, const Message* /*message*/)
{
    bool8_t *pAlreadyInProgress = NULL;

    switch (rolloverData.getStatPeriodType())
    {
    case Stats::STAT_PERIOD_DAILY:
        pAlreadyInProgress = &mProcessingEndOfSeasonDaily;
        break;
    case Stats::STAT_PERIOD_WEEKLY:
        pAlreadyInProgress = &mProcessingEndOfSeasonWeekly;
        break;
    case Stats::STAT_PERIOD_MONTHLY:
        pAlreadyInProgress = &mProcessingEndOfSeasonMonthly;
        break;
    default:
        // not processing this type of period. Leave pAlreadyInProgress as NULL
        break;
    }

    if (NULL != pAlreadyInProgress)
    {
        TRACE_LOG("[FifaCupsMasterImpl].processFinishedEndOfSeasonProcessing: received notification that end of season processing has finished for stats period "<<Stats::StatPeriodTypeToString(rolloverData.getStatPeriodType())<<" ");

        *pAlreadyInProgress = false;

        // record the amount of time that end of season processing took
        mMetrics.mLastEndSeasonProcessingTimeMs.setCount(
            static_cast<uint32_t>((TimeValue::getTimeOfDay() - mEndOfSeasonProcStartTime).getMillis()));
    }

    return FinishedEndOfSeasonProcessingError::ERR_OK;
}



/*************************************************************************************************/
/*!
    \brief  getStatusInfo

    Called periodically to collect status and statistics about this component that can be used 
    to determine what future improvements should be made.

*/
/*************************************************************************************************/
void FifaCupsMasterImpl::getStatusInfo(ComponentStatus& status) const
{
    TRACE_LOG("[FifaCupsMasterImpl].getStatusInfo");

    mMetrics.report(&status);
}


bool FifaCupsMasterImpl::onConfigure()
{
    TRACE_LOG("[FifaCupsMasterImpl].onConfigure: configuring component.");

    const char8_t* dbName = getConfig().getDbName();
    mDbId = gDbScheduler->getDbId(dbName);
    if(mDbId == DbScheduler::INVALID_DB_ID)
    {
        ERR_LOG("[FifaCupsMasterImpl].onConfigure(): Failed to initialize db");
        return false;
    }

    FifaCupsDb::initialize(mDbId);

    if(!parseConfig())
    {
		ERR_LOG("[FifaCupsMasterImpl].onConfigure(): Failed to parse configuration");
        return false;
    }

	return true;
}
void FifaCupsMasterImpl::onReconfigureBlockable()
{
	onConfigure();
}


bool FifaCupsMasterImpl::onReconfigure()
{
    TRACE_LOG("[FifaCupsMasterImpl].onReconfigure: reconfiguring component.");

    gSelector->scheduleFiberCall<FifaCupsMasterImpl>(this, &FifaCupsMasterImpl::onReconfigureBlockable, "FifaCupsMasterImpl::onReconfigureBlockable");
	return true;
}


bool8_t FifaCupsMasterImpl::parseBool8(const ConfigMap *map, const char8_t *name, bool8_t &result)
{
    bool8_t value = false;
    if (false == map->isDefined(name))
    {
        ERR_LOG("[FifaCupsMasterImpl].parseBool8(): missing configuration parameter <bool> "<<name <<".");
        result = false;
    }
    else
    {
        value = map->getBool(name, false);
    }

    return value;
}


uint8_t FifaCupsMasterImpl::parseUInt8(const ConfigMap *map, const char8_t *name, bool8_t &result)
{
    int8_t value = map->getInt8(name, -1);
    if (value == -1)
    {
        ERR_LOG("[FifaCupsMasterImpl].parseUInt8(): missing configuration parameter <uint8> "<<name<<".");
        result = false;
    }
    return static_cast<uint8_t>(value);
}


int32_t FifaCupsMasterImpl::parseInt32(const ConfigMap *map, const char8_t *name, bool8_t &result)
{
    int32_t value = map->getInt32(name, -1);
    if (value == -1)
    {
        ERR_LOG("[FifaCupsMasterImpl].parseInt32(): missing configuration parameter <uint32> "<<name<<".");
        result = false;
    }
    return value;
}


uint32_t FifaCupsMasterImpl::parseUInt32(const ConfigMap *map, const char8_t *name, bool8_t &result)
{
    int32_t value = map->getInt32(name, -1);
    if (value == -1)
    {
        ERR_LOG("[FifaCupsMasterImpl].parseUInt32(): missing configuration parameter <uint32> "<<name<<".");
        result = false;
    }
    return static_cast<uint32_t>(value);
}


MemberType FifaCupsMasterImpl::parseMemberType(const ConfigMap *map, const char8_t *name, bool8_t &result)
{
    int32_t value = map->getInt32(name, -1);
    if (value == -1)
    {
        ERR_LOG("[FifaCupsMasterImpl].parseMemberType(): missing configuration parameter <MemberType> "<<name<<".");
        result = false;
    }
    return static_cast<MemberType>(value);

}


TournamentRule FifaCupsMasterImpl::parseTournamentRule(const ConfigMap *map, const char8_t *name, bool8_t &result)
{
    int32_t value = map->getInt32(name, -1);
    if (value == -1)
    {
        ERR_LOG("[FifaCupsMasterImpl].parseTournamentRule(): missing configuration parameter <TournamentRule> "<<name<<".");
        result = false;
    }
    return static_cast<TournamentRule>(value);
}


const char8_t* FifaCupsMasterImpl::parseString(const ConfigMap *map, const char8_t *name, bool8_t& result)
{
    const char8_t* strValue = map->getString(name, "");
    if (NULL == strValue)
    {
        ERR_LOG("[FifaCupsMasterImpl].parseString(): missing configuration parameter <string> "<<name<<".");
        result = false;
    }
    return strValue;
}


TimeStamp FifaCupsMasterImpl::parseTimeDuration(int32_t iMinutes, int32_t iHours, int32_t iDays, bool8_t &result)
{
    TimeStamp duration = 0;

    if ((iMinutes < 0) ||
        (iMinutes > 59) ||
        (iHours < 0) ||
        (iHours > 23) ||
        (iDays < 0) ||
        (iDays > 31))
    {
        ERR_LOG("[FifaCupsMasterImpl].parseRolloverData(): invalid parameters for time duration. hours = "<<iHours<<", days = "<<iDays<<" ");
        result = false;
        return duration;
    }

    duration = FifaCupsTimeUtils::getDurationInSeconds(iDays, iHours, iMinutes);

    return duration;
}


Stats::StatPeriodType FifaCupsMasterImpl::convertPeriodType(int32_t iPeriodType, bool8_t& result)
{
    Blaze::Stats::StatPeriodType ePeriodType = Blaze::Stats::STAT_NUM_PERIODS; // using num periods as the error condition

    
    // convert the iPerdiodType bit mask to the StatPeriodType enum
    switch (iPeriodType)
    {
	case 0x1:
		ePeriodType = Blaze::Stats::STAT_PERIOD_ALL_TIME;
		break;
    case 0x2:
        ePeriodType = Blaze::Stats::STAT_PERIOD_MONTHLY;
        break;
    case 0x4:
        ePeriodType = Blaze::Stats::STAT_PERIOD_WEEKLY;
        break;
    case 0x8:
        ePeriodType = Blaze::Stats::STAT_PERIOD_DAILY;
        break;
    default:
        break;
    }
    if (Blaze::Stats::STAT_NUM_PERIODS == ePeriodType)
    {
        // invalid stat period
        ERR_LOG("[FifaCupsMasterImpl].parsePeriodType(): missing or invalid configuration parameter <StatPeriodType> periodType.");
        result = false;
    }

    return ePeriodType;
}

StatMode FifaCupsMasterImpl::parseStatMode(const ConfigMap *map, const char8_t *name, bool8_t &result)
{
	int32_t value = map->getInt32(name, -1);
	if (value == -1)
	{
		ERR_LOG("[FifaCupsMasterImpl].parseStatMode(): missing configuration parameter <StatMode> "<<name<<".");
		result = false;
	}
	return static_cast<StatMode>(value);

}

void FifaCupsMasterImpl::parseCups(const ConfigSequence *sequence, SeasonConfigurationServer::CupList& cupList, bool8_t& result)
{
	int32_t iCupCount = 0;

	while(sequence->hasNext() && result)
	{
		const ConfigMap *map = sequence->nextMap();
		if (NULL == map)
		{
			ERR_LOG("[FifaCupsMasterImpl].parseCups(): missing or invalid cup structure at element "<<iCupCount<<" of 'cups'");
			result = false;
			break;
		}

		Cup *cup = BLAZE_NEW Cup();

		cup->setCupId(parseUInt32(map, "cupId", result));
		cup->setStartDiv(parseUInt8(map, "startDiv", result));
		cup->setEndDiv(parseUInt8(map, "endDiv", result));
		cup->setTournamentRule(parseTournamentRule(map, "tournamentRule", result));

		iCupCount++;

		cupList.push_back(cup);
	}

	return;
}


bool8_t FifaCupsMasterImpl::parseConfig()
{
	bool8_t bResult = true;
	int32_t count = 1;
	FifaCupsDb dbHelper;

	const FifaCupsConfig& config = getConfig();

	if (config.getInstances().size() <= 0)
	{
		ERR_LOG("[FifaCupsMasterImpl].parseConfig(): missing configuration parameter 'instances'");
		bResult = false;
	}
	else
	{
		// establish a database connection
		DbConnPtr dbConnection = gDbScheduler->getConnPtr(mDbId);
		if (dbConnection == NULL)
		{
			ERR_LOG("[FifaCupsMasterImpl].onConfigure() - failed to obtain db connection.");
			bResult = false;
		}

		else
		{

			BlazeRpcError dbError = dbHelper.setDbConnection(dbConnection);

			if (ERR_OK != dbError)
			{
				ERR_LOG("[FifaCupsMasterImpl].parseConfig(): error setting db connection");
				bResult = false;
			}
		}
	}

	if(false != bResult)
	{
		ReplicatedSeasonConfigurationMap* replicatedConfigMap = getReplicatedSeasonConfigurationMap();

		TRACE_LOG("[FifaCupsMasterImpl].parseConfig(): reading instances");

		for (int i=0;  static_cast<uint32_t>(i) < config.getInstances().size(); i++)
		{
			EA::TDF::tdf_ptr<FifaCups::Instances> currentInstance = config.getInstances().at(i);

			SeasonConfigurationServer *seasonConfig = BLAZE_NEW SeasonConfigurationServer();

			seasonConfig->setSeasonId(currentInstance->getId());
			seasonConfig->setMemberType(static_cast<MemberType>(currentInstance->getMemberType()));
			seasonConfig->setLeagueId(currentInstance->getLeagueId());
			seasonConfig->setTournamentId(currentInstance->getTournamentId());
			seasonConfig->setTournamentMode(currentInstance->getTournamentMode());
			seasonConfig->setStatPeriodtype(convertPeriodType(currentInstance->getPeriodType(), bResult));
			seasonConfig->setStatMode(static_cast<StatMode>(currentInstance->getStatMode()));
			seasonConfig->setStartDivision(currentInstance->getStartDivision());
			seasonConfig->setStatCategoryName(currentInstance->getCupStatCategory());
			FifaCups::CupDuration cupDuration = currentInstance->getCupDuration();
			seasonConfig->setCupDuration(parseTimeDuration(cupDuration.getMinutes(), cupDuration.getHours(), cupDuration.getDays(), bResult));
			seasonConfig->setRotatingCupsCount(currentInstance->getRotatingCupsCount());

			if ((currentInstance->getCups().size() <= 0) || (false==bResult))
			{
				ERR_LOG("[FifaCupsMasterImpl].parseConfig(): missing or invalid cups sequence");
				bResult = false;
			}
			else
			{
				//parse cups
				for (int cupIdx = 0; static_cast<uint32_t>(cupIdx) < currentInstance->getCups().size(); cupIdx++)
				{
					TRACE_LOG("[FifaCupsMasterImpl].parseConfig(): reading cup "<< cupIdx <<".");
					Cup* currentCup = currentInstance->getCups().at(cupIdx);
					if (currentCup == NULL || currentCup->getTournamentRule() < FIFACUPS_TOURNAMENTRULE_UNLIMITED
						|| currentCup->getTournamentRule() > FIFACUPS_TOURNAMENTRULE_ONE_ATTEMPT )
					{
						ERR_LOG("[FifaCupsMasterImpl].parseConfig(): invalid TournamentRule at index:"<<cupIdx<<" encountered.");
						bResult = false;
						break;
					}

					seasonConfig->getCups().push_back(currentCup);
				}
			}

			if (false != bResult)
			{
				seasonConfig->getRotationExceptions().clear();

				FifaCupsTimeUtils timeUtil;
				eastl::vector<CupRotationException*> sortingVec;

				for (uint32_t index = 0; index < currentInstance->getRotationExceptions().size(); index++)
				{
					CupRotationException* cException = currentInstance->getRotationExceptions().at(index);
					if (cException == NULL)
					{
						ERR_LOG("[FifaCupsMasterImpl].parseConfig(): invalid cup rotation exception at index:" << index << " encountered.");
						bResult = false;
						break;
					}

					TimeValue exceptionTimeValue;
					exceptionTimeValue.parseTimeAllFormats(cException->getTime());
					cException->setTimeInSeconds(exceptionTimeValue.getSec());
					TRACE_LOG("[FifaCupsMasterImpl].parseConfig(): reading excpetion time value:" << cException->getTimeInSeconds() << " cup: " << cException->getCupId() << ".");

					sortingVec.push_back(cException);
				}

				eastl::sort(sortingVec.begin(), sortingVec.end(), [](auto& i, auto& j) { return i->getTimeInSeconds() < j->getTimeInSeconds(); });

				for (uint32_t k = 0; k < sortingVec.size(); k++)
				{
					seasonConfig->getRotationExceptions().push_back(sortingVec[k]);
				}
			}

			if(false == bResult)
			{
				delete seasonConfig;
				break;
			}

			// insert the season into the database (will fail silently if the season already exists)
			BlazeRpcError dbError = dbHelper.insertNewSeason(seasonConfig->getSeasonId());

			if (ERR_OK != dbError)
			{
				ERR_LOG("[FifaCupsMasterImpl].parseConfig(): error inserting season "<<seasonConfig->getSeasonId()<<" into database");
				bResult = false;
				delete seasonConfig;
				break;
			}
			else
			{
				// replicate the season config
				replicatedConfigMap->insert(seasonConfig->getSeasonId(), *seasonConfig);
				mMetrics.mTotalSeasons.add(1);
			}
			++count;
		}
	}


	if (false == bResult)
	{
		ERR_LOG("[FifaCupsMasterImpl].parseConfig(): check element "<< count - 1<<" of the 'instances' sequence");
	}

	debugPrintConfig();

	return bResult;
}


void FifaCupsMasterImpl::debugPrintConfig()
{
    TRACE_LOG("[FifaCupsMasterImpl].debugPrintConfig: print configuration");

    int32_t iSize = 0;
    char8_t strDuration[16];    // for printing duration

    // print the season configuration
    ReplicatedSeasonConfigurationMap* replicatedConfigMap = getReplicatedSeasonConfigurationMap();
    iSize = replicatedConfigMap->size();
    INFO_LOG("\tNumber of Configured Seasons: "<<iSize<<" ");

    if (iSize > 0)
    {
        int32_t iCount = 1;

        ReplicatedSeasonConfigurationMap::const_iterator itr = replicatedConfigMap->begin();
        ReplicatedSeasonConfigurationMap::const_iterator itr_end = replicatedConfigMap->end();

        for ( ; itr != itr_end; ++itr)
        {
            const SeasonConfigurationServer *season = *itr;
            INFO_LOG("\t  Instance #"<< iCount <<" ");
            INFO_LOG("\t\tId                = "<< season->getSeasonId()<<" ");
            INFO_LOG("\t\tmMemberType       = "<< MemberTypeToString(season->getMemberType())<< " ");
            INFO_LOG("\t\tmLeagueId         = "<< season->getLeagueId()<< " ");
            INFO_LOG("\t\tmTournamentId     = "<< season->getTournamentId() <<" ");
            INFO_LOG("\t\tmStatPeriodtype   = "<< Blaze::Stats::StatPeriodTypeToString(season->getStatPeriodtype())<<" ");
			FifaCupsTimeUtils::getDurationAsString(season->getCupDuration(), strDuration, sizeof(strDuration));
			INFO_LOG("\t\tmCupDuration = "<<strDuration<<" ");
           
            ++iCount;
        }
    }
}

} // FifaCups
} // Blaze
