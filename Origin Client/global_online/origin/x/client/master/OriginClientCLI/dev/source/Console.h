//
//  Console.h
//  
//
//  Created by Kiss, Bela on 12-06-07.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#ifndef _Console_h
#define _Console_h

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>

#include "services/log/LogService.h"

class ConsoleWindow : public QWidget
{
    Q_OBJECT
    
public:
    ConsoleWindow();
    
    void setConfigLocation(const QString& configLocation);
    void submitCommand(const QString& commandText);
    
public slots:
    void onNewLogEntry_Handler(LogMsgType msgType, QString sFullFuncName, QString sLogMessage, QString sSourceFile, int fileLine);

    void onEditReturnPressed();
    void onConsoleMessage_Handler(QString message);
    
signals:
    void command(QStringList const& commandList);
    
private:
    QLabel *_someLabel;
    QLabel *_eacoreLabel;
    QLabel *_eacoreLocation;
    QLineEdit *_inputLine;
    QTextEdit *_logWindow;
    QPushButton *_submitButton;
};


class Console : public QObject
{
    Q_OBJECT
    
signals:

    void command(QStringList const& commandList);
    void consoleMessage(QString message);
    
public slots:

    void work();
    
public:
    static Console* Instance();
    
    void WriteLine(const QString& message);
    
    class ConsoleEntryStreamer
    {
    public:
        ConsoleEntryStreamer();
        ~ConsoleEntryStreamer();
        
        ConsoleEntryStreamer &	operator<< ( QChar t );
        ConsoleEntryStreamer &	operator<< ( bool t );
        ConsoleEntryStreamer &	operator<< ( char t );
        ConsoleEntryStreamer &	operator<< ( signed short i );
        ConsoleEntryStreamer &	operator<< ( unsigned short i );
        ConsoleEntryStreamer &	operator<< ( signed int i );
        ConsoleEntryStreamer &	operator<< ( unsigned int i );
        ConsoleEntryStreamer &	operator<< ( signed long l );
        ConsoleEntryStreamer &	operator<< ( unsigned long l );
        ConsoleEntryStreamer &	operator<< ( qint64 i );
        ConsoleEntryStreamer &	operator<< ( quint64 i );
        ConsoleEntryStreamer &	operator<< ( float f );
        ConsoleEntryStreamer &	operator<< ( double f );
        ConsoleEntryStreamer &	operator<< ( const char * s );
        ConsoleEntryStreamer &	operator<< ( const wchar_t * s );
        ConsoleEntryStreamer &	operator<< ( const QString & s );
        ConsoleEntryStreamer &	operator<< ( const QStringRef & s );
        ConsoleEntryStreamer &	operator<< ( const QLatin1String & s );
        ConsoleEntryStreamer &	operator<< ( const QByteArray & b );
        ConsoleEntryStreamer &	operator<< ( const void * p );
    private:
        QString mLogString;
    };
};

#define CONSOLE_WRITE  Console::ConsoleEntryStreamer()

#endif
