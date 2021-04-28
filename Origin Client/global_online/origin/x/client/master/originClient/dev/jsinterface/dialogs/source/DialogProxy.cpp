/////////////////////////////////////////////////////////////////////////////
// DialogProxy.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "DialogProxy.h"
#include <QMetaObject>
#include <QUuid>
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "DialogController.h"
#include "EbisuSystemTray.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

static const QString KEY_ID = "id";

DialogItem::DialogItem(const QJsonObject& dialogInfo, const QJsonObject& persistingInfo, QObject* callbackObj, const QString& callbackFunc, const QString& id)
: QObject()
, mDialogInfo(dialogInfo)
, mPersistingInfo(persistingInfo)
, mCallbackObj(callbackObj)
, mCallbackFunc(callbackFunc)
{
    mDialogInfo[KEY_ID] = id;
}


void DialogItem::processDialogResults(QJsonObject obj) 
{
    if(mCallbackObj && mCallbackFunc.count())
    {
        QMetaObject::invokeMethod(mCallbackObj, mCallbackFunc.toStdString().c_str(), Qt::QueuedConnection, Q_ARG(QJsonObject, obj));
    }
}


const QString DialogItem::id() const
{
    return mDialogInfo[KEY_ID].toString();
}


const QString DialogItem::title() const
{
    return mDialogInfo[DialogController::MessageTemplate::KEY_TITLE].toString();
}


const QString DialogItem::rejectGroup() const
{
    return mDialogInfo[DialogController::MessageTemplate::KEY_REJECTGROUP].toString();
}


void DialogItem::setDialogInfo(const QJsonObject& info)
{
    const QString id = mDialogInfo[KEY_ID].toString();
    mDialogInfo = info;
    // Don't lose the Id. We attach the id to the dialog info when it's created.
    mDialogInfo[KEY_ID] = id;
}



DialogProxy::DialogProxy(QObject* parent)
: QObject(parent)
, mCurrentDialogId("")
{
    ORIGIN_VERIFY_CONNECT(
        DialogController::instance(), SIGNAL(showDialog(const QJsonObject&, QObject*, const char*, const QJsonObject&)), 
        this, SLOT(onShowDialog(const QJsonObject&, QObject*, const char*, const QJsonObject&))
        );
    ORIGIN_VERIFY_CONNECT(
        DialogController::instance(), SIGNAL(updateDialog(const QJsonObject&, QObject*, const char*, const QJsonObject&)), 
        this, SLOT(onUpdateDialog(const QJsonObject&, QObject*, const char*, const QJsonObject&))
        );
    ORIGIN_VERIFY_CONNECT(
        DialogController::instance(), SIGNAL(closeDialog(const QString&)), 
        this, SLOT(onCloseDialogByOverrideId(const QString&))
        );
    ORIGIN_VERIFY_CONNECT(
        DialogController::instance(), SIGNAL(closeGroup(const QString&)), 
        this, SLOT(onCloseGroup(const QString&))
        );
}


void DialogProxy::onShowDialog(const QJsonObject& dialogInfo, QObject* callbackObj, const char* callbackFunc, const QJsonObject& persistingInfo)
{
    EbisuSystemTray::instance()->showPrimaryWindow();
    // Check to see if this kind of dialog is already showing. If so, just send update information
    int index = -1;
    DialogItem* dialog = getDialogByOverrideId(dialogInfo[DialogController::MessageTemplate::KEY_OVERRIDEID].toString(), index);
    if(dialog)
    {
        dialog->setDialogInfo(dialogInfo);
        ORIGIN_LOG_ACTION << dialogInfo[DialogController::MessageTemplate::KEY_TITLE].toString() <<": dialog exists at index " << QString::number(index);
        emit dialogChanged(dialogInfo.toVariantMap());
    }
    else
    {
        if(dialogInfo[DialogController::MessageTemplate::KEY_HIGHPRIORITY].toBool())
        {
            DialogItem* dialogItem = new DialogItem(dialogInfo, persistingInfo, callbackObj, callbackFunc, QUuid::createUuid().toString());
            mCurrentDialogId = dialogItem->id();
            mDialogs.push_front(dialogItem);
        }
        else
        {
            mDialogs.push_back(new DialogItem(dialogInfo, persistingInfo, callbackObj, callbackFunc, QUuid::createUuid().toString()));
        }

        // If there's at least one item in mDialogs, then show
        if(mDialogs.length() >= 1)
        {
            QJsonObject showContext;
            showContext["context"] = "onShowDialog";
            show(showContext.toVariantMap());
        }
    }
}


void DialogProxy::onUpdateDialog(const QJsonObject& dialogInfo, QObject* callbackObj, const char* callbackFunc, const QJsonObject& persistingInfo)
{
    int index = -1;
    DialogItem* dialog = getDialogByOverrideId(dialogInfo[DialogController::MessageTemplate::KEY_OVERRIDEID].toString(), index);
    if(dialog)
    {
        dialog->setDialogInfo(dialogInfo);
        ORIGIN_LOG_ACTION << dialogInfo[DialogController::MessageTemplate::KEY_TITLE].toString() <<": dialog exists at index " << QString::number(index);
        emit dialogChanged(dialogInfo.toVariantMap());
    }
}


void DialogProxy::onCloseDialogByOverrideId(const QString& overrideId)
{
    int index = -1;
    DialogItem* dialog = getDialogByOverrideId(overrideId, index);
    if(dialog)
    {
        ORIGIN_LOG_ACTION << "Dialog Closing with override id: " << overrideId;
        mDialogs.removeAt(index);
        closeDialog(dialog->id());
        delete dialog;
    }
    else
    {
        ORIGIN_LOG_ERROR << "Can't close dialog with override id: " << overrideId;
    }
}


void DialogProxy::onCloseGroup(const QString& rejectGroup)
{
    ORIGIN_LOG_ACTION << "Closing rejectGroup: " << rejectGroup;
    QStringList closedIds;
    foreach(DialogItem* iDialog, mDialogs)
    {
        if(QString::compare(iDialog->rejectGroup(), rejectGroup, Qt::CaseInsensitive) == 0)
        {
            mDialogs.removeOne(iDialog);
            closedIds << iDialog->id();
            iDialog->deleteLater();
        }
    }
    ORIGIN_LOG_ACTION << QString("Closed %1: %2").arg(closedIds.count()).arg(closedIds.join(", "));
    if(closedIds.contains(mCurrentDialogId))
    {
        ORIGIN_LOG_ACTION << "Closing current dialog with id: " << mCurrentDialogId;
        closeDialog(mCurrentDialogId);
    }
}


void DialogProxy::closeDialog(const QString& id)
{
    QJsonObject obj;
    obj[KEY_ID] = id;
    if(mCurrentDialogId == id)
        mCurrentDialogId = "";
    emit dialogClosed(obj.toVariantMap());
}


DialogItem* DialogProxy::getDialogById(const QString& uuid, int& index) 
{
    for(int iDialog = 0, length = mDialogs.length(); iDialog < length; iDialog++)
    {
        if(mDialogs[iDialog]->id() == uuid) 
        {
            index = iDialog;
            return mDialogs[iDialog];
        }
    }
    return NULL;
}


DialogItem* DialogProxy::getDialogByOverrideId(const QString& overrideId, int& index) 
{
    if(overrideId.isEmpty())
        return NULL;

    for(int iDialog = 0, length = mDialogs.length(); iDialog < length; iDialog++)
    {
        QJsonObject compareInfo = mDialogs[iDialog]->dialogInfo();
        if(compareInfo[DialogController::MessageTemplate::KEY_OVERRIDEID].toString() == overrideId) 
        {
            index = iDialog;
            return mDialogs[iDialog];
        }
    }
    return NULL;
}


QVariantMap DialogProxy::show(QVariantMap args)
{
    const QJsonObject obj = QJsonObject::fromVariantMap(args);
    const QString context = Services::JsonValueValidator::validate(obj["context"]).toString();
    ORIGIN_LOG_ACTION << "Trying to show dialog: " << context;

    if(mDialogs.length() == 0)
    {
        ORIGIN_LOG_ERROR << QString("Won't show dialog - Dialog queue is empty");
        return QVariantMap();
    }

    if(mCurrentDialogId.count() && mDialogs[0]->id() != mCurrentDialogId)
    {
        ORIGIN_LOG_ERROR << QString("Won't show dialog - %0 (%1) | id: %2 is showing").arg(mDialogs[0]->title()).arg(mDialogs[0]->id()).arg(mCurrentDialogId);
        return QVariantMap();
    }
    const QJsonObject dialogInfo = mDialogs[0]->dialogInfo();
    mCurrentDialogId = mDialogs[0]->id();
    ORIGIN_LOG_ACTION << QString("Showing Dialog: %1 | id: %2").arg(mDialogs[0]->title()).arg(mCurrentDialogId);
    emit dialogOpen(dialogInfo.toVariantMap());
    return QVariantMap();
}


QVariantMap DialogProxy::finished(QVariantMap args)
{
    const QJsonObject obj = QJsonObject::fromVariantMap(args);
    const QString id = Services::JsonValueValidator::validate(obj["id"]).toString();
    const bool accepted = Services::JsonValueValidator::validate(obj["accepted"]).toBool();
    QJsonObject dialogResults = Services::JsonValueValidator::validate(obj["content"]).toObject();
    int index = -1;
    DialogItem* dialog = getDialogById(id, index);

    if(dialog)
    {
        QString content;
        foreach(QString ikey, dialogResults.keys())
        {
            content.append(QString(" | %0: %1").arg(ikey).arg(dialogResults[ikey].toString()));
        }

        // Send back the persisted info
        const QStringList persistingKeys = dialog->persistingInfo().keys();
        foreach(QString key, persistingKeys)
        {
            dialogResults.insert(key, dialog->persistingInfo().value(key));
        }

        // Insert accepted into the dialog results so we don't have to worry about having a blind bool parameter.
        dialogResults.insert("accepted", accepted);
        mDialogs.removeAt(index);
        dialog->processDialogResults(dialogResults);

        const QString rejectGroup = dialog->rejectGroup();
        if(accepted == false && rejectGroup.count())
        {
            onCloseGroup(rejectGroup);
        }

        ORIGIN_LOG_ACTION << QString("Dialog Finished. id %0 | accepted: %1")
            .arg(id)
            .arg(accepted ? "true" : "false")
            .arg(content);
        delete dialog;

        // Need closeDialog() to close the local version of the window if it's closed in an external web SPA
        closeDialog(id);
        // Try to show the next one
        QJsonObject showContext;
        showContext["context"] = "finish";
        show(showContext.toVariantMap());
    }

    return QVariantMap();
}


// We call this from the javascript. This is for capturing dialogs created inside of javascript.
// This is so we don't interrupt any dialogs that the web browser created.
QVariantMap DialogProxy::showingDialog(QVariantMap args)
{
    const QJsonObject obj = QJsonObject::fromVariantMap(args);
    const QString id = Services::JsonValueValidator::validate(obj["id"]).toString();
    ORIGIN_ASSERT(id.count() > 0);
    int index = -1;
    DialogItem* dialog = getDialogById(id, index);
    if(dialog == NULL)
    {
        mCurrentDialogId = id;
        // If there isn't an override id (most likely not since that's a client thing) set the override id to id.
        if(obj[DialogController::MessageTemplate::KEY_OVERRIDEID].toString().isEmpty())
        {
            obj[DialogController::MessageTemplate::KEY_OVERRIDEID] = id;
        }
        mDialogs.push_front(new DialogItem(obj, QJsonObject(), NULL, "", id));
        EbisuSystemTray::instance()->showPrimaryWindow();
    }
    return QVariantMap();
}


QVariantMap DialogProxy::linkReact(QVariantMap args)
{
    QJsonObject obj = QJsonObject::fromVariantMap(args);
    if(mDialogs.count())
    {
        obj["id"] = mDialogs[0]->id();
    }
    DialogController::instance()->linkClicked(obj);
    return QVariantMap();
}

}
}
}