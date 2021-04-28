#ifndef ORIGINLAUNCHER_H
#define ORIGINLAUNCHER_H

#include <QObject>
#include <QString>

#include "CrashReportData.h"

class OriginLauncher : public QObject
{
    Q_OBJECT
public:
    explicit OriginLauncher( QString originPath, QObject *parent = 0 );
    
public slots:
    void launchOrigin();
    
private:
    QString mOriginPath;
};

#endif // ORIGINLAUNCHER_H
