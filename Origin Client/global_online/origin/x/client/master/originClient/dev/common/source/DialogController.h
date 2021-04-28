/////////////////////////////////////////////////////////////////////////////
// DialogController.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef DIALOG_CONTROLLER_H
#define DIALOG_CONTROLLER_H
#include <QObject>
#include "services/plugin/PluginAPI.h"
#include <QJsonObject>

namespace Origin
{
namespace Client
{
class OriginWebController;

class ORIGIN_PLUGIN_API DialogController : public QObject
{
    Q_OBJECT
public:
    enum DialogMode
    {
        SPA = 0,
        OIG,
        WebWidget
    };

    static void create();
    static DialogController *instance();
    static void destroy();

    struct MessageTemplate
    {
        static const QString KEY_DIALOGTYPE;
        static const QString KEY_CONTENTTYPE;
        static const QString KEY_OVERRIDEID;
        static const QString KEY_TITLE;
        static const QString KEY_TEXT;
        static const QString KEY_ACCEPTTEXT;
        static const QString KEY_ACCEPTENABLED;
        static const QString KEY_REJECTTEXT;
        static const QString KEY_DEFAULTBTN;
        static const QString KEY_CONTENT;
        static const QString KEY_REJECTGROUP;
        static const QString KEY_HIGHPRIORITY;
        static const QString DIALOG_PROMPT_TYPE;
        QString overrideId;
        QString title;
        QString text;
        QString acceptText;
        bool acceptEnabled;
        QString rejectText;
        QString defaultBtn;
        QObject* callbackObj;
        QString callbackFunc;
        QJsonObject persistingInfo;
        QJsonObject contentInfo;
        QString contentDirective;
        QString rejectGroup;
        bool highPriority;
        QJsonObject toDialogInfo();
        static QString toAttributeFriendlyText(const QString& text);
        MessageTemplate() {}
        // Basic prompt
        MessageTemplate(const QString& aContentDirective, const QString& aTitle,
                        const QString& aText, const QString& aAcceptText, const QString& aRejectText,
                        const QString& aDefaultBtn = "no", const QJsonObject& aContentInfo = QJsonObject(), QObject* aCallbackObj = NULL,
                        const char* aCallbackFunc = "", const QJsonObject& aPersistingInfo = QJsonObject());
        // Basic alert
        MessageTemplate(const QString& aContentDirective, const QString& aTitle, 
                        const QString& aText, const QString& aRejectText, const QJsonObject& aContentInfo = QJsonObject(), 
                        QObject* aCallbackObj = NULL, const char* aCallbackFunc = "", const QJsonObject& aPersistingInfo = QJsonObject());
    };

    void showMessageDialog(const DialogMode& mode, MessageTemplate mt);
    void updateMessageDialog(const DialogMode& mode, MessageTemplate mt);
    void closeMessageDialog(const DialogMode& mode, const QString& overrideId);

signals:
    void showDialog(const QJsonObject& dialogInfo, QObject* callbackObj = NULL, const char* callbackFunc = "", const QJsonObject& persistingInfo = QJsonObject());
    void updateDialog(const QJsonObject& dialogInfo, QObject* callbackObj = NULL, const char* callbackFunc = "", const QJsonObject& persistingInfo = QJsonObject());
    void closeDialog(const QString& overrideId);
    void closeGroup(const QString& rejectGroup);
    void linkClicked(const QJsonObject& args);

private:
    DialogController();
    ~DialogController();
};

}
}
#endif //DIALOG_CONTROLLER_H