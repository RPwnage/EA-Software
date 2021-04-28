#include "ConversionHelpers.h"

#include <QMap>

namespace Origin
{
namespace Client
{
namespace JsInterface
{
namespace ConversionHelpers
{
    using namespace Chat;

    namespace
    {
        QMap<QString, Presence::Availability> availabilityMap()
        {
            static QMap<QString, Presence::Availability> staticMap;

            if (staticMap.isEmpty())
            {
                staticMap["CHAT"] = Presence::Chat;
                staticMap["ONLINE"] = Presence::Online;
                staticMap["AWAY"] = Presence::Away;
                staticMap["XA"] = Presence::Xa;
                staticMap["DND"] = Presence::Dnd;
                staticMap["UNAVAILABLE"]  = Presence::Unavailable;
            }

            return staticMap;
        }
    }

    QString conversationTypeToString(Engine::Social::Conversation::ConversationType type)
    {
        switch (type)
        {
        case Origin::Engine::Social::Conversation::OneToOneType:
            return "ONE_TO_ONE";

        case Origin::Engine::Social::Conversation::GroupType:
            return "MULTI_USER";

        case Origin::Engine::Social::Conversation::DestroyedType:
            return "DESTROYED";

        case Origin::Engine::Social::Conversation::KickedType:
            return "KICKED";

        case Origin::Engine::Social::Conversation::LeftGroupType:
            return "LEFT_GROUP";
       }
    }

    QString availabilityToString(Chat::Presence::Availability availability)
    {
        return availabilityMap().key(availability);
    }

    Chat::Presence::Availability stringToAvailability(const QString &str, bool &ok)
    {
        if (availabilityMap().contains(str))
        {
            ok = true;
            return availabilityMap()[str];
        }
        else
        {
            ok = false;
            return Presence::Unavailable;
        }
    }
    
    QVariant originGameActivityToDict(const Chat::OriginGameActivity &gameActivity)
    {
        if (gameActivity.isNull())
        {
            return QVariant();
        }

        QVariantMap playingInfo;
        playingInfo["productId"] = gameActivity.productId();
        playingInfo["gameTitle"] = gameActivity.gameTitle();
        playingInfo["joinable"] = gameActivity.isLocalUser() ? gameActivity.joinable() || gameActivity.joinable_invite_only() : gameActivity.joinable();

        if (gameActivity.broadcastUrl().isEmpty())
        {
            playingInfo["broadcastUrl"] = QVariant();
        }
        else
        {
            playingInfo["broadcastUrl"] = gameActivity.broadcastUrl();
        }

        playingInfo["purchasable"] = gameActivity.purchasable();

        return playingInfo;
    }
    
    QVariantMap subscriptionStateToDict(const Chat::SubscriptionState &subscriptionState)
    {
        using Chat::SubscriptionState;

        QVariantMap ret;

        ret["pendingContactApproval"] = subscriptionState.isPendingContactApproval();
        ret["pendingCurrentUserApproval"] = subscriptionState.isPendingCurrentUserApproval();

        switch(subscriptionState.direction())
        {
        case SubscriptionState::DirectionNone:
            ret["direction"] = "NONE";
            break;
        case SubscriptionState::DirectionTo:
            ret["direction"] = "TO";
            break;
        case SubscriptionState::DirectionFrom:
            ret["direction"] = "FROM";
            break;
        case SubscriptionState::DirectionBoth:
            ret["direction"] = "BOTH";
            break;
        }

        return ret;
    }

    QString subscriptionStateToString(const Chat::SubscriptionState &subscriptionState)
    {
        using Chat::SubscriptionState;

        QString subStateStr = "NONE";
        switch(subscriptionState.direction())
        {
        case SubscriptionState::DirectionNone:
            subStateStr = "NONE";
            break;
        case SubscriptionState::DirectionTo:
            subStateStr = "TO";
            break;
        case SubscriptionState::DirectionFrom:
            subStateStr = "FROM";
            break;
        case SubscriptionState::DirectionBoth:
            subStateStr = "BOTH";
            break;
        }

        return subStateStr;
    }
}
}
}
}
