//
//  Command.h
//  
//
//  Created by Kiss, Bela on 12-06-05.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#ifndef _Command_h
#define _Command_h

#include <map>
#include <string>
#include <vector>

#include <QObject>
#include <QStringList>

enum CommandResult
{
    RESULT_EXIT = -1,
    RESULT_SUCCESS,
    RESULT_ERROR
};

typedef void (*CommandHandler)(QStringList const& options);

class Command : public QObject
{
    Q_OBJECT
    
    public:
    
        Command();
        Command(QString _name, QString _options, CommandHandler _handler);
        Command(Command const& from);

        virtual Command& operator=(Command const& from);
        void operator()(QStringList const& options);
    
        QString const& name() const;
        
    private:
    
        QString mName;
        QStringList mConfiguredOptions;
        CommandHandler mHandler;
};

inline QString const& Command::name() const
{
    return mName;
}

#endif
