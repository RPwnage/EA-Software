///////////////////////////////////////////////////////////////////////////////
// ServerUserData.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _SERVERUSERDATA_H_INCLUDED_
#define _SERVERUSERDATA_H_INCLUDED_

namespace Origin
{
	namespace Services
	{
		///
		/// struct ServerUserData
		/// \brief declares interface of all user-retrieved data
		struct ServerUserData
		{
			virtual bool isValid() const = 0;
			virtual void reset() = 0;
			virtual ~ServerUserData() {}
		};

	}
}

#endif