#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QTimer>

class OriginSDKMain;

class AppController : public QObject
{
    Q_OBJECT
public:
    AppController();
    ~AppController();

public slots:
    void onStartup();
    void onHeartbeat();

signals:
    void LogMessage(QString msg);

private:
    QTimer* _heartbeatTimer;
    OriginSDKMain* _sdkMain;
};

#endif // APPCONTROLLER_H
