/*************************************************************************************************/
/*!
    \file   gameeventsprocessor.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEEVENTSPROCESSOR_H
#define BLAZE_GAMEEVENTSPROCESSOR_H

/*** Include files *******************************************************************************/
#include "gamereporting/tdf/gamereporting.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

class GameReportingSlaveImpl;

/*!
    class GameEventsProcessor

    Teams that wish to use game events must implement their own GameEventsProcessor.  On calling submitGameEvents,
    GameReporting uses the GameEventsProcessor createInterface factory to instantiate an event processor.  The factory 
    method takes a string to differentiate one processor from another.  The current implementation will return
    a NullGameEventsProcessor (stubbed-out processing) instance.

    Game events can be used as an alternative to using game reports depending on a title's requirements.  The event submitter
    is responsible for collecting its own events.  Because game events aren't tied to GameManager games, there is no collation 
    of event streams from different users.  One example could be a dedicated server title where the server submits events for all players.    

    It's possible to use game events for online gameplay provided the title takes into account the restrictions noted above.  
    Also there is no game history saved.

    Because of these restrictions events are suggested only for offline gameplay as a possible alternative to
    offline game reports.   Events are then collected on the client during a solo game, and explicitly submitted via submitGameEvents 
    when the client logs in.
*/
class GameEventsProcessor
{
    NON_COPYABLE(GameEventsProcessor);

public:
    static GameEventsProcessor* createInterface(const char8_t* gameEventProcessorName, GameReportingSlaveImpl& component);

public:
    virtual ~GameEventsProcessor() {}  
 
    /*! ****************************************************************************/
    /*! \brief Called for each event submitted in the submitGameEvents call.
 
        \param gameEvent Event containing type and custom data.
        \param blazeUserId The Blaze ID submitting the event.
        \return ERR_OK on success, or a game reporting error code on failure.
    ********************************************************************************/
    virtual BlazeRpcError process(const GameEvent& gameEvent, BlazeId blazeId) = 0;

protected:
    GameEventsProcessor(GameReportingSlaveImpl& component);

    GameReportingSlaveImpl& getComponent() {
        return mComponent;
    }

protected:
    GameReportingSlaveImpl& mComponent;

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_GAMEEVENTSPROCESSOR_H
