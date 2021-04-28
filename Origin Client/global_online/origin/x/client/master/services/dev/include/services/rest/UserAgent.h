///////////////////////////////////////////////////////////////////////////////
// UserAgent.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _USER_AGENT_HEADER_INCLUDED_
#define _USER_AGENT_HEADER_INCLUDED_

#include "version/version.h"
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
	namespace Services
	{
		class ORIGIN_PLUGIN_API UserAgent
		{
		public:
			///
			/// Get User agent header as a QString.
			///
			static QString headerAsQString()
			{
				return mUserAgentHeader;
			}
			///
			/// Get user agent header as a Byte Array
			///
			static QByteArray headerAsBA()
			{
				return mUserAgentHeader.toLatin1();
			}
			///
			/// Get user agent title as a Byte Array
			///
			static QByteArray title() 
			{ 
				return mUserAgentTitle.toLatin1();
			}
			///
			/// Get user agent header as a std::string
			///
			static std::string headerAsStdStr()
			{
				return mUserAgentHeader.toStdString();
			}
			///
			/// Get user agent header as a std::wstring
			///
			static std::wstring headerAsStdWStr()
			{
				return mUserAgentHeader.toStdWString();
			}

			///
			/// Get user agent title as a Byte Array
			///
			static std::string titleAsStdStr() 
			{ 
				return mUserAgentTitle.toStdString();
			}
			///
			/// returns the whole User Header
			///
			static std::string userAgentHeaderAsStdStr() 
			{ 
				return mUserAgentTitle.toStdString()
					+ std::string(": ") 
					+ mUserAgentHeader.toStdString();
			}

		private:
			///
			/// User agent header container
			///
			static QString mUserAgentHeader;
			///
			/// User agent title
			///
			static QString mUserAgentTitle;
			///
			/// disable instantiation, copy CTOR and operator assignment
			///
			UserAgent();
			UserAgent(const UserAgent&);
			UserAgent operator=(const UserAgent&);

		};

		ORIGIN_PLUGIN_API QString userAgentHeader();        
		ORIGIN_PLUGIN_API QString userAgentHeaderNoMozilla();
    }
}

#endif
