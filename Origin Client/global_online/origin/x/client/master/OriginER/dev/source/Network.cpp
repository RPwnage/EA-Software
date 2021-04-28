///////////////////////////////////////////////////////////////////////////////
// Network.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Network.h"
#include "DiagnosticApplication.h"
#include "ErrorHandler.h"
#include "DataCollector.h"
#include "ReportErrorView.h"
#include "GzipCompress.h"
#include "services/debug/DebugService.h"

#include <QNetworkProxyFactory>
#include <QSslConfiguration>
#include <QDomDocument>
#include <QMessageBox>
#include <QDateTime>
#include <assert.h>

Network::Network(DiagnosticApplication* app)
    : m_url(app->hasIniValue("URL") ? app->getIniValue("URL") : PROD_URL)
    , m_debugVerifyPeer( app->getIniValue("ValidatePeer").toUpper() != "FALSE") // default to true
    , m_debugFailRequest(app->getIniValue("FailRequest").toUpper()  == "TRUE")  // default to false
    , m_debugFailUpload( app->getIniValue("FailUpload").toUpper()   == "TRUE")  // default to false
    , m_debugDumpData(   app->getIniValue("DumpData").toUpper()     == "TRUE")  // default to false
    , m_errorHandler(0)
    , m_dataCollector(0)
    , m_reportView(0)
    , m_diagApp(app)
    , m_networkAccessManager(0)
{
    if ( app->getIniValue("PopUp").toUpper() == "TRUE" ) // default to false
    {
        QMessageBox::information(0, "OriginER INI settings", 
            QString(" URL = ") + m_url 
            + "\n Locale = " + app->getLocale()
            + "\n ValidatePeer = " + (m_debugVerifyPeer ? "true" : "false") 
            + "\n FailRequest = " + (m_debugFailRequest ? "true" : "false") 
            + "\n FailUpload = " + (m_debugFailUpload ? "true" : "false") 
            + "\n DumpData = " + (m_debugDumpData ? "true" : "false") 
            + "\n This box is shown because the ini file contains the key PopUp"
        );
    }
    // these are needed since queued connections are used, see below.
    qRegisterMetaType<QList<QSslError> >("QList<QSslError>");
    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");

    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // Get our current SSL config, code copied and modified from 
    // \global_online\ebisu\client\main\EbisuCommon\EbisuNetworkAccessManagerFactory.cpp
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    assert( QSslSocket::AutoVerifyPeer == sslConfig.peerVerifyMode() );

    if ( !m_debugVerifyPeer )
    {
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    }

    m_trustedCerts = sslConfig.caCertificates();

    // Probably the only ones we need are these two
    // And a VeriSign cert for Live
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/VeriSignG5.cer"));
    // And Entrust Amazon S3/cloud saves
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/Entrust.cer"));
    // Add the GeoTrust cert used for Common Services
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/GeoTrust_Global_CA.cer"));
    // And the AddTrust cert used for Omniture
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/AddTrust_External_CA.cer"));
    // And the UserTrust cert used for Playstation Network
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/UserTrust_CA.cer"));
    // And the PositiveSSL cert also used by PSN (?)
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/PositiveSSL.cer"));
    // And DigiCert used by Facebook
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/DigiCertHighAssuranceCA3.cer"));
    m_trustedCerts.append(readCertificateFile(":/Resources/certificates/DigiCertHighAssuranceEVRootCA.cer"));

    // Set our new cert chain
    sslConfig.setCaCertificates(m_trustedCerts);

    // Apply our config
    QSslConfiguration::setDefaultConfiguration(sslConfig);
}

Network::~Network(void)
{
    if(m_networkAccessManager)
    {
        ORIGIN_VERIFY_DISCONNECT(m_networkAccessManager , 0, this, 0);
        m_networkAccessManager->deleteLater();
        m_networkAccessManager = NULL;
    }
}

QString const& Network::getUrl()
{
    return m_url;
}

QList<QSslCertificate> Network::readCertificateFile(const QString &filename)
{
    QList<QSslCertificate> ret; 
    QFile certFile(filename);
    if (certFile.exists() && certFile.open(QIODevice::ReadOnly))
    {
        ret = QSslCertificate::fromDevice(&certFile, QSsl::Pem); 
        // Make sure we can parse the cert
        assert(ret.length() >= 1);
        assert(!ret[0].isNull());
        assert(!ret[0].isBlacklisted());

#ifndef _DEBUG
        assert(QDateTime::currentDateTime() <  ret[0].expiryDate());
        assert(QDateTime::currentDateTime() >=  ret[0].effectiveDate());
#endif // _DEBUG

    }
    return ret;
}

void Network::setErrorHandler(ErrorHandler* errorHandler)
{
    assert(errorHandler);
    m_errorHandler = errorHandler;
}

ErrorHandler* Network::getErrorHandler()
{
    assert(m_errorHandler);
    return m_errorHandler;
}

void Network::setDataCollector(DataCollector* dataCollector)
{
    assert(dataCollector);
    m_dataCollector = dataCollector;
}

DataCollector* Network::getDataCollector()
{
    assert(m_dataCollector);
    return m_dataCollector;
}

void Network::setReportView(ReportErrorView* dialog)
{
    assert(dialog);
    m_reportView = dialog;
}

ReportErrorView* Network::getReportView()
{
    assert(m_reportView);
    return m_reportView;
}

QString const& Network::getTrackingNumber()
{
    return m_trackingNumber;
}

QByteArray const& Network::getPayload()
{
    return getDataCollector()->getDiagnosticDataCompressed();
}

QString const& Network::getPayloadMd5()
{
    if ( m_payloadMd5.isEmpty() )
    {
        QCryptographicHash h(QCryptographicHash::Md5);
        h.addData(getPayload());
        m_payloadMd5 = h.result().toBase64();
    }
    return m_payloadMd5;
}

QString Network::getSignedUrlRequestXML()
{
    // TODO add <?xml version="1.0" encoding="UTF-8"?>
    QDomDocument doc;

    QDomElement root = doc.createElement("reportRequest");
    doc.appendChild(root);

    QDomElement tag = doc.createElement("originId");
    root.appendChild(tag);
    QDomText text = doc.createTextNode(getReportView()->getUserId());
    tag.appendChild(text);

    tag = doc.createElement("note");
    root.appendChild(tag);
    text = doc.createTextNode(getReportView()->getMessage());
    tag.appendChild(text);

    tag = doc.createElement("category");
    root.appendChild(tag);
    text = doc.createTextNode(getReportView()->getProblemArea());
    tag.appendChild(text);

    tag = doc.createElement("cenumber");
    root.appendChild(tag);
    text = doc.createTextNode(getReportView()->getCeNumber());
    tag.appendChild(text);

    tag = doc.createElement("subscriber");
    root.appendChild(tag);
    text = doc.createTextNode(m_diagApp->m_cmdline_sub);
    tag.appendChild(text);

    tag = doc.createElement("launchSource");
    root.appendChild(tag);
    if(m_diagApp)
        text = doc.createTextNode(m_diagApp->m_cmdline_source);
    else
        text = doc.createTextNode("N/A");
    tag.appendChild(text);

    tag = doc.createElement("ejob");
    root.appendChild(tag);
    if(m_diagApp)
        text = doc.createTextNode(m_diagApp->m_download_ejob);
    else
        text = doc.createTextNode("");
    tag.appendChild(text);

    tag = doc.createElement("esys");
    root.appendChild(tag);
    if(m_diagApp)
        text = doc.createTextNode(m_diagApp->m_download_esys);
    else
        text = doc.createTextNode("");
    tag.appendChild(text);

    tag = doc.createElement("offerid");
    root.appendChild(tag);
    if(m_diagApp)
        text = doc.createTextNode(m_diagApp->m_download_offerid);
    else
        text = doc.createTextNode("");
    tag.appendChild(text);

    QDomElement files = doc.createElement("files");
    root.appendChild(files);

    QDomElement file = doc.createElement("file");
    files.appendChild(file);

    tag = doc.createElement("name");
    file.appendChild(tag);
    text = doc.createTextNode("dump.tar.gz");
    tag.appendChild(text);

    tag = doc.createElement("size");
    file.appendChild(tag);
    text = doc.createTextNode(QString::number(getPayload().size()));
    tag.appendChild(text);

    tag = doc.createElement("contentType");
    file.appendChild(tag);
    text = doc.createTextNode("x-application/gzip");
    tag.appendChild(text);

    tag = doc.createElement("b64Md5");
    file.appendChild(tag);
    text = doc.createTextNode(getPayloadMd5());
    tag.appendChild(text);

    return doc.toString();
}

// static
bool Network::testComputeHash()
{
    QString body = "<simpleBody></simpleBody>";
    QString timestamp = "1320777827064";
    QString machineHash = "F799ECEC46E8F468";
    QString expected = "45116";

    QString computed = computeHash( body, machineHash, timestamp );

    return computed == expected;
}

// static
QString Network::computeHash( QString const& body, QString const& machineHash, QString const& timestamp )
{
    QString stringToHash = machineHash + timestamp + body;
    QByteArray stream = stringToHash.toUtf8();
    unsigned short salt =  (('r' & 0xff) << 8) | ('e' & 0xff);
    unsigned short hash = 0;
    for (int i = 0; i < stream.length(); ++i ) 
    {
        hash += (hash << 2) + stream.at(i);
        if ((i & 1) > 0) 
            hash ^= salt;
    }
    return QString::number(hash);
}

// slot
void Network::requestSignedURL()
{
    if ( getPayload().size() > MAX_UPLOAD_SIZE )
        getErrorHandler()->addError("Upload file size (compressed) larger than 10MB");

    if ( !getErrorHandler()->isOk() )
        return;

    assert(testComputeHash());

    QString body        = getSignedUrlRequestXML();
    QString machineHash = getDataCollector()->getMachineHash();
    QString timestamp   = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString hash        = computeHash(body, machineHash, timestamp);
    QString version     = getDataCollector()->getOriginVersion();
    
    QUrl url(getUrl());
    QNetworkRequest req(url);

    req.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    req.setRawHeader("x-machine-hash", machineHash.toUtf8());
    req.setRawHeader("x-timestamp", timestamp.toUtf8());
    req.setRawHeader("x-origin-client-version", version.toUtf8());
    req.setRawHeader("User-Agent", getUserAgent().toUtf8());
    req.setRawHeader("osVersion", getDataCollector()->m_OSVersionString.toUtf8());
    if ( m_debugFailRequest )
        req.setRawHeader("x-origin-hash", "DEBUGFAIL");
    else
        req.setRawHeader("x-origin-hash", hash.toUtf8());

    m_postData = body.toUtf8();
    m_resultEmitted = false;

    // create network access manager so that it's created on the QThread
    m_networkAccessManager = new QNetworkAccessManager();

    m_networkReply = m_networkAccessManager->post(req, m_postData);

    // queued connect to work arount Qt bug, see
    // http://stackoverflow.com/questions/7650978/qnetworkreply-emits-error-signal-twice-when-contentnotfounderror-occures-when-ev
    ORIGIN_VERIFY_CONNECT_EX(   m_networkReply, SIGNAL(finished()), 
                this, SLOT(initiateUploadToCloud()),
                Qt::QueuedConnection
            );
    ORIGIN_VERIFY_CONNECT_EX(   m_networkReply, SIGNAL(error(QNetworkReply::NetworkError)), 
                this, SLOT(onRequestSignedURLError(QNetworkReply::NetworkError)),
                Qt::QueuedConnection
            );
    // queued connect not used here, since this would make ignoring the error impossible
    ORIGIN_VERIFY_CONNECT_EX(   m_networkAccessManager , SIGNAL(sslErrors(QNetworkReply* , const QList<QSslError>&)), 
                this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError>&)),
                Qt::DirectConnection
            );
}

QString Network::getUserAgent() const
{
    return QString(EBISU_VERSION_STR).replace(",", ".").prepend("Origin Error Reporter/");
}

void Network::initiateUploadToCloud()
{
    m_trackingNumber = "";
    QString signedUrl;
    QDomDocument doc;
    QString errorMessage;
    int errorLine;
    int errorColumn;
    // QDomDocument tries to determine the encoding from the XML data
    // TODO the server should ideally include UTF-8 in a header, as in 
    // <?xml version="1.0" encoding="UTF-8"?>
    if ( doc.setContent(m_networkReply->readAll(), false, &errorMessage, &errorLine, &errorColumn) )
    {
        QDomElement root = doc.documentElement();
        QDomNodeList id = root.elementsByTagName("trackingId");
        if ( id.size() == 1 && id.item(0).isElement() )
        {
            // getting the tracking number
            m_trackingNumber = id.item(0).toElement().text();
            QDomNodeList puts = root.elementsByTagName("puts");
            if ( puts.size() == 1 && puts.item(0).isElement() ) 
            {
                QDomNodeList put = puts.item(0).toElement().elementsByTagName("put");
                if ( put.size() == 1 && put.item(0).isElement() ) 
                {
                    QDomNodeList url = put.item(0).toElement().elementsByTagName("signedUrl");
                    if ( url.size() == 1 && url.item(0).isElement() ) 
                    {
                        // get the signed url
                        signedUrl = url.item(0).toElement().text();
                    }
                }
            }
        }
    } 
    else 
    {
        getErrorHandler()->addError(QString("Signed URL reply XML parsing error %1 at line %2 char %3").arg(errorMessage).arg(errorLine).arg(errorColumn));
        showResultWindow();
        return;
    }
    if ( m_trackingNumber == "" || signedUrl == "" )
    {
        getErrorHandler()->addError("Error parsing signed URL reply");
        showResultWindow();
        return;
    }
    if ( m_debugDumpData )
    {
        QFile f(m_trackingNumber + ".gzip");
        f.open(QIODevice::WriteOnly);
        f.write(QByteArray(getPayload()));
    }

    QUrl url = QUrl::fromEncoded(signedUrl.toLatin1(), QUrl::StrictMode);
    QNetworkRequest req(url);

    QString sizeStr = QString::number(getPayload().size());
    req.setRawHeader("x-amz-meta-size", sizeStr.toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "x-application/gzip");
    if ( m_debugFailUpload )
        req.setRawHeader("Content-MD5", "DEBUGFAIL");
    else
        req.setRawHeader("Content-MD5", getPayloadMd5().toUtf8());

    m_postData = getPayload();
    m_resultEmitted = false;
    m_networkReply = m_networkAccessManager->put(req, m_postData);

    // queued connect to work arount Qt bug, see
    // http://stackoverflow.com/questions/7650978/qnetworkreply-emits-error-signal-twice-when-contentnotfounderror-occures-when-ev
    ORIGIN_VERIFY_CONNECT_EX(   m_networkReply, SIGNAL(finished()), 
                this, SLOT(onFinished()),
                Qt::QueuedConnection
            );
    ORIGIN_VERIFY_CONNECT_EX(   m_networkReply, SIGNAL(error(QNetworkReply::NetworkError)), 
                this, SLOT(onPutPayloadError(QNetworkReply::NetworkError)),
                Qt::QueuedConnection
            );
    // queued connect not used here, since this would make ignoring the error impossible
    ORIGIN_VERIFY_CONNECT_EX(   m_networkAccessManager , SIGNAL(sslErrors(QNetworkReply* , const QList<QSslError>&)), 
                this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError>&)),
                Qt::DirectConnection
            );
}

void Network::showResultWindow()
{
    if ( !m_resultEmitted )
    {
        m_resultEmitted = true;
        emit reportResult();
    }
}

void Network::onFinished()
{
    if ( !m_resultEmitted  )
    {
        m_resultEmitted = true;
        emit reportResult();
        emit finished();
    }
}

void Network::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    // Qt has problem parsing the expiry date for our local part of
    // certain certificates on some systems. This really needs to be fixed
    // properly but for now ignore expiry errors when we consider the expiry
    // date to be invalid
    if ((errors.length() == 1) &&
        (errors.at(0).error() == QSslError::CertificateExpired) &&
        !errors.at(0).certificate().expiryDate().isValid())
    {
        reply->ignoreSslErrors();
        return;
    }
    // For some reason if Qt can't find an issuer for a trusted certificate it
    // will still raise an error
    else if ((errors.length() == 2) &&
                (errors.at(0).error() == QSslError::UnableToGetLocalIssuerCertificate) &&
                (errors.at(1).error() == QSslError::CertificateUntrusted) &&
                (errors.at(0).certificate() == errors.at(1).certificate()))
    {
        // Is this certificate explicitly trusted by us?
        if (m_trustedCerts.contains(errors.at(0).certificate()))
        {
            // Then don't fail the connection
            reply->ignoreSslErrors();
            return;
        }
    }
}

void Network::onRequestSignedURLError(QNetworkReply::NetworkError error)
{
    getErrorHandler()->addError(QString("Error requesting signed URL: %1").arg(error));
    showResultWindow();
}

void Network::onPutPayloadError(QNetworkReply::NetworkError error)
{
    QByteArray arr = m_networkReply->readAll();
    QString str = QString::fromUtf8(arr.constData(), arr.size());
    getErrorHandler()->addError(QString("Error uploading data: %1 (%2)").arg(str).arg(error));
    showResultWindow();
}
