#ifndef _CONVERSIONHELPERS_H
#define _CONVERSIONHELPERS_H

/**********************************************************************************************************
 * This file is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QVariantMap>

#include "chat/Presence.h"
#include "chat/SubscriptionState.h"
#include "chat/OriginGameActivity.h"
#include "services/plugin/PluginAPI.h"
#include "engine/social/Conversation.h"
#include "engine/social/ConversationEvent.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{
namespace ConversionHelpers
{
    ORIGIN_PLUGIN_API QString conversationTypeToString(Engine::Social::Conversation::ConversationType type);
    ORIGIN_PLUGIN_API QString availabilityToString(Chat::Presence::Availability);
    ORIGIN_PLUGIN_API Chat::Presence::Availability stringToAvailability(const QString &str, bool &ok);

    // Will return null QVariant if the game activity is null
    ORIGIN_PLUGIN_API QVariant originGameActivityToDict(const Chat::OriginGameActivity &gameActivity);

    ORIGIN_PLUGIN_API QVariantMap subscriptionStateToDict(const Chat::SubscriptionState &);
    ORIGIN_PLUGIN_API QString subscriptionStateToString(const Chat::SubscriptionState &);
}
}
}
}

#endif

