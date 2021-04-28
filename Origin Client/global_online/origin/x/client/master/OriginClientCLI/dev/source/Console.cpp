//
//  Console.cpp
//  
//
//  Created by Kiss, Bela on 12-06-07.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#include "Console.h"

#include <QtCore>

#include <iostream>

#include <QLayout>

#include "services/debug/DebugService.h"

ConsoleWindow::ConsoleWindow()
{
    _someLabel = new QLabel("Enter Command:", this);
    _eacoreLabel = new QLabel("Loading config from: ", this);
    _eacoreLocation = new QLabel("", this);
    _inputLine = new QLineEdit(this);
    _logWindow = new QTextEdit(this);
    _submitButton = new QPushButton(this);
    
    _submitButton->setText("Submit");
    _logWindow->setReadOnly(true);
    
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    QHBoxLayout *line1 = new QHBoxLayout();
    line1->addWidget(_someLabel);
    line1->addWidget(_inputLine);
    line1->addWidget(_submitButton);
    vLayout->addItem(line1);
    QHBoxLayout *line2 = new QHBoxLayout();
    line2->addWidget(_eacoreLabel);
    line2->addWidget(_eacoreLocation);
    vLayout->addItem(line2);
    QHBoxLayout *line3 = new QHBoxLayout();
    line3->addWidget(_logWindow);
    vLayout->addItem(line3);
    
    
    this->setLayout(vLayout);
    
    this->setWindowTitle("OriginClientCLI");

    this->setMinimumSize(1000, 150);
    
    ORIGIN_VERIFY_CONNECT(_inputLine, SIGNAL(returnPressed()), this, SLOT(onEditReturnPressed()));
}

void ConsoleWindow::setConfigLocation(const QString& configLocation)
{
    QByteArray excludes("/");
    QString linkCompatibleUrl = "file://localhost" + QString(QUrl::toPercentEncoding(configLocation, excludes));
    
    _eacoreLocation->setText(QString("<html> <a style='text-decoration:none;text-align:left' href='%1'>%2</a></html>").arg(linkCompatibleUrl).arg(configLocation));
}

void ConsoleWindow::submitCommand(const QString& commandText)
{
    QStringList args(commandText.split(QRegExp("\\s+")));
    emit command(args);
}

void ConsoleWindow::onEditReturnPressed()
{
    QString commandText = _inputLine->text();
    if (!commandText.isEmpty())
    {
        submitCommand(commandText);
        _inputLine->setText("");
    }
}

void ConsoleWindow::onNewLogEntry_Handler(LogMsgType msgType, QString sFullFuncName, QString sLogMessage, QString sSourceFile, int fileLine)
{
    _logWindow->append("[" + sFullFuncName + "]: " + sLogMessage);
}

void ConsoleWindow::onConsoleMessage_Handler(QString message)
{
    _logWindow->append(message);
}

void Console::work()
{
    QTextStream qin(stdin, QFile::ReadOnly);
    for (bool done = false; !done; )
    {
//        std::cout << ">" << std::flush;
        
        QString input = qin.readLine();
        if (!input.isNull())
        {
            QStringList args(input.split(QRegExp("\\s+")));
            emit command(args);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//	LOGGER
///////////////////////////////////////////////////////////////////////////////

// Singleton static instance - note that this isn't a static member variable,
// just static (internal) linkage.
static Console* sConsole = NULL;

Console* Console::Instance()
{
    if (sConsole == NULL)
        sConsole = new Console();
    return sConsole;
}

void Console::WriteLine(const QString& message)
{
    std::cout << qPrintable(message);
    
    emit (consoleMessage(message));
}

Console::ConsoleEntryStreamer::ConsoleEntryStreamer() 
{
}

Console::ConsoleEntryStreamer::~ConsoleEntryStreamer()
{
    Console::Instance()->WriteLine(mLogString);
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( QChar t )
{
    mLogString = mLogString.append(t);
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( bool t )
{
    mLogString = mLogString.append(t ? "true" : "false");
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( char t )
{
    mLogString = mLogString.append(QChar(t));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( signed short i )
{
    mLogString = mLogString.append(QString::number(i));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( unsigned short i )
{
    mLogString = mLogString.append(QString::number(i));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( signed int i )
{
    mLogString = mLogString.append(QString::number(i));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( unsigned int i )
{
    mLogString = mLogString.append(QString::number(i));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( signed long l )
{
    mLogString = mLogString.append(QString::number(l));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( unsigned long l )
{
    mLogString = mLogString.append(QString::number(l));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( qint64 i )
{
    mLogString = mLogString.append(QString::number(i));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( quint64 i )
{
    mLogString = mLogString.append(QString::number(i));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( float f )
{
    mLogString = mLogString.append(QString::number(f));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( double f )
{
    mLogString = mLogString.append(QString::number(f));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( const char * s )
{
    mLogString = mLogString.append(s);
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( const wchar_t * s )
{
    mLogString = mLogString.append(QString::fromWCharArray(s));
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( const QString & s )
{
    mLogString = mLogString.append(s);
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( const QStringRef & s )
{
    mLogString = mLogString.append(s);
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( const QLatin1String & s )
{
    mLogString = mLogString.append(s);
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( const QByteArray & b )
{
    mLogString = mLogString.append(b);
    return *this;
}

Console::ConsoleEntryStreamer &	Console::ConsoleEntryStreamer::operator<< ( const void * p )
{
    mLogString = mLogString.append(QString("0x%1").arg((unsigned long)p, 8, 16, QChar('0')));
    return *this;
}
