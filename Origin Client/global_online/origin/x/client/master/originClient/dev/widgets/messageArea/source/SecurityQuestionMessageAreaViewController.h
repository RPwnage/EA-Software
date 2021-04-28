/////////////////////////////////////////////////////////////////////////////
// SecurityQuestionMessageAreaViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SECURITYQUESTIONMESSAGEAREAVIEWCONTROLLER_H
#define SECURITYQUESTIONMESSAGEAREAVIEWCONTROLLER_H

#include "MessageAreaViewControllerBase.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
class ClientMessageAreaView;
/// \brief Message that lets the user know they need to set their security question.
class ORIGIN_PLUGIN_API SecurityQuestionMessageAreaViewController : public MessageAreaViewControllerBase
{
	Q_OBJECT

public:
	SecurityQuestionMessageAreaViewController(QObject* parent);
	~SecurityQuestionMessageAreaViewController();
    QWidget* view();

    /// \brief Returns true if it's ok to show this message.
    bool okToShowMessage();

signals:
    /// \brief Emitted when the Set Security Question button is pressed.
    void showAccountSettings();

    /// \brief Emitted when the Dismiss button is pressed.
    void dismiss();

    /// \brief Emitted when the response from the server has been received.
    void responseReceived();


private slots:
    /// \brief Returns the response of our query to the server.
    void onSecurityQAReceived();

    /// \brief Sends telemetry when the user elects to set the security question
    void onSetQuestionNowSelected();

    /// \brief Sends telemetry when the user elects to postpone setting the security question
    void onDismissQuestionSelected();

private:
    /// \brief Calls to the server asking if the user's security question has been set.
    void querySecurityQuestionInfo();

	ClientMessageAreaView* mClientMessageAreaView;
    bool mSecurityQASet;
};
}
}
#endif