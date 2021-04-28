/*************************************************************************************************/
/*!
    \file   gamereportprocessor.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "gamereportprocessor.h"
#include "gamereportingslaveimpl.h"

namespace Blaze
{
namespace GameReporting
{

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  constructor
GameReportProcessor::GameReportProcessor(GameReportingSlaveImpl& component)
:   mComponent(component),
    mRefCount(0)
{
}

}   // namespace GameReporting
}   // namespace Blaze
