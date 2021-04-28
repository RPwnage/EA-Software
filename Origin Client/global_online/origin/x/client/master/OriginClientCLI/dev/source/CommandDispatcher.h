//
//  CommandDispatcher.h
//  
//
//  Created by Kiss, Bela on 12-06-08.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#ifndef _CommandDispatcher_h
#define _CommandDispatcher_h

#include <QObject>
#include <QMap>
#include <QString>
#include <QPair>
#include <QList>

#include "Command.h"

class QStringList;

class CommandDispatcher : public QObject
{
    Q_OBJECT
    
public:

    CommandDispatcher();
    
    void add(Command const& command);
    
    /// Causes the command dispatcher to pass the next line of text to the specified slot.
    void takeInputFromNextLine(QObject* receiver, const char* slot);
    
public slots:

    void onCommand(QStringList const& args);
    
private:
    
    typedef QMap< QString, Command > Commands;
    Commands mCommands;
    
    typedef QPair< QObject*, const char* > InputReceiver;
    typedef QList<InputReceiver> InputReceivers;
    InputReceivers mInputReceivers;
};

#endif
