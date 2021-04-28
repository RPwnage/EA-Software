/*************************************************************************************************/
/*!
    \file   fifacupsmasterimpl.h

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/fifacupsmasterimpl.h#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
        (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFACUPS_MASTERIMPL_H
#define BLAZE_FIFACUPS_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "fifacups/rpc/fifacupsmaster_stub.h"
#include "fifacups/tdf/fifacupstypes_server.h"
#include "fifacups/mastermetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConfigMap;
class ConfigSequence;

namespace FifaCups
{

class FifaCupsMasterImpl : public FifaCupsMasterStub
{
public:
	FifaCupsMasterImpl();
    ~FifaCupsMasterImpl();

    // end of season processing
    virtual StartEndOfSeasonProcessingError::Error processStartEndOfSeasonProcessing(const EndOfSeasonRolloverData &rolloverData, const Message* message);
    virtual FinishedEndOfSeasonProcessingError::Error processFinishedEndOfSeasonProcessing(const EndOfSeasonRolloverData &rolloverData, const Message* message);

private:
    virtual uint16_t  		getDbSchemaVersion() const	{ return 2; }

    //! Called periodically to collect status and statistics about this component that can be used to determine what future improvements should be made.
    virtual void getStatusInfo(ComponentStatus& status) const;

    //! Called when the component should grab configuration for the very first time, during startup.
    virtual bool onConfigure();

	//! Called when the component should update to the latest configuration, for runtime patching.
	virtual bool onReconfigure();

    //! Called when the component should update to the latest configuration on a blockable fiber , for runtime patching.
    void onReconfigureBlockable();

    bool8_t parseBool8(const ConfigMap *map, const char8_t *name, bool8_t& result);
    uint8_t parseUInt8(const ConfigMap *map, const char8_t *name, bool8_t& result);
    int32_t parseInt32(const ConfigMap *map, const char8_t *name, bool8_t& result);
    uint32_t parseUInt32(const ConfigMap *map, const char8_t *name, bool8_t& result);
    const char8_t* parseString(const ConfigMap *map, const char8_t *name, bool8_t& result);
    MemberType parseMemberType(const ConfigMap *map, const char8_t *name, bool8_t& result);
    TournamentRule parseTournamentRule(const ConfigMap *map, const char8_t *name, bool8_t& result);
	TimeStamp parseTimeDuration(int32_t iMinutes, int32_t iHours, int32_t iDays, bool8_t &result);
	Stats::StatPeriodType convertPeriodType(int32_t iPeriodType, bool8_t& result);
	StatMode parseStatMode(const ConfigMap *map, const char8_t *name, bool8_t& result);
	void parseCups(const ConfigSequence *sequence, SeasonConfigurationServer::CupList& cupList, bool8_t& result);

    bool8_t parseConfig();
	bool8_t reparseConfig();
    void debugPrintConfig();    

    uint32_t            mDbId;

    // keeps track in progress end of season processing.
    bool8_t             mProcessingEndOfSeasonDaily;
    bool8_t             mProcessingEndOfSeasonWeekly;
    bool8_t             mProcessingEndOfSeasonMonthly;

    //! Health monitoring statistics.
    MasterMetrics       mMetrics;

    //! End of season processing start time
    Blaze::TimeValue    mEndOfSeasonProcStartTime;
};

} // FifaCups
} // Blaze

#endif  // BLAZE_FIFACUPS_MASTERIMPL_H
