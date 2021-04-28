//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#ifndef ORIGINUSER_H
#define ORIGINUSER_H

#include <QtCore>

#include "services/common/Accessor.h"
#include "services/common/Property.h"
#include "services/session/AbstractSession.h"
#include "services/rest/AuthenticationData.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            class ContentController;
            class GamesController;
            class ContentOperationQueueController;
        }

        namespace Social
        {
            class SocialController;
        }

        namespace Voice
        {
            class VoiceController;
        }

        /// \brief Contains information about an Origin user.
        class ORIGIN_PLUGIN_API User : public QObject
        {
            Q_OBJECT

        public:
            /// \brief Creates a new User object.
            /// \param session Reference to the user's session object.
            /// \param serverData The user's authentication data.
            /// \param online True if the user is logging in online.
            /// \param parent The parent object.
            /// \return A QSharedPointer to the new User object.
            static QSharedPointer<User> create(Origin::Services::Session::SessionRef session, Origin::Services::Authentication2 const& serverData, bool online = true, QObject *parent = NULL);

            /// \brief The User destructor.
            ~User();

            /// \brief Sets the internal pointer to self.
            ///
            /// Needed because Qt doesn't have enable_shared_from_this.
            /// \param self The QSharedPointer to this object.
            void setSelf(QSharedPointer<User> self);

            /// \brief Gets a QSharedPointer to this object.
            /// \return A QSharedPointer to this object.
            QSharedPointer<User> self();

            /// \brief Creates the user's content controller.
            void createContentController();

            /// \brief Creates the user's games controller.
            void createGamesController();

            /// \brief Creates the user's achievement controller.
            void createAchievementController(Origin::Services::Session::SessionRef session, bool online);

            /// \brief Creates the user's social controller
            void createSocialController();

            /// \brief Creates the user's voice controller
            void createVoiceController();            

            /// \brief Creates the content operation queue controller
            void createContentOperationQueueController();

signals:
            /// \brief Emitted when the user goes online or offline.
            /// \param isOnline True if the user is now online.
            void onlineStatusChanged(bool isOnline);

            /// \brief Emitted when the user's locale changes.
            /// \param locale String representing the locale the user changed to.
            void localeChanged(QString const& locale);

        public slots:

            /// \brief Sets the user's locale.
            /// \param locale A string representing the user's new locale.
            void setLocale(QString const& locale);

        public:

            /// \brief Checks if the user is underage.
            /// \return True if the user is underage.
            bool isUnderAge() const { return mServerData.user.isUnderage; }

            /// \brief Gets the user's nucleus ID.
            /// \return The user's nucleus ID.
            qint64 userId() const { return mServerData.user.userId; }

            /// \brief Gets the user's persona ID (how is this different from nucleus ID?).
            /// \return The user's persona ID.
            qint64 personaId() const { return mServerData.user.personaId; }

            /// \brief Gets the user's email address.
            /// \return The user's email address.
            QString email() const { return mServerData.user.email; }

            /// \brief Gets the user's country.
            /// \return The user's country.
            QString country() const { return mServerData.user.country; }

            /// \brief Gets the user's EA ID (Origin ID).
            /// \return The user's EA ID (Origin ID).
            QString eaid() const { return mServerData.user.originId; }

            /// \brief Gets the user's first name.
            /// \return The user's first name.
            QString firstName() const { return mServerData.user.firstName; }

            /// \brief Gets the user's last name.
            /// \return The user's last name.
            QString lastName() const { return mServerData.user.lastName; }

            /// \brief Gets the user's DOB
            /// \return The user's DOB.
            QString dob() const { return mServerData.user.dob; }

            /// \brief Checks if JIT is enabled.
            /// \return True if JIT is enabled.
            bool enableJIT() const { return true/*QString::compare(mServerData.misc.enableJTL, "true", Qt::CaseInsensitive) == 0*/; }

            /// \brief Gets the user's default tab.
            /// \return The user's default tab.
            QString defaultTab() const { return mServerData.appSettings.dt; }

            /// \brief Gets the user's override tab.
            /// \return The user's override tab.
            bool overrideTab() const { return mServerData.appSettings.overrideTab; }

            /// \brief Gets the user's current locale.
            /// \return A string representing the user's current locale.
            QString const& locale() const;

            /// \brief Gets the user's content controller object.
            /// \return The user's content controller object.
            Origin::Engine::Content::ContentController* contentControllerInstance();

            /// \brief Gets the user's games controller object.
            /// \return The user's games controller object.
            Origin::Engine::Content::GamesController* gamesControllerInstance();
            
            /// \brief Gets the user's social controller object.
            /// \return The user's social controller object.
            Origin::Engine::Social::SocialController* socialControllerInstance();

            /// \brief Gets the user's voice controller object.
            /// \return The user's voice controller object.
            Origin::Engine::Voice::VoiceController* voiceControllerInstance();

            /// \brief Gets the user's content queue controller object.
            /// \return The user's content queue controller object.
            Origin::Engine::Content::ContentOperationQueueController* contentOperationQueueControllerInstance();

            /// \brief The user's authentication data.
            Origin::Services::Authentication2 const& mServerData;

            /// \brief Gets the user's session.
            ///
            /// \todo TODO rename to session()
            /// \return A reference to the user's session.
            Origin::Services::Session::SessionRef getSession () {return mSession;}

            /// \brief Clears the user's Controllers
            void cleanUp();

        private:
            /// \brief The User constructor
            /// \param session Reference to the user's session object.
            /// \param data The user's authentication data.
            /// \param online True if the user is logging in online.
            /// \param parent The parent object.
            User(Services::Session::SessionRef session, Origin::Services::Authentication2 const& data, bool online = true, QObject *parent = 0);

            QWeakPointer<User> mSelf; ///< Weak pointer to this object.
            Origin::Services::Session::SessionRef mSession; ///< Reference to the user's session.
            QScopedPointer<QObject> m_ContentController; ///< Pointer to the user's ContentController object.
            QScopedPointer<QObject> m_GamesController; ///< Pointer to the user's GamesController object.
            QScopedPointer<QObject> m_SocialController; ///< Pointer to the user's SocialController object
            QScopedPointer<QObject> m_VoiceController; ///< Pointer to the user's VoiceController object
            QScopedPointer<QObject> m_ContentOperationQueueController; ///< Pointer to the user's ContentOperationQueueController object

            /// \brief String representing the user's locale.
            QString mLocale;
        };

        typedef QSharedPointer<User> UserRef;
        typedef QWeakPointer<User> UserWRef;
        //typedef QSharedPointer<User> StrongUserRef;

        /// \brief Gets the user's locale.
        /// \return The string representing the user's locale.
        inline QString const& User::locale() const
        {
            return mLocale;
        }

    }
}
#endif // ORIGINUSER_H
