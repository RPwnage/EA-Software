//
//  CommandDispatcher.cpp
//  
//
//  Created by Trent Tuggle on 6/28/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>

#include "CommandDispatcher.h"
#include "Console.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////
//
// CommandDispatcher

CommandDispatcher::CommandDispatcher()
{
}

void CommandDispatcher::add(Command const& command)
{
    Commands::const_iterator where = mCommands.find(command.name());
    if (where != mCommands.end())
        return;
    
    mCommands[command.name()] = command;
}

void CommandDispatcher::onCommand(QStringList const& args)
{
    if (mInputReceivers.size())
    {
        InputReceiver r = mInputReceivers.takeLast();
        QMetaObject::invokeMethod(r.first, r.second, Q_ARG(QStringList, args));
    }
    else
    {
        QString const& command = args[0];
        
        Commands::iterator where = mCommands.find(command);
        if (where == mCommands.end())
        {
            CONSOLE_WRITE << "unknown command, '" << command.toLatin1().constData() << "'";
            return;
        }
        
        where.value()(args);
    }
}

void CommandDispatcher::takeInputFromNextLine(QObject* receiver, const char* slot)
{
    mInputReceivers.append(InputReceiver( receiver, slot ));
}

