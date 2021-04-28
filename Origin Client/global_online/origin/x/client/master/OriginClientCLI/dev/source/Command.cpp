//
//  Command.cpp
//  
//
//  Created by Trent Tuggle on 6/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>

#include "Command.h"

/////////////////////////////////////////////////////////////////////////
//
// Command

Command::Command()
: QObject()
, mName()
, mConfiguredOptions()
, mHandler(NULL)
{
}

Command::Command(QString _name, QString _options, CommandHandler _handler)
: QObject()
, mName(_name)
, mConfiguredOptions(_options)
, mHandler(_handler)
{
}

Command::Command(Command const& from)
: mName(from.mName)
, mConfiguredOptions(from.mConfiguredOptions)
, mHandler(from.mHandler)
{
}

Command& Command::operator=(Command const& from)
{
    mName = from.mName;
    mConfiguredOptions = from.mConfiguredOptions;
    mHandler = from.mHandler;
    return *this;
}

void Command::operator()(QStringList const& options)
{
    if (mHandler != NULL)
        (*mHandler)(options);
}

