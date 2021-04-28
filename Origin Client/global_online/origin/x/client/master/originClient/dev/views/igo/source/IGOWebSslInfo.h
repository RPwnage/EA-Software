///////////////////////////////////////////////////////////////////////////////
// IGOWebSslInfo.h
// 
// Created by Steven Gieng
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOWEBSSLINFO_H
#define IGOWEBSSLINFO_H

#include <QDialog>

#include "services/plugin/PluginAPI.h"

namespace Ui{
    class IGOWebSslInfo;
}

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API IGOWebSslInfo : public QDialog
{
    Q_OBJECT

public:
    IGOWebSslInfo(QWidget *parent = 0, Qt::WindowFlags f = 0);
    virtual ~IGOWebSslInfo();

    void setHostName(const QString& hostName);
    void setSubject(const QString& subject);
    void setIssuer(const QString& issuer);
    void setValidity(const QString& from, const QString& to);
    void setEncryption(const QString& encryption);

private:
    Ui::IGOWebSslInfo* ui;
};

} // Client
} // Origin

#endif