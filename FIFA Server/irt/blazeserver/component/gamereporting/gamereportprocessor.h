/*************************************************************************************************/
/*!
    \file   gamereportprocessor.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTPROCESSOR_H
#define BLAZE_GAMEREPORTPROCESSOR_H

/*** Include files *******************************************************************************/
#include "gamereporting/tdf/gamereporting.h"
#include "processedgamereport.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

class GameReportingSlaveImpl;
/*!
    class GameReportProcessor

    Teams that wish to modify report validation and processing on the slave can implement their
    own GameReportProcessor based class.   It's suggested to implement a reporting pipeline
    per GameType.  Often processing is customizable by GameType, so separate pipeline classes will
    help to prevent switch/case statements based on GameReportName within a pipeline's implementation.

    When Game Reporting creates a pipeline object, it passes along the GameType specified in GameReport
    and as defined in gamereporting.cfg.    

    Teams must implement a named constructor in the class with the signature
        static GameReportProcessor* create(GameReportingSlaveImpl& component);

    The class must then register itself with the slave component.
    For example, please see component/gamereporting/basicgamereportprocessor.cpp
*/
class GameReportProcessor
{
    NON_COPYABLE(GameReportProcessor);

public:
    virtual ~GameReportProcessor() {}

    /*! ****************************************************************************/
    /*! \brief Implementation performs simple validation and if necessary modifies
            the game report.
 
        On success report may be submitted for collation or direct to processing
        for offline or trusted reports.   Behavior depends on the calling RPC.

        \param report Incoming game report from submit request.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError validate(GameReport& report) const = 0;

    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.
 
        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds) = 0;

	// FIFA SPECIFIC CODE BEGIN
	/*! ****************************************************************************/
	/*! \brief Called after process() has been called

		\param processedReport Contains the final game report and information used by game history.
		\return ERR_OK on success.  GameReporting specific error on failure.
	********************************************************************************/
	virtual BlazeRpcError postProcess(ProcessedGameReport& processedReport, bool reportWasSuccessful)
	{
		return ERR_OK;
	}
	// FIFA SPECIFIC CODE END

protected:
    GameReportProcessor(GameReportingSlaveImpl& component);

    GameReportingSlaveImpl& getComponent()
    {
        return mComponent;
    }

protected:
    GameReportingSlaveImpl& mComponent;

private:
    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(GameReportProcessor* ptr);
    friend void intrusive_ptr_release(GameReportProcessor* ptr);
};

typedef eastl::intrusive_ptr<GameReportProcessor> GameReportProcessorPtr;
inline void intrusive_ptr_add_ref(GameReportProcessor* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(GameReportProcessor* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

}   // namespace GameReporting
}   // namespace Blaze

#endif // BLAZE_GAMEREPORTPROCESSOR_H
