/////////////////////////////////////////////////////////////////////////////
// EmailVerificationMessageAreaViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef EMAILVERIFICATIONMESSAGEAREAVIEWCONTROLLER_H
#define EMAILVERIFICATIONMESSAGEAREAVIEWCONTROLLER_H

#include "MessageAreaViewControllerBase.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class ClientMessageAreaView;
/// \brief Message that lets the user know they need to set their security question.
class ORIGIN_PLUGIN_API EmailVerificationMessageAreaViewController : public MessageAreaViewControllerBase
{
	Q_OBJECT

public:
	EmailVerificationMessageAreaViewController(QObject* parent = NULL);
	~EmailVerificationMessageAreaViewController();
    QWidget* view();

    /// \brief Returns true if it's ok to show this message.
    bool okToShowMessage();

    void showMessageSentWindow();
    int showMessageErrorWindow();

signals:
    /// \brief Emitted when the Set Security Question button is pressed.
    void showAccountSettings();

    /// \brief Emitted when the Dismiss button is pressed.
    void dismiss();

    /// \brief Emitted when the response from the server has been received.
    void responseReceived();


private slots:
    /// \brief Returns the response of our query to the server.
    void onInfoReceived();

    /// \brief Sends telemetry when the user elects to set the security question
    void onSetQuestionNowSelected();

    /// \brief Sends telemetry when the user elects to postpone setting the security question
    void onDismissQuestionSelected();

    /// \brief called when the send email verification request is completed
    void onSendEmailRequestComplete();

private:
    /// \brief Calls to the server asking if the user's email has been verified.
    void queryInfo();

    /// \brief initaites the rest call to request the sending of the verfication email
    void sendVerificationEmailRequest();

	ClientMessageAreaView* mClientMessageAreaView;
    bool mEmailVerified;
};
}
}
#endif