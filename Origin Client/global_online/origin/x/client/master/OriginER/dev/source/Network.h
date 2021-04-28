///////////////////////////////////////////////////////////////////////////////
// Network.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef Network_h
#define Network_h

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslCertificate>
#include <QList>
#include <QByteArray>
#include <QThread>

class DiagnosticApplication;
class ErrorHandler;
class DataCollector;
class ReportErrorView;

// TODO Rename to DataUploader
class Network : public QObject
{
	Q_OBJECT
public:
	enum EFormat {FORMAT_JSON, FORMAT_XML};

	Network( DiagnosticApplication* app );
	~Network(void);

	void setErrorHandler(ErrorHandler* errorHandler);
	ErrorHandler* getErrorHandler();

	void setDataCollector(DataCollector* dataCollector);
	DataCollector* getDataCollector();

	void setReportView(ReportErrorView* reportView);
	ReportErrorView* getReportView();

	QString const& getTrackingNumber();

signals:
	//! Emitted on completion of upload of contents, or on failure at any point
	void reportResult();
	void finished();

public slots:
	void requestSignedURL();

private slots:
	//! Signed URL has been received, initiate data upload
	void initiateUploadToCloud();
	//! Upload is finished
	void onFinished();
	//! Set error state and emit reportResult()
	void onRequestSignedURLError(QNetworkReply::NetworkError);
	void onPutPayloadError(QNetworkReply::NetworkError error);
	//! Potentially ignore the error, used for debugging only
	void onSslErrors(QNetworkReply* reply, const QList<QSslError>&);

private:
	static bool testComputeHash();
	static QString computeHash( QString const& body, QString const& machineHash, QString const& timestamp );

	QString const& getUrl();
	QList<QSslCertificate> readCertificateFile(const QString &filename);

	QString getSignedUrlRequestXML();
	QByteArray const& getPayload();
	QString const& getPayloadMd5();
	QString getUserAgent() const;

    void showResultWindow();

	ErrorHandler* m_errorHandler;
	DataCollector* m_dataCollector;
	ReportErrorView* m_reportView;
	DiagnosticApplication *m_diagApp;

	QNetworkAccessManager *m_networkAccessManager;

	//! List of local certs that we trust
	/*!
	  Useful for ignoring local SSL errors.
	 */
	QList<QSslCertificate> m_trustedCerts;

	QByteArray m_postData;
	QNetworkReply* m_networkReply;

	QString const m_url;
	bool const m_debugVerifyPeer;
	bool const m_debugFailRequest;
	bool const m_debugFailUpload;
	bool const m_debugDumpData;
	//! Flag used to ensure the reportResult() signal is emitted only once
	bool m_resultEmitted;
	QString m_payloadMd5;
	QString m_trackingNumber;
};

#endif
