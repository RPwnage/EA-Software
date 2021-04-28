///////////////////////////////////////////////////////////////////////////////
// IGOWebSslInfo.cpp
// 
// Created by Steven Gieng
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOWebSslInfo.h"
#include "ui_IGOWebSslInfo.h"

namespace Origin
{
namespace Client
{

IGOWebSslInfo::IGOWebSslInfo(QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    ui(NULL)
{
    ui = new Ui::IGOWebSslInfo();
    ui->setupUi(this);

    // set text
    ui->label_cert_info->setText(tr("ebisu_client_certificate_info")); // "Certificate Information"
}

IGOWebSslInfo::~IGOWebSslInfo()
{
    delete ui;
    ui = NULL;
}

void IGOWebSslInfo::setHostName(const QString& hostName)
{
    ui->label_connected_to->setText(tr("ebisu_client_securely_connected_to_website_CAPS").arg(hostName)); // "<b>YOU ARE SECURELY CONNECTED TO</b><br>%1"
}

void IGOWebSslInfo::setSubject(const QString& subject)
{
    ui->label_issued_to->setText(tr("ebisu_client_issued_to").arg(subject)); // "<b>Issued to:</b> %1"
}

void IGOWebSslInfo::setIssuer(const QString& issuer)
{
    ui->label_issued_by->setText(tr("ebisu_client_issued_by").arg(issuer)); // "<b>Issued by:</b> %1"
}

void IGOWebSslInfo::setValidity(const QString& from, const QString& to)
{
    ui->label_validity->setText(tr("ebisu_client_validity_range").arg(from).arg(to)); // "<b>Valid from:</b> %1 to %2"
}

void IGOWebSslInfo::setEncryption(const QString& encryptionStrength)
{
    ui->label_encryption->setText(tr("ebisu_client_connection_encryption_strength").arg(encryptionStrength)); // "Your connection to this site is encrypted with<br>%1-bit encryption."
}

} // Client
} // Origin

