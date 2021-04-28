#ifndef ORIGINSDKMAIN_H
#define ORIGINSDKMAIN_H

#include "OriginSDK/OriginSDK.h"

#include <QObject>
#include <QString>

namespace fb
{
class OriginStreamInstallBackend;
class MessageManager;
}

class OriginSDKMain : public QObject
{
    friend class fb::MessageManager;

    Q_OBJECT
public:
    OriginSDKMain();
    ~OriginSDKMain();

    bool StartOrigin();

    void DoOriginUpdate();

    static OriginErrorT EventCallback (int32_t eventId, void* pContext, void* eventData, uint32_t count);
    OriginErrorT EventCallback (int32_t eventId, void* eventData, uint32_t count);

    void Log(QString msg);

private:
    void QueueMessage(int category, int type, QString msg);

private slots:
    void onQueuedMessage(int category, int type, QString msg);

signals:
    void LogMessage(QString msg);

private:
    bool _sdkInitialized;
    fb::OriginStreamInstallBackend* _fbBackend;
};

#endif // ORIGINSDKMAIN_H
