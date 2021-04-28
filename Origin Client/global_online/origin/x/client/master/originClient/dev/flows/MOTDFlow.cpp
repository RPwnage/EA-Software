///////////////////////////////////////////////////////////////////////////////
// MOTDFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "flows/MOTDFlow.h"
#include "MOTDViewController.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
	namespace Client
	{
		MOTDFlow::MOTDFlow()
		{

		}

		void MOTDFlow::start()
		{
			const QString motdURL = Origin::Services::readSetting(Origin::Services::SETTING_MotdURL);
			mMOTDViewController.reset(new MOTDViewController(motdURL));
		}

        void MOTDFlow::raise()
        {
            if (mMOTDViewController.isNull() == false)
            {
                mMOTDViewController->raise();
            }
        }
	}
}