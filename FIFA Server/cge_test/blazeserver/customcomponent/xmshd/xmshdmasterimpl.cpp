/*************************************************************************************************/
/*!
    \file   xmshdmasterimpl.cpp

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/xmshdmasterimpl.cpp#2 $

    \attention
        (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class XmsHdMasterImpl

    XmsHd Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "xmshdmasterimpl.h"

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
	namespace XmsHd
	{

		// static
		XmsHdMaster* XmsHdMaster::createImpl()
		{
			return BLAZE_NEW XmsHdMasterImpl();
		}

		/*** Public Methods ******************************************************************************/

		/*************************************************************************************************/
		/*!
			\brief  constructor
		*/
		/*************************************************************************************************/
		XmsHdMasterImpl::XmsHdMasterImpl() : 
			mMetrics()
		{
		}


		/*************************************************************************************************/
		/*!
			\brief  destructor
		*/
		/*************************************************************************************************/    
		XmsHdMasterImpl::~XmsHdMasterImpl()
		{
		}

		/*************************************************************************************************/
		/*!
			\brief  getStatusInfo

			Called periodically to collect status and statistics about this component that can be used 
			to determine what future improvements should be made.

		*/
		/*************************************************************************************************/
		void XmsHdMasterImpl::getStatusInfo(ComponentStatus& status) const
		{
			TRACE_LOG("[XmsHdMasterImpl].getStatusInfo");

			mMetrics.report(&status);
		}


		bool XmsHdMasterImpl::onConfigure()
		{
			TRACE_LOG("[XmsHdMasterImpl].onConfigure: configuring component.");

			return true;
		}


		bool XmsHdMasterImpl::onReconfigure()
		{
			TRACE_LOG("[XmsHdMasterImpl].onReconfigure: reconfiguring component.");

			return onConfigure();
		}

	} // XmsHd
} // Blaze
