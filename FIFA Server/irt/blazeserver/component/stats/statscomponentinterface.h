/*************************************************************************************************/
/*!
    \file   statscomponentinterface.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSCOMPONENTINTERFACE_H
#define BLAZE_STATS_STATSCOMPONENTINTERFACE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{
// Forward declarations
class StatsConfigData;

class StatsComponentInterface
{
public:
    virtual ~StatsComponentInterface() {}

    virtual const StatsConfigData* getConfigData() const = 0;
    virtual uint32_t getDbId() const = 0;

    virtual int32_t getPeriodId(int32_t periodType) = 0;
    virtual int32_t getRetention(int32_t periodType) = 0;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_STATSCOMPONENTINTERFACE_H
