/*************************************************************************************************/
/*!
    \file   ganmeeventsprocessor.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "gameeventsprocessor.h"
#include "gamereportingslaveimpl.h"


namespace Blaze
{

namespace GameReporting
{

//
//  class NullGameEventsProcessor
//      Null reference events processor for submitGameEvents
class NullGameEventsProcessor : public GameEventsProcessor
{
public:
    NullGameEventsProcessor(GameReportingSlaveImpl& component) : GameEventsProcessor(component) 
    {
    }

    BlazeRpcError process(const GameEvent& gameEvent, BlazeId blazeId) override {        
        return ERR_OK;
    }
};


//
//  class GameEventsProcessor factory
//      Teams can modify this static function if implementing their own custom event pipeline.
//      See header for more details.
//

GameEventsProcessor* GameEventsProcessor::createInterface(const char8_t* gameEventProcessorName, GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED(gameEventProcessorName) NullGameEventsProcessor(component);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  constructor
GameEventsProcessor::GameEventsProcessor(GameReportingSlaveImpl& component) :
    mComponent(component)
{
}


}   // namespace GameReporting

}   // namespace Blaze
