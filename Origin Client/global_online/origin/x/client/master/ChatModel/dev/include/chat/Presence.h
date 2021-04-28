#ifndef _CHATMODEL_PRESENCE_H
#define _CHATMODEL_PRESENCE_H

#include <QString>
#include "OriginGameActivity.h"

namespace Origin
{
namespace Chat
{
    /// Activity defined by http://xmpp.org/extensions/xep-0108.html User Activity
    struct UserActivity
    {
        UserActivity(const std::string& category_, const std::string& instance_, const std::string& body_ = std::string())
            : general(category_.c_str())
            , specific(instance_.c_str())
            , description(body_.c_str())
        {
        }

        UserActivity()
            : general()
            , specific()
            , description()
        {
        }

        UserActivity& operator=(const UserActivity& from)
        {
            general = from.general;
            specific = from.specific;
            description = from.description;
            return *this;
        }

        bool operator==(const UserActivity& other) const
        {
            return  (general == other.general) && (specific == other.specific) && (description == other.description);
        }

        std::string general;
        std::string specific;
        std::string description;
    };

    ///
    /// Represents an XMPP user's presence state
    ///
    /// Presents consists of three components:
    /// - A base availability (online, away, do not disturb, etc)
    /// - An optional human-readable status string
    /// - An optional Origin-specific game activity
    class Presence
    {
    public:
        ///
        /// Represents an XMPP availability
        /// 
        /// This corresponds to the \<show\> child element of \<presence\> described in RFC 6121 section 2.2.2.1
        ///
        enum Availability
        {
            ///
            /// The user is interested in chatting
            ///
            Chat,

            ///
            /// The user is online and available
            ///
            Online,

            ///
            /// The user is busy
            ///
            Dnd,

            ///
            /// The user is temporarily away
            ///
            Away,
            
            ///
            /// The user is away for an extended period
            ///
            Xa,

            ///
            /// The user is unavailable
            ///
            Unavailable,
        };

        ///
        /// Instantiates a null Presence object
        ///
        Presence() :
            mNull(true),
            mAvailability(Unavailable)
        {
        }

        // Instantiates an activity only Presence object
        Presence(const UserActivity& activity) :
            mNull(false),
            mUserActivity(activity)
        {
        }

        ///
        /// Instantiates a non-null Presence object with the specified components
        ///
        explicit Presence(
            Availability availability,
            const QString &statusText = QString(),
            const OriginGameActivity &gameActivity = OriginGameActivity(),
            const UserActivity& userActivity = UserActivity()) :
            mNull(false),
            mAvailability(availability),
            mStatusText(statusText),
            mGameActivity(gameActivity),
            mUserActivity(userActivity)
        {
        }

        Presence(const Presence& from) :
            mNull(from.mNull),
            mAvailability(from.mAvailability),
            mStatusText(from.mStatusText),
            mGameActivity(from.mGameActivity),
            mUserActivity(from.mUserActivity)
        {
        }

        Presence& operator=(const Presence& from)
        {
            mNull = from.mNull;
            mAvailability = from.mAvailability;
            mStatusText = from.mStatusText;
            mGameActivity = from.mGameActivity;
            mUserActivity = from.mUserActivity;
            return *this;
        }

        ///
        /// Returns if this presence is null
        ///
        bool isNull() const 
        {
            return mNull;
        }

        ///
        /// Sets the null component of the presence
        ///
        void setNull(bool null)
        {
            mNull = null;
        }

        ///
        /// Returns the availability component of the presence
        ///
        Availability availability() const
        {
            return mAvailability;
        }

        ///
        /// Sets the availability component of the presence
        ///
        void setAvailability(Availability availability)
        {
            mAvailability = availability;
        }

        ///
        /// Returns the status component of the presence
        ///
        const QString& statusText() const
        {
            return mStatusText;
        }

        ///
        /// Sets the status component of the presence
        ///
        void setStatusText(const QString &statusText)
        {
            mStatusText = statusText;
        }

        ///
        /// Returns the Origin game activity component of the presence
        ///
        OriginGameActivity gameActivity() const
        {
            return mGameActivity;
        }

        ///
        /// Sets the Origin game activity component of the presence
        ///
        void setGameActivity(const OriginGameActivity &gameActivity)
        {
            mGameActivity = gameActivity;
        }

        ///
        /// Returns the user activity
        ///
        UserActivity userActivity() const
        {
            return mUserActivity;
        }

        ///
        /// Compares two Presence instances for equality
        ///
        bool operator==(const Presence &other) const
        {
            return isNull() == other.isNull() &&
                   availability() == other.availability() &&
                   statusText() == other.statusText() &&
                   gameActivity() == other.gameActivity() &&
                   mUserActivity == other.userActivity();
        }

        ///
        /// Compares two presence instances for inequality
        ///
        bool operator!=(const Presence &other) const
        {
            return !(*this == other);
        }

    private:
        bool mNull;
        Availability mAvailability;
        QString mStatusText;
        OriginGameActivity mGameActivity;
        UserActivity mUserActivity;
    };
}
}

#endif
