#ifndef IGNITION_H
#define IGNITION_H

#include "EABase/eabase.h"

#include "PyroSDK/pyrosdk.h"

#include "Ignition/IgnitionTypes.h"
#include "Ignition/IgnitionDefines.h"
#include "Ignition/Transmission.h"

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"

#include "eadp/foundation/Hub.h"

namespace Ignition
{

extern Blaze::BlazeHub *gBlazeHub;
extern Blaze::BlazeNetworkAdapter::ConnApiAdapter *gConnApiAdapter;
extern Transmission2 gTransmission2;

extern eadp::foundation::SharedPtr<eadp::foundation::IHub> gEadpHub;

void initializePlatform();
void tickPlatform();
void finalizePlatform();

INTRUSIVE_PTR(BlazeHubUiDriver);
INTRUSIVE_PTR(LocalUserUiDriver);

INTRUSIVE_PTR(BlazeInitialization);
INTRUSIVE_PTR(Connection);
INTRUSIVE_PTR(Example);
INTRUSIVE_PTR(ByteVault);
INTRUSIVE_PTR(Census);
INTRUSIVE_PTR(GameManagement);
INTRUSIVE_PTR(Stats);
INTRUSIVE_PTR(Leaderboards);
#if !defined(EA_PLATFORM_NX)
INTRUSIVE_PTR(VoipConfig);
#endif
INTRUSIVE_PTR(DirtySockDev);
INTRUSIVE_PTR(Clubs);
INTRUSIVE_PTR(LocalUserConfig);

extern BlazeHubUiDriver *gBlazeHubUiDriver;

class BlazeHubUiBuilder
    : public Pyro::UiBuilder
{
    friend class BlazeHubUiDriver;

    public:
        virtual ~BlazeHubUiBuilder() { }

        BlazeHubUiDriver *getUiDriverMaster() { return (BlazeHubUiDriver *)Pyro::UiBuilder::getUiDriverMaster(); }

    protected:
        BlazeHubUiBuilder(const char8_t *id)
            : Pyro::UiBuilder(id)
        {
        }
};

class BlazeHubUiDriver
    : public Pyro::UiDriverMaster,
      public Blaze::BlazeStateEventHandler,
      public Blaze::UserManager::PrimaryLocalUserListener
{
    friend class BlazeInitialization;

    public:
        BlazeHubUiDriver();
        virtual ~BlazeHubUiDriver();

        BlazeInitialization &getBlazeInitialization()   { return *mBlazeInitialization; }
        Connection &getConnection()                     { return *mConnection; }
        Example &getExample()                           { return *mExample; }
        Census &getCensus()                             { return *mCensus; }
        GameManagement &getGameManagement()             { return *mGameManagement; }
        Stats &getStats()                               { return *mStats; }
        Leaderboards &getLeaderboards()                 { return *mLeaderboards; }
#if !defined(EA_PLATFORM_NX)
        VoipConfig &getVoipConfig()                     { return *mVoipConfig; }
#endif
        DirtySockDev &getDirtySockDev()                 { return *mDirtySockDev; }
        Clubs &getClubs()                               { return *mClubs; }
        LocalUserConfig &getLocalUserConfig()           { return *mLocalUserConfig; }

        void runSendMessageToGroup(uint32_t fromUserIndex, const Blaze::UserGroup *userGroup, const char8_t *text);

        void reportBlazeError(Blaze::BlazeError blazeError, const char8_t *filename, const char8_t *functionName, int32_t lineNumber);

        class BlazeHubConnectionListener
        {
            public:
                virtual ~BlazeHubConnectionListener()
                {
                    gBlazeHubUiDriver->removeBlazeHubConnectionListener(this);
                }

                virtual void onConnected() { }
                virtual void onDisconnected() { }

            protected:
                BlazeHubConnectionListener()
                {
                    gBlazeHubUiDriver->addBlazeHubConnectionListener(this, false);
                }
        };

        void addBlazeHubConnectionListener(BlazeHubConnectionListener *listener, bool callDisconnected);
        void removeBlazeHubConnectionListener(BlazeHubConnectionListener *listener);

        class PrimaryLocalUserAuthenticationListener
        {
            public:
                virtual ~PrimaryLocalUserAuthenticationListener()
                {
                    gBlazeHubUiDriver->removePrimaryLocalUserAuthenticationListener(this);
                }

                virtual void onAuthenticatedPrimaryUser() { }
                virtual void onDeAuthenticatedPrimaryUser() { }

            protected:
                PrimaryLocalUserAuthenticationListener()
                {
                    gBlazeHubUiDriver->addPrimaryLocalUserAuthenticationListener(this, false);
                }
        };

        void addPrimaryLocalUserAuthenticationListener(PrimaryLocalUserAuthenticationListener *listener, bool callDeAuthenticated);
        void removePrimaryLocalUserAuthenticationListener(PrimaryLocalUserAuthenticationListener *listener);

        class LocalUserAuthenticationListener
        {
            public:
                virtual ~LocalUserAuthenticationListener()
                {
                    gBlazeHubUiDriver->removeLocalUserAuthenticationListener(mUserIndex, this);
                }

                uint32_t getUserIndex() { return mUserIndex; }

                virtual void onAuthenticated() { }
                virtual void onDeAuthenticated() { }
                virtual void onForcedDeAuthenticated(Blaze::UserSessionForcedLogoutType forcedLogoutType) {}

            protected:
                LocalUserAuthenticationListener(uint32_t userIndex)
                    : mUserIndex(userIndex)
                {
                    gBlazeHubUiDriver->addLocalUserAuthenticationListener(mUserIndex, this, false);
                }

            private:
                uint32_t mUserIndex;
        };

        void addLocalUserAuthenticationListener(uint32_t userIndex, LocalUserAuthenticationListener *listener, bool callDeAuthenticated);
        void removeLocalUserAuthenticationListener(uint32_t userIndex, LocalUserAuthenticationListener *listener);

        class PrimaryLocalUserChangedListener
        {
        public:
            virtual ~PrimaryLocalUserChangedListener()
            {
                gBlazeHubUiDriver->removePrimaryLocalUserChangedListener(this);
            }

            virtual void onPrimaryLocalUserChanged(uint32_t userIndex) { }

        protected:
            PrimaryLocalUserChangedListener()
            {
                gBlazeHubUiDriver->addPrimaryLocalUserChangedListener(this, false);
            }

        };

        void addPrimaryLocalUserChangedListener(PrimaryLocalUserChangedListener *listener, bool callDeAuthenticated);
        void removePrimaryLocalUserChangedListener(PrimaryLocalUserChangedListener *listener);

    protected:
        virtual void onInitialize();
        virtual void onFinalize();

        virtual void onIdle();

        virtual void onConnected();
        virtual void onDisconnected(Blaze::BlazeError errorCode);
        virtual void onAuthenticated(uint32_t userIndex);
        virtual void onDeAuthenticated(uint32_t userIndex);
        virtual void onForcedDeAuthenticated(uint32_t userIndex, Blaze::UserSessionForcedLogoutType forcedLogoutType);

        virtual void onPrimaryLocalUserChanged(uint32_t primaryUserIndex);
        virtual void onPrimaryLocalUserAuthenticated(uint32_t primaryUserIndex);
        virtual void onPrimaryLocalUserDeAuthenticated(uint32_t primaryUserIndex);

    private:
        void onBlazeHubInitialized();
        void onBlazeHubFinalized();

        void SendMessageCb(const Blaze::Messaging::SendMessageResponse *sendMessageResponse, Blaze::BlazeError blazeError, Blaze::JobId jobId);

    private:
        BlazeInitializationPtr mBlazeInitialization;
        ConnectionPtr mConnection;
        ExamplePtr mExample;
        CensusPtr mCensus;
        GameManagementPtr mGameManagement;
        StatsPtr mStats;
        LeaderboardsPtr mLeaderboards;
#if !defined(EA_PLATFORM_NX)
        VoipConfigPtr mVoipConfig;
#endif
        DirtySockDevPtr mDirtySockDev;
        ClubsPtr mClubs;
        LocalUserConfigPtr mLocalUserConfig;

        typedef Blaze::Dispatcher<BlazeHubConnectionListener, 32> BlazeHubConnectionDispatcher;
        BlazeHubConnectionDispatcher mBlazeHubConnectionDispatcher;

        typedef Blaze::Dispatcher<PrimaryLocalUserAuthenticationListener, 32> PrimaryLocalUserAuthenticationDispatcher;
        PrimaryLocalUserAuthenticationDispatcher mPrimaryLocalUserAuthenticationDispatcher;

        typedef Blaze::Dispatcher<LocalUserAuthenticationListener, 32> LocalUserAuthenticationDispatcher;
        LocalUserAuthenticationDispatcher *mLocalUserAuthenticationDispatchers;

        typedef Blaze::Dispatcher<PrimaryLocalUserChangedListener, 32> PrimaryLocalUserChangedDispatcher;
        PrimaryLocalUserChangedDispatcher mPrimaryLocalUserChangedDispatchers;
        

        template <class T>
        void addIgnitionUiBuilder(T &member)
        {
            member = new typename T::element_type();
            getUiBuilders().add(member);
        }

        template <class T>
        void removeIgnitionUiBuilder(T &member)
        {
            getUiBuilders().remove(member);
            member = nullptr;
        }
};

class LocalUserUiBuilder
    : public BlazeHubUiBuilder,
      public BlazeHubUiDriver::LocalUserAuthenticationListener
{
    public:
        virtual ~LocalUserUiBuilder() { }

        virtual void onAuthenticated() { }
        virtual void onDeAuthenticated() { }

    protected:
        LocalUserUiBuilder(const char8_t *id, uint32_t userIndex)
            : BlazeHubUiBuilder(id),
              BlazeHubUiDriver::LocalUserAuthenticationListener(userIndex)
        {
        }
};

INTRUSIVE_PTR(LoginAndAccountCreation);
INTRUSIVE_PTR(UserManager);
INTRUSIVE_PTR(AssociationLists);
INTRUSIVE_PTR(Messaging);
INTRUSIVE_PTR(Achievements);
INTRUSIVE_PTR(Friends);
INTRUSIVE_PTR(UserExtendedData);
INTRUSIVE_PTR(PlatformFeatures);

class LocalUserUiDriver
    : public Pyro::UiDriverSlave
{
    friend class BlazeHubUiDriver;

    public:
        LocalUserUiDriver(uint32_t userIndex);
        virtual ~LocalUserUiDriver();

        LoginAndAccountCreation &getLoginAndAccountCreation() { return *mLoginAndAccountCreation; }
        UserManager &getUserManager() { return *mUserManager; }
        AssociationLists &getAssociationLists() { return *mAssociationLists; }
        Messaging &getMessaging() { return *mMessaging; }
        Achievements &getAchievements() { return *mAchievements; }
        Friends &getFriends() { return *mFriends; }
        UserExtendedData &getUserExtendedData() { return *mUserExtendedData; }
        PlatformFeatures &getPlatformFeatures() { return *mPlatformFeatures; }
        ByteVault& getByteVault() { return *mByteVault; }

    private:
        uint32_t mUserIndex;

        LoginAndAccountCreationPtr mLoginAndAccountCreation;
        UserManagerPtr mUserManager;
        AssociationListsPtr mAssociationLists;
        MessagingPtr mMessaging;
        AchievementsPtr mAchievements;
        FriendsPtr mFriends;
        UserExtendedDataPtr mUserExtendedData;
        PlatformFeaturesPtr mPlatformFeatures;
        ByteVaultPtr mByteVault;

        template <class T>
        void addLocalUserUiBuilder(T &member)
        {
            member = new typename T::element_type(mUserIndex);
            getUiBuilders().add(member);
        }

        template <class T>
        void removeLocalUserUiBuilder(T &member)
        {
            getUiBuilders().remove(member);
            member = nullptr;
        }
};

class LocalUserActionGroup :
    public Pyro::UiNodeActionGroup,
    public BlazeHubUiDriver::LocalUserAuthenticationListener
{
    public:
        virtual ~LocalUserActionGroup() { }

        virtual void onAuthenticated()
        {
            Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
            setText_va("%s%s", localUser->getName(), (localUser->getUserSessionType() == Blaze::USER_SESSION_GUEST ? " (Guest)" : ""));
        }

        virtual void onDeAuthenticated()
        {
            setText("No User");
        }

    protected:
        LocalUserActionGroup(const char8_t *id, uint32_t userIndex)
            : Pyro::UiNodeActionGroup(id),
              BlazeHubUiDriver::LocalUserAuthenticationListener(userIndex)
        {
        }

};

}

#endif
