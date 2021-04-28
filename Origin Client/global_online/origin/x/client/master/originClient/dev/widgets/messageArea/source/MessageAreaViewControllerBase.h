/////////////////////////////////////////////////////////////////////////////
// MessageAreaViewControllerBase.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MESSAGEAREAVIEWCONTROLLERBASE_H
#define MESSAGEAREAVIEWCONTROLLERBASE_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

class QWidget;

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API MessageAreaViewControllerBase : public QObject
{
	Q_OBJECT

public:
    enum MessageType
    {
        Offline,
        SecurityQuestion,
        EmailVerification,
        Subscription
    };
    MessageAreaViewControllerBase(const MessageType& type, QObject* parent = 0) 
    : QObject(parent), mType(type) {}

	virtual QWidget* view() = 0;
    MessageType messageType() const {return mType;}

protected:
    const MessageType mType;
};
}
}
#endif