//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.

#ifndef GAMESCONTROLLER_H
#define GAMESCONTROLLER_H

#include <limits>
#include <QDateTime>
#include <QObject>
#include "engine/login/user.h"
#include "engine/content/Entitlement.h"
#include "services/rest/GamesServiceResponse.h"
#include "services/plugin/PluginAPI.h"

namespace Origin 
{
    namespace Engine
    {
        class User;

        namespace Content
        {
            
            /// \brief TBD.
            class ORIGIN_PLUGIN_API GamesController : public QObject
            {
                Q_OBJECT
            public:
                
                /// \brief The GamesController destructor; releases the resources of a GamesController object.
                virtual ~GamesController();

                /// \brief Creates a new games controller.
                ///
                /// \param  user  The user associated with these entitlements. 
                /// \returns GamesController A pointer to the newly created games controller.
                static GamesController * create(Origin::Engine::UserRef user);
                
                
                /// \brief TBD.
                /// \return GamesController TBD.
                static GamesController * currentUserGamesController();


            public:
                
                /// \brief Returns the user.
                ///
                /// \return UserRef A reference to the user. 
                Origin::Engine::UserRef user() const;

                /// \brief Kicks off a request to get the time played from the masterTitleId.
                /// \param masterTitleId TBD.
                void getPlayedGameTime(const QString& masterTitleId, const QString &multiplayerId);

                /// \brief Kicks off a request to get the time played for a non Origin game.
                /// \param appId The app id of the non Origin game.
                void getNonOriginPlayedGameTime(const QString appId);

                /// \brief Kicks off a request to post that you're playing a game.
                /// \param entitlement Reference to the entitlement.
                void postPlayingGame(Content::EntitlementRef entitlement);

                /// \brief Kicks off a request to post that you're playing a non-Origin game.
                /// \param entitlement Reference to the entitlement.
                void postPlayingNonOriginGame(Content::EntitlementRef entitlement);

                /// \brief Kicks off a request to post that you've finished playing a game.
                /// \param entitlement Reference to the entitlement.
                void postFinishedPlayingGame(Content::EntitlementRef entitlement);

                /// \brief Kicks off a request to post that you've finished playing a non-Origin game.
                /// \param entitlement Reference to the entitlement.
                void postFinishedPlayingNonOriginGame(Content::EntitlementRef entitlement);

				/// \brief Returns the time in seconds for the given entitlement, 0 if the time is not valid.
				quint64 getCachedTimePlayed(Content::EntitlementRef entitlement);

                /// \brief Returns the time in seconds for the given entitlement, 0 if the time is not valid.
                QDateTime getCachedLastTimePlayed(Content::EntitlementRef entitlement);

                /// \brief Returns if the passed product ID is has a multi launch default
                QString gameMultiLaunchDefault(const QString& productId);

                /// \brief Sets the passed title as the passed product id's default launch title
                void setGameMultiLaunchDefault(const QString& productId, const QString& title);

                /// \brief Returns if the passed product ID has OIG manually disabled.
                bool gameHasOIGManuallyDisabled(const QString& productId);

                /// \brief Manually turns off the OIG for this entitlement.
                void setGameHasOIGManuallyDisabled(const QString& productId, const bool& disabled);

                /// \brief ORSUBS-1959: Returns if we showed the first launch message.
                QString firstLaunchMessageShown(const QString& productId);

                /// \brief ORSUBS-1959: Sets if we should/have shown the entitlement's first launch message
                void setFirstLaunchMessageShown(const QString& productId, const QString& arg);

                /// \brief ORSUBS-1526: Returns if we showed a warning for upgraded save game issue
                bool upgradedSaveGameWarningShown(const QString& productId);

                /// \brief ORSUBS-1526: Sets the command line arguments the game will launch with.
                void setUpgradedSaveGameWarningShown(const QString& productId, const bool& arg);

                /// \brief Returns the command line arguments the game will launch with.
                QString gameCommandLineArguments(const QString& productId);

                /// \brief Sets the command line arguments the game will launch with.
                void setGameCommandLineArguments(const QString& productId, const QString& arg);

                /// \brief Returns if the passed content ID is hidden
                bool gameIsHidden(Content::EntitlementRef entitlement);

                /// \brief Sets the passed content ID as hidden
                void setGameIsHidden(Content::EntitlementRef entitlement, bool hidden);

                /// \brief Return if the passed content ID is favorited
                bool gameIsFavorite(Content::EntitlementRef entitlement);

                /// \brief Sets the passed content ID is favorited
                void setGameIsFavorite(Content::EntitlementRef entitlement, bool favorite);

                /// \brief Special label meaning to show all launchers to the user
                static const QString SHOW_ALL_TITLES;

            signals:
				/// \brief Emitted when we have retrieved a successful response from the playing time request.
                /// \param masterTitleId TBD.
                /// \param multiplayerId TBD.
				void playingGameTimeUpdateCompleted(const QString &masterTitleId, const QString &multiplayerId);

				/// \brief Emitted when we get an error from the playing time request.
                /// \param masterTitleId TBD.
                /// \param multiplayerId TBD.
				void playingGameTimeUpdateFailed(const QString &masterTitleId, const QString &multiplayerId); 

            private slots:
				/// \brief Posts slots.
                void onPostPlayingFinished();
                void processPlayingNonOriginGameResponse();
                                
				/// \brief TBD.
                void onPostFinishedPlayingFinished();
                void processFinishedPlayingNonOriginGameResponse();

				/// \brief Gets slots.
                void onGetPlayedGameTimeFinished();
                void onGetNonOriginGameUsageFinished();

            private:
                /// \brief Reads in local game settings from user settings.
                QString readGameSettingFromUserSetting(const QString& userSetting, const QString& productId);

                /// \brief Formats local game settings to write to the user settings.
                void formatGameSettingToUserSetting(QString& userSetting, const QString& settingValue, const QString& productId);

				/// \brief TBD.
                UserWRef m_User;
                                
				/// \brief The GamesController constructor.
                /// \param user TBD.
                explicit GamesController(Origin::Engine::UserRef user);
                                
				/// \brief TBD.
                QString buildSaveDataPath();
                                
				/// \brief TBD.
				QMap<QString, QMap<QString, quint64> > mGameTimePlayedMap;
                                
				/// \brief TBD.
                QMap<QString, QMap<QString, QDateTime> > mGameLastTimePlayedMap;

            }; //GamesController
        } //Content        
    } //Engine
} //Origin

#endif // GAMESCONTROLLER_H
