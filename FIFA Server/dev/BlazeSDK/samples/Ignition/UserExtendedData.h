
#ifndef USEREXTENDEDDATA_H
#define USEREXTENDEDDATA_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/usermanager/usermanager.h"

namespace Ignition
{

class UserExtendedData :
    public LocalUserUiBuilder
{
    public:
        

        UserExtendedData(uint32_t userIndex);
        virtual ~UserExtendedData();

        virtual void onAuthenticated();
        virtual void onDeAuthenticated();

    private:

        PYRO_ACTION(UserExtendedData, GetUserExtendedData);
        PYRO_ACTION(UserExtendedData, GetUserExtendedDataForUser);
        PYRO_ACTION(UserExtendedData, SetCrossplayEnabled);
        PYRO_ACTION(UserExtendedData, AddClientAttribute);
        PYRO_ACTION(UserExtendedData, RemoveClientAttribute);
        PYRO_ACTION(UserExtendedData, SetUserInfoAttribute);
        PYRO_ACTION(UserExtendedData, SetCrossPlatformOptIn);
        PYRO_ACTION(UserExtendedData, LookupUsersByBlazeId);
        PYRO_ACTION(UserExtendedData, LookupUsersByPersonaNames);
        PYRO_ACTION(UserExtendedData, LookupUsersByPersonaNamePrefix);
        PYRO_ACTION(UserExtendedData, LookupUsersByNucleusAccountId);
        PYRO_ACTION(UserExtendedData, LookupUsersByOriginPersonaId);
        PYRO_ACTION(UserExtendedData, LookupUsersByOriginPersonaName);
        PYRO_ACTION(UserExtendedData, LookupUsersBy1stPartyId);
        PYRO_ACTION(UserExtendedData, LookupUsersByExternalId);

        void getUserExtendedDataCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::UserManager::User* user);
        void updateExtendedDataAttributeCb(Blaze::BlazeError error, Blaze::JobId jobId);
        void removeExtendedDataAttributeCb(Blaze::BlazeError error, Blaze::JobId jobId);
        void setUserInfoAttributeCb(Blaze::BlazeError error, Blaze::JobId jobId);
        void setUserCrossPlatformOptInCb(Blaze::BlazeError error, Blaze::JobId jobId);
        void lookupUsersCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::UserManager::UserManager::UserVector& users);

        void setupLookupUsersCrossPlatformOptions(Pyro::UiNodeParameterStruct& parameters, const char8_t* clientPlatformOnlyIdentifier);
        void setupLookupUsersCrossPlatformCommon(Blaze::CrossPlatformLookupType lookupType, Pyro::UiNodeParameterStruct& parameters);
        void addParameterArrayForPlatform(Blaze::ClientPlatformType platform, Pyro::UiNodeParameterStruct& parameters);

        void doLookupUsersCrossPlatform(Blaze::CrossPlatformLookupType lookupType, Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);
        void getPlatformInfoListForPlatform(Blaze::ClientPlatformType platform, Blaze::PlatformInfoList& platformList, Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);

        void setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, const char8_t* rowValue);
        void setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, bool rowValue);
        void setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, uint32_t locale);
        void setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, int64_t rowValue);
        void setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, uint64_t rowValue);
        void setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, const Blaze::EaIdentifiers& eaIds);
        void setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, const Blaze::ExternalIdentifiers& externalIds);

#ifdef BLAZE_USER_PROFILE_SUPPORT
        PYRO_ACTION(UserExtendedData, GetUserProfiles);
        void getUserProfileCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::UserManager::UserProfilePtr userProfile);
        void getUserProfilesCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::UserManager::UserManager::UserProfileVector& userProfiles);
#endif
};

}

#endif //USEREXTENDEDDATA_H
