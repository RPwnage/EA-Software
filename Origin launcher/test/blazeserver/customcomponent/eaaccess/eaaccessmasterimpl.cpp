/*************************************************************************************************/
/*!
    \file   eaaccessmasterimpl.cpp

    $Header: //gosdev/games/FIFA/2015/Gen4/DEV/blazeserver/13.x/customcomponent/eaaccess/eaaccessmasterimpl.cpp#2 $

    \attention
        (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EaAccessMasterImpl

    EaAccess Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "eaaccessmasterimpl.h"

// global includes
#include "framework/controller/controller.h"
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicator.h"
#include "framework/util/random.h"

// easfc includes
#include "xmshd/tdf/xmshdtypes.h"
#include "xmshd/tdf/xmshdtypes_server.h"

namespace Blaze
{
	namespace EaAccess
	{

		// static
		EaAccessMaster* EaAccessMaster::createImpl()
		{
			return BLAZE_NEW EaAccessMasterImpl();
		}

		/*** Public Methods ******************************************************************************/

		/*************************************************************************************************/
		/*!
			\brief  constructor
		*/
		/*************************************************************************************************/
		EaAccessMasterImpl::EaAccessMasterImpl() : 
			mMetrics()
		{
		}


		/*************************************************************************************************/
		/*!
			\brief  destructor
		*/
		/*************************************************************************************************/    
		EaAccessMasterImpl::~EaAccessMasterImpl()
		{
		}

		/*************************************************************************************************/
		/*!
			\brief  getStatusInfo

			Called periodically to collect status and statistics about this component that can be used 
			to determine what future improvements should be made.

		*/
		/*************************************************************************************************/
		void EaAccessMasterImpl::getStatusInfo(ComponentStatus& status) const
		{
			TRACE_LOG("[EaAccessMasterImpl].getStatusInfo");

			mMetrics.report(&status);
		}


		bool EaAccessMasterImpl::onConfigure()
		{
			TRACE_LOG("[EaAccessMasterImpl].onConfigure: configuring component.");

			return true;
		}


		bool EaAccessMasterImpl::onReconfigure()
		{
			TRACE_LOG("[EaAccessMasterImpl].onReconfigure: reconfiguring component.");

			return onConfigure();
		}

	} // EaAccess
} // Blaze
