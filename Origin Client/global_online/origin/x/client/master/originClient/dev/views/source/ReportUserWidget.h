// *********************************************************************
//  ReportUserWidget.h
//  Copyright (c) 2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#ifndef REPORT_USER_WIDGET_H
#define REPORT_USER_WIDGET_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Origin 
{
    namespace UIToolkit 
    {
        class OriginWindow;
    }
}

namespace Ui
{
    class ReportUserWidget;
}

#define REPORT_CHILD_SOLICITATION_STR "CHILD_SOLICITATION"
#define REPORT_TERRORIST_THREAT_STR "TERRORIST_THREAT"
#define REPORT_HARASSMENT_STR "HARASSMENT"
#define REPORT_SPAM_STR "SPAM"
#define REPORT_CHEATING_STR "CHEATING"
#define REPORT_OFFENSIVE_CONTENT_STR "OFFENSIVE_CONTENT"
#define REPORT_SUICIDE_THREAT_STR "SUICIDE_THREAT"
#define REPORT_CLIENT_HACK_STR "CLIENT_HACK"
#define REPORT_NAME_STR "NAME"
#define REPORT_AVATAR_STR "AVATAR"
#define REPORT_CUSTOM_GAME_NAME_STR "CUSTOM_GAME_NAME"
#define REPORT_CHAT_ROOM_NAME_STR "CHAT_ROOM_NAME"
#define REPORT_IN_GAME_STR "IN_GAME"

class ORIGIN_PLUGIN_API ReportUserWidget : public QWidget
{
    Q_OBJECT

public:
    ReportUserWidget(bool fromSDK, const QString& username, QWidget* parent = NULL);
    ~ReportUserWidget();

    // what is the reason that the user is being reported
    QString reportReason();
    // where did the offense happen
    QString reportWhere();
    // Additional comments from the user
    QString reportComments();

    // hides the contents of the widget when the report has been submitted
    void reportSubmitted();

signals:

    // emitted whenever a reason or the where is chosen
    // true if both have been chosen, false if either one are still at "choose one"
    void reasonAndWhereChosen(bool);

private slots:
    // callback if the user changed one of the combos
    void dropdownChanged(int);

private:

    Ui::ReportUserWidget* ui;
};

class ORIGIN_PLUGIN_API ReportUserDialog : public QObject
{
    Q_OBJECT

public:
    ReportUserDialog(QString userName,  quint64 id, const QString& masterTitle);

    // returns the window
    Origin::UIToolkit::OriginWindow* getWindow() {return mWindow;}

    // returns the widget that makes up the window
    ReportUserWidget* getReportUserWidget() {return mReportUserWidget;}

public slots:
    // the report has been submitted, update the dialog to reflect that.
    void onReportSubmitted();

signals:
    // user clicked on submit, we've gathered the data, signal out to submit the report
    void submitReportUser(quint64 /*mId*/, QString /*reasonStr*/, QString /*whereStr*/, QString /*masterTitle*/, QString /*comments*/);
    // do the report user dialog delete
    void reportUserDialogDelete();

private slots:
    // update the submit button, true to enable, false to disable
    void updateButtons(bool);
    // the user has hit the submit so gather the data
    void onReportRequested();
    // check if we should close the dialog (submit does not close it)
    void checkClose(int);

private:
     quint64 mId;
     QString mMasterTitle;
     Origin::UIToolkit::OriginWindow* mWindow;
     ReportUserWidget* mReportUserWidget;
    
     // setup the initial window
     void initWindow(QString userName);

};

#endif // REPORT_USER_WIDGET_H