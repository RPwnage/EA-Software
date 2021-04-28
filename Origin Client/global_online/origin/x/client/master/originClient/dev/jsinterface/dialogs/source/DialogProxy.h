/////////////////////////////////////////////////////////////////////////////
// DialogProxy.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef _DialogProxy_H
#define _DialogProxy_H

#include <QObject>
#include <QVector>
#include <QVariant>
#include <QJsonObject>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class DialogItem : public QObject
{
    Q_OBJECT

public:
    DialogItem(const QJsonObject& dialogInfo, const QJsonObject& persistingInfo, QObject* callbackObj, const QString& callbackFunc, const QString& id);
    void processDialogResults(QJsonObject obj);
    const QString id() const;
    const QString title() const;
    const QString rejectGroup() const;
    void setDialogInfo(const QJsonObject& info);
    const QJsonObject dialogInfo() const {return mDialogInfo;}
    const QJsonObject persistingInfo() const {return mPersistingInfo;}

private:
    QJsonObject mDialogInfo;
    const QJsonObject mPersistingInfo;
    QObject* mCallbackObj;
    const QString mCallbackFunc;
};


class ORIGIN_PLUGIN_API DialogProxy : public QObject
{
    Q_OBJECT

public:
    DialogProxy(QObject* parent = 0);
    Q_INVOKABLE QVariantMap show(QVariantMap args = QVariantMap());
    Q_INVOKABLE QVariantMap finished(QVariantMap args);
    // We call this from the javascript. This is for capturing dialogs created inside of javascript.
    // This is so we don't interrupt any dialogs that the web browser created.
    Q_INVOKABLE QVariantMap showingDialog(QVariantMap args = QVariantMap());
    Q_INVOKABLE QVariantMap linkReact(QVariantMap args = QVariantMap());

public slots:
    void onShowDialog(const QJsonObject& dialogInfo, QObject* callbackObj = NULL, const char* callbackFunc = "", const QJsonObject& persistingInfo = QJsonObject());
    void onUpdateDialog(const QJsonObject& dialogInfo, QObject* callbackObj = NULL, const char* callbackFunc = "", const QJsonObject& persistingInfo = QJsonObject());
    void onCloseDialogByOverrideId(const QString& overrideId);
    void onCloseGroup(const QString& rejectGroup);

signals:
    void dialogOpen(QVariantMap);
    void dialogClosed(QVariantMap);
    void dialogChanged(QVariantMap);

private:
    DialogItem* getDialogById(const QString& uuid, int& index);
    DialogItem* getDialogByOverrideId(const QString& overrideId, int& index);
    void closeDialog(const QString& id);
    QList<DialogItem*> mDialogs;
    QString mCurrentDialogId;
};

}
}
}

#endif