#include <QStringList>

#include "OriginGameActivity.h"

namespace Origin
{
namespace Chat
{
    OriginGameActivity::OriginGameActivity() : 
        mNull(true),
        mIsLocalUser(false),
        mJoinable(false),
        mJoinableInviteOnly(false),
        mInvited(false)
    {
    }

    OriginGameActivity::OriginGameActivity(const OriginGameActivity &activity) :
        mNull(activity.mNull),
        mProductId(activity.mProductId),
        mGameTitle(activity.mGameTitle),
        mIsLocalUser(activity.mIsLocalUser),
        mJoinable(activity.mJoinable),
        mJoinableInviteOnly(activity.mJoinableInviteOnly),
        mInvited(activity.mInvited), 
        mGamePresence(activity.mGamePresence),
        mMultiplayerId(activity.mMultiplayerId),
        mBroadcastUrl(activity.mBroadcastUrl),
        mPurchasable(activity.mPurchasable)
    {

    }


    OriginGameActivity::OriginGameActivity(bool isLocalUser, const QString &productId, const QString &gameTitle, bool joinable, bool invite_only, const QString &gamePresence, const QString &multiplayerId) :
        mNull(false),
        mProductId(productId),
        mGameTitle(gameTitle),
        mIsLocalUser(isLocalUser),
        mJoinable(joinable),
        mJoinableInviteOnly(invite_only),
        mInvited(false),
        mGamePresence(gamePresence),
        mMultiplayerId(multiplayerId)
    {
		mPurchasable = mProductId.startsWith("NOG_") ? false : true;
    }
        
    bool OriginGameActivity::operator==(const OriginGameActivity &other) const
    {
        if (isNull())
        {
            // Other fields should be constant if we're null
            return other.isNull();
        }
        else
        {
            return (mProductId == other.mProductId) &&
                   (mGameTitle == other.mGameTitle) &&
                   (mIsLocalUser == other.mIsLocalUser) &&
                   (mJoinable == other.mJoinable) &&
                   (mJoinableInviteOnly == other.mJoinableInviteOnly) &&
                   (mInvited == other.mInvited) &&
                   (mGamePresence == other.mGamePresence) &&
                   (mMultiplayerId == other.mMultiplayerId) &&
                   (mBroadcastUrl == other.mBroadcastUrl);
        }
    }
}
}
