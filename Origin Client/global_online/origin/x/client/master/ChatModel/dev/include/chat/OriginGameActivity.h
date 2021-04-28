#ifndef _CHATMODEL_ORIGINGAMEACTIVITY_H
#define _CHATMODEL_ORIGINGAMEACTIVITY_H

#include <QString>

namespace Origin
{
namespace Chat
{
    ///
    /// Represents Origin-specific game playing activity
    ///
    /// This encoded in to an XEP-0108 user activity. It can also be encoded in to a legacy presence/status pair
    /// for usage by Origin clients <= 9.2
    ///
    class OriginGameActivity
    {
    public:
        ///
        /// Creates an null OriginGameActivity
        ///
        OriginGameActivity();

        ///
        /// Copy Contructor for OriginGameActivity
        ///
        OriginGameActivity(const OriginGameActivity &activity);

        ///
        /// Creates a non-null OriginGameActivity
        ///
        /// \param localUser      indicator whether the game activity is for a local user or not. The behavior of joinable will be different depending on this flag.
        /// \param productId      Product ID being played
        /// \param gameTitle      Human readable title of the game being played. This is in the playing user's locale.
        /// \param joinable       Indicates if the game has a joinable multiplayer session
        /// \param invite_only    user only shows up as joinable when he also sends an invite.
        /// \param gamePresence   Opaque game-internal presence string
        /// \param multiplayerId  Multiplayer ID of the game being played
        ///
        OriginGameActivity(bool localUser, const QString &productId, const QString &gameTitle, bool joinable, bool invite_only, const QString &gamePresence = QString(), const QString &multiplayerId = QString());

        ///
        /// Returns the product ID of the game being played
        ///
        const QString& productId() const
        {
            return mProductId;
        }

        ///
        /// Returns the human readable title of the game being played
        ///
        /// This is in the playing user's locale
        ///
        const QString& gameTitle() const
        {
            return mGameTitle;
        }

        bool isLocalUser() const { return mIsLocalUser; }

        ///
        /// Returns if the game has a joinable multiplayer session
        ///
        bool joinable() const
        {
            if(mIsLocalUser)
            {
                return mJoinable;
            }
            else
            {
                // Only return joinable and 
                return mJoinable || (mJoinableInviteOnly && mInvited);
            }
        }

        ///
        /// Returns if the game has a joinable invite only multiplayer session
        ///
        bool joinable_invite_only() const
        {
            return mJoinableInviteOnly;
        }

        ///
        /// Returns an opaque game-internal presence string
        ///
        const QString& gamePresence() const
        {
            return mGamePresence;
        }

        ///
        /// Returns the multiplayer ID of the game being played
        ///
        /// This may be null
        ///
        const QString& multiplayerId() const
        {
            return mMultiplayerId;
        }

        ///
        /// Returns the broadcast url of the user
        ///
        /// This may be null
        ///
        const QString& broadcastUrl() const
        {
            return mBroadcastUrl;
        }

        ///
        /// Returns a game activity with the given broadcast Url
        ///
        /// \param broadcastUrl      Product ID being played
        ///
        OriginGameActivity withBroadcastUrl(const QString& broadcastUrl) const
        {
            OriginGameActivity copy = *this;
            copy.mBroadcastUrl = broadcastUrl;
            return copy;
        }

        ///
        /// Returns if this represents a null playing status
        ///
        bool isNull() const
        {
            return mNull;
        }

        ///
        /// Compares two OriginGameActivity instances for equality
        ///
        bool operator==(const OriginGameActivity &other) const;
        
        ///
        /// Compares two OriginGameActivity instances for inequality
        ///
        bool operator!=(const OriginGameActivity &other) const
        {
            return !(*this == other);
        }

		///
        /// Returns if the game is purchasable NOG's are not
        ///
		bool purchasable() const
		{
			return mPurchasable;
		}

        void setInvited(bool _invited)
        {
            mInvited = _invited;
        }

        bool invited() const
        {
            return mInvited;
        }

    private:
        bool mNull;

        QString mProductId;
        QString mGameTitle;
        bool mIsLocalUser;
        bool mJoinable;
        bool mJoinableInviteOnly;
        bool mInvited;              
        QString mGamePresence;
        QString mMultiplayerId;
        QString mBroadcastUrl;
		bool mPurchasable;
    };
}
}

#endif
