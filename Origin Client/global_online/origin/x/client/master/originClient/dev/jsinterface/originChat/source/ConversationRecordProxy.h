#ifndef _CONVERSATIONRECORDPROXY_H
#define _CONVERSATIONRECORDPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/
#include <QObject>
#include <QVariantList>
#include <QDateTime>
#include <QSharedPointer>

#include <WebWidget/ScriptOwnedObject.h>
#include <engine/social/ConversationRecord.h>
#include <services/plugin/PluginAPI.h>

namespace Origin
{

namespace Client
{
namespace JsInterface
{
    class OriginSocialProxy;

    class ORIGIN_PLUGIN_API ConversationRecordProxy : public WebWidget::ScriptOwnedObject
    {
        Q_OBJECT
    public:
        ConversationRecordProxy(QWeakPointer<Engine::Social::ConversationRecord> conversationRecord);

        Q_PROPERTY(QDateTime startTime READ startTime);
        QDateTime startTime();
        
        Q_PROPERTY(QDateTime endTime READ endTime);
        QDateTime endTime();

        Q_PROPERTY(QVariantList events READ events);
        QVariantList events();

    private:
		QWeakPointer<Engine::Social::ConversationRecord> mProxied;
    };
}
}
}

#endif

