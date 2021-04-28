//
//  UserAgent.cpp
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#include "UserAgent.h"

namespace Origin 
{
    namespace Services 
    {
  
        QString UserAgent::mUserAgentHeader = userAgentHeader();
        QString UserAgent::mUserAgentTitle = "User-Agent";

		QString userAgentHeader()
		{
			static QString uAgentHeader 
				= "Mozilla/5.0 " 
				+ QString("EA Download Manager ") 
				+ QString(EBISU_PRODUCT_NAME_STR) 
				+ "/" + QString(EALS_VERSION_P_DELIMITED);
			return uAgentHeader;
		}

		QString userAgentHeaderNoMozilla()
		{
			static QString uAgentHeader 
				= QString("EA Download Manager ");

			return uAgentHeader;
		}
    }
}
