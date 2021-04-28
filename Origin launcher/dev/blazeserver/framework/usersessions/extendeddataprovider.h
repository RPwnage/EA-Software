/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EXTENDED_DATA_PROVIDER_H
#define BLAZE_EXTENDED_DATA_PROVIDER_H

/*** Include files *******************************************************************************/
#include "framework/tdf/userextendeddatatypes_server.h"

namespace Blaze
{

/*! *****************************************************************************/
/*! \class 
    \brief Interface to be implemented by anyone that adds data to the user session
            extended data
*********************************************************************************/
class ExtendedDataProvider
{
public:

    virtual ~ExtendedDataProvider() {} 

    /*! ***********************************************************************/
    /*! \brief The custom data for a user is being loaded
    
        \param[in] data - info about the user to load extended data for
        \param[in] extendedData - the user session extended data for the user
        \return Inheritors are responsible for returning an error if one occurs.
                When any error is returned, no other ExtendedDataProviders are called.
    ***************************************************************************/
    virtual BlazeRpcError onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) = 0;

    /*! ***********************************************************************/
    /*! \brief looks up extended data that isn't replicated and adds it to
            an existing UserSessionExtendedData object.

        \param[in] data - info about the user to load extended data for
        \param[in] extendedData - the user session extended data for the user
        \return Inheritors are responsible for returning an error if one occurs.
                When any error is returned, no other ExtendedDataProviders are called.
    ***************************************************************************/
    virtual BlazeRpcError onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) = 0;
};

}

#endif //BLAZE_EXTENDED_DATA_PROVIDER_H