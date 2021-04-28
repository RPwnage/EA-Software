/////////////////////////////////////////////////////////////////////////////
// DialogController.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "DialogController.h"
#include "services/debug/DebugService.h"
#include "DialogProxy.h"
#include "OriginWebController.h"

namespace Origin
{
namespace Client
{
const QString DialogController::MessageTemplate::KEY_DIALOGTYPE = "dialogType";
const QString DialogController::MessageTemplate::KEY_CONTENTTYPE = "contentType";
const QString DialogController::MessageTemplate::KEY_OVERRIDEID = "overrideId";
const QString DialogController::MessageTemplate::KEY_TITLE = "title";
const QString DialogController::MessageTemplate::KEY_TEXT = "text";
const QString DialogController::MessageTemplate::KEY_ACCEPTTEXT = "acceptText";
const QString DialogController::MessageTemplate::KEY_ACCEPTENABLED = "acceptEnabled";
const QString DialogController::MessageTemplate::KEY_REJECTTEXT = "rejectText";
const QString DialogController::MessageTemplate::KEY_DEFAULTBTN = "defaultBtn";
const QString DialogController::MessageTemplate::KEY_CONTENT = "content";
const QString DialogController::MessageTemplate::KEY_REJECTGROUP = "rejectGroup";
const QString DialogController::MessageTemplate::KEY_HIGHPRIORITY = "highPriority";
const QString DialogController::MessageTemplate::DIALOG_PROMPT_TYPE = "origin-dialog-content-prompt";
DialogController::MessageTemplate::MessageTemplate(const QString& aContentDirective, const QString& aTitle,
                                                   const QString& aText, const QString& aAcceptText, const QString& aRejectText,
                                                   const QString& aDefaultBtn, const QJsonObject& aContentInfo, QObject* aCallbackObj,
                                                   const char* aCallbackFunc, const QJsonObject& aPersistingInfo)
: contentDirective(aContentDirective), overrideId(""), title(aTitle), text(aText), acceptText(aAcceptText), acceptEnabled(true), rejectText(aRejectText)
, defaultBtn(aDefaultBtn), contentInfo(aContentInfo), callbackObj(aCallbackObj), callbackFunc(aCallbackFunc), persistingInfo(aPersistingInfo), highPriority(false)
{
    ORIGIN_ASSERT(defaultBtn == "yes" || defaultBtn == "no");
}


DialogController::MessageTemplate::MessageTemplate(const QString& aContentDirective, const QString& aTitle, 
                                                   const QString& aText, const QString& aRejectText, const QJsonObject& aContentInfo, 
                                                   QObject* aCallbackObj, const char* aCallbackFunc, const QJsonObject& aPersistingInfo)
: contentDirective(aContentDirective), overrideId(""), title(aTitle), text(aText), acceptText(""), acceptEnabled(true), rejectText(aRejectText)
, defaultBtn("no"), contentInfo(aContentInfo), callbackObj(aCallbackObj), callbackFunc(aCallbackFunc), persistingInfo(aPersistingInfo), highPriority(false)
{
    ORIGIN_ASSERT(defaultBtn == "yes" || defaultBtn == "no");
}


QString DialogController::MessageTemplate::toAttributeFriendlyText(const QString& text2clean)
{
    QString ret = text2clean;
    // toHtmlEscaped doesn't covert single quote
    ret = ret.replace("<b>", "<strong>");
    ret = ret.replace("</b>", "</strong>");
    ret = ret.replace("&quot;", "\"");
    ret = ret.replace('\'', "\&#39;");
    ret = ret.replace('<', "&lt;");
    ret = ret.replace('>', "&gt;");
    
    return ret;
}


QJsonObject DialogController::MessageTemplate::toDialogInfo()
{
    QJsonObject obj;
    obj[KEY_OVERRIDEID] = overrideId;
    obj[KEY_DIALOGTYPE] = DIALOG_PROMPT_TYPE;
    obj[KEY_CONTENTTYPE] = contentDirective;
    obj[KEY_TITLE] = toAttributeFriendlyText(title);
    obj[KEY_TEXT] = toAttributeFriendlyText(text);
    obj[KEY_ACCEPTTEXT] = acceptText;
    obj[KEY_ACCEPTENABLED] = acceptEnabled;
    obj[KEY_REJECTTEXT] = rejectText;
    obj[KEY_DEFAULTBTN] = defaultBtn;
    obj[KEY_CONTENT] = contentInfo;
    obj[KEY_REJECTGROUP] = rejectGroup;
    obj[KEY_HIGHPRIORITY] = highPriority;
    return obj;
}




DialogController* DialogControllerInstance = NULL;
DialogController::DialogController()
: QObject(NULL)
{

}


DialogController::~DialogController()
{
}


void DialogController::create()
{
    ORIGIN_ASSERT(DialogControllerInstance == NULL);
    DialogControllerInstance = new DialogController();
}


void DialogController::destroy()
{
    delete DialogControllerInstance;
}


DialogController* DialogController::instance()
{
    return DialogControllerInstance;
}


void DialogController::showMessageDialog(const DialogMode& mode, MessageTemplate mt)
{
    switch(mode)
    {
    case SPA:
        emit showDialog(mt.toDialogInfo(), mt.callbackObj, mt.callbackFunc.toStdString().c_str(), mt.persistingInfo);
        break;
    default:
        break;
    }
}


void DialogController::updateMessageDialog(const DialogMode& mode, MessageTemplate mt)
{
    switch(mode)
    {
    case SPA:
        emit updateDialog(mt.toDialogInfo(), mt.callbackObj, mt.callbackFunc.toStdString().c_str(), mt.persistingInfo);
        break;
    default:
        break;
    }
}


void DialogController::closeMessageDialog(const DialogMode& mode, const QString& overrideId)
{
    switch(mode)
    {
    case SPA:
        emit closeDialog(overrideId);
        break;
    default:
        break;
    }
}

}
}
