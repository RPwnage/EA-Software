/*************************************************************************************************/
/*!
    \file   easfcmasterimpl.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/easfcmasterimpl.cpp#2 $

    \attention
        (c) Electronic Arts 2012. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EasfcMasterImpl

    Easfc Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "easfcmasterimpl.h"

// global includes
#include "framework/controller/controller.h"
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicator.h"
#include "framework/util/random.h"

// easfc includes
#include "easfc/tdf/easfctypes.h"
#include "easfc/tdf/easfctypes_server.h"

namespace Blaze
{
	namespace EASFC
	{

		// static
		EasfcMaster* EasfcMaster::createImpl()
		{
			return BLAZE_NEW EasfcMasterImpl();
		}

		/*** Public Methods ******************************************************************************/

		/*************************************************************************************************/
		/*!
			\brief  constructor
		*/
		/*************************************************************************************************/
		EasfcMasterImpl::EasfcMasterImpl() : 
			mMetrics()
		{
		}


		/*************************************************************************************************/
		/*!
			\brief  destructor
		*/
		/*************************************************************************************************/    
		EasfcMasterImpl::~EasfcMasterImpl()
		{
		}

		/*************************************************************************************************/
		/*!
			\brief  getStatusInfo

			Called periodically to collect status and statistics about this component that can be used 
			to determine what future improvements should be made.

		*/
		/*************************************************************************************************/
		void EasfcMasterImpl::getStatusInfo(ComponentStatus& status) const
		{
			TRACE_LOG("[EasfcMasterImpl].getStatusInfo");

			mMetrics.report(&status);
		}


		bool EasfcMasterImpl::onConfigure()
		{
			TRACE_LOG("[EasfcMasterImpl].onConfigure: configuring component.");

			return true;
		}


		bool EasfcMasterImpl::onReconfigure()
		{
			TRACE_LOG("[EasfcMasterImpl].onReconfigure: reconfiguring component.");

			return onConfigure();
		}

	} // EASFC
} // Blaze
