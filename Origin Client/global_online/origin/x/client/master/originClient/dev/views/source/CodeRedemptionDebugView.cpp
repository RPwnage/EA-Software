/////////////////////////////////////////////////////////////////////////////
// CodeRedemptionDebugView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CodeRedemptionDebugView.h"
#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManager.h"
#include "engine/login/LoginController.h"
#include "services/rest/HttpStatusCodes.h"
#include "ui_CodeRedemptionDebugView.h"
#include "TelemetryAPIDLL.h"

namespace {
    const QString LOCKBOX_INT_IP = "http://10.102.219.177/2/";
    const QString CATALOG_PRODUCT_TYPE = "catalogproducttype?code=";
    const QString REDEEM_OFFER = "consumecode/";
}

namespace Origin
{
namespace Client
{

CodeRedemptionDebugView::CodeRedemptionDebugView(QWidget* parent)
    : QWidget(parent)
    , mUi(new Ui::CodeRedemptionDebugView())
    , mGet(true)
{
    mUi->setupUi(this);
    init();
}

CodeRedemptionDebugView::~CodeRedemptionDebugView()
{
    delete mUi;
}

void CodeRedemptionDebugView::init()
{
    mUi->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Normal);
    ORIGIN_VERIFY_CONNECT(mUi->mRedeemCodeBtn, SIGNAL(clicked()), this, SLOT(onRedeemClicked()));
    ORIGIN_VERIFY_CONNECT(mUi->listWidget, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(onItemClicked(QListWidgetItem *)));
    ORIGIN_VERIFY_CONNECT(mUi->mClearForms, SIGNAL(clicked()), this, SLOT(onClear()));
}

void CodeRedemptionDebugView::onClear()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("clear_table");

    mUi->mResponseInformationMultiedit->setHtml("");
    mUi->listWidget->clear();
    mResponseInformation.clear();
}

void CodeRedemptionDebugView::onItemClicked(QListWidgetItem *item)
{
    int rowSelected = mUi->listWidget->currentRow();
    if(rowSelected >= mResponseInformation.size() || mResponseInformation.isEmpty())
        mUi->mResponseInformationMultiedit->setHtml("Waiting for response...");
    else
        mUi->mResponseInformationMultiedit->setHtml(mResponseInformation[rowSelected]);
}

void CodeRedemptionDebugView::onRedeemClicked()
{
    GetTelemetryInterface()->Metric_ODT_BUTTON_PRESSED("redeem_code");

    clearVariables();
    mUi->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Validating);
    mCdKey = mUi->codeInputSingleLineEdit->text().simplified().replace(" ","");
    createGetRequest();
}

void CodeRedemptionDebugView::clearVariables()
{
    mOfferId = "";
    mCdKey = "";
    mGetString = "";
    mPutString = "";
    mGet = true;
    mUi->mRedeemCodeBtn->setDisabled(true);
}

void CodeRedemptionDebugView::createGetRequest()
{
    QString urlWithKey = LOCKBOX_INT_IP + CATALOG_PRODUCT_TYPE + mCdKey;
    QUrl url(urlWithKey);
    QNetworkRequest req(url);
    req.setRawHeader("Nucleus-RequestorId", "GOP-Lockbox");
    QNetworkReply *reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->get(req);
    mGetString = "GET " + urlWithKey;
    mUi->listWidget->addItem(mGetString);
    ORIGIN_VERIFY_CONNECT_EX(reply, SIGNAL(finished()), this, SLOT(onLockboxNetworkReceived()), Qt::QueuedConnection);
}
void CodeRedemptionDebugView::createPutRequest()
{
    QString urlWithKey = LOCKBOX_INT_IP + REDEEM_OFFER + mCdKey + "?";
    QByteArray device;
    QUrlQuery urlQuery;

    urlQuery.addQueryItem("source","redempdebugger");
    urlQuery.addQueryItem("pidId",QString::number(Engine::LoginController::currentUser()->userId()));
    urlQuery.addQueryItem("ipAddress","1234");
    urlQuery.addQueryItem("productId",mOfferId);

    QUrl url(urlWithKey);
    url.setQuery(urlQuery);

    QNetworkRequest req(url);
    req.setRawHeader("Nucleus-RequestorId", "GOP-Lockbox");
    QNetworkReply *reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->put(req,device);
    mPutString = "PUT " + url.toString();
    mUi->listWidget->addItem(mPutString);
    ORIGIN_VERIFY_CONNECT_EX(reply, SIGNAL(finished()), this, SLOT(onLockboxNetworkReceived()), Qt::QueuedConnection);
}

void CodeRedemptionDebugView::onLockboxNetworkReceived()
{
    QString responseToWindow;
    QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());
    if(reply)
    {
        int httpResponse = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString rawResponse = reply->readAll();
        if(mGet)
        {
            responseToWindow = buildResponseString(reply,rawResponse,httpResponse, Get); 
            mResponseInformation.push_back(responseToWindow);
            // GET response
            if(httpResponse == Services::Http200Ok)
            {
                mOfferId = parseJsonForOfferId(rawResponse.toLatin1());
                createPutRequest();
            }
            else
            {
                mUi->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Invalid);
                mUi->mRedeemCodeBtn->setDisabled(false);
            }

            mGet = false;
        }
        else
        {
            responseToWindow = buildResponseString(reply,rawResponse,httpResponse, Put); 
            mResponseInformation.push_back(responseToWindow);
            // PUT response
            if(httpResponse == Services::Http200Ok)
                mUi->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Valid);
            else
                mUi->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Invalid);

            mUi->mRedeemCodeBtn->setDisabled(false);
        }

    }
    else
    {
        // no reply
        mResponseInformation.push_back("Failed to connect to server");
        mUi->originValidationContainer->setValidationStatus(Origin::UIToolkit::OriginValidationContainer::Invalid);
    }
}

QString CodeRedemptionDebugView::buildResponseString(QNetworkReply* reply, const QString rawResponse, const int httpResponse, Lockbox type) const
{
    QString call;
    if(type == Get)
        call = mGetString;
    else if (type == Put)
        call = mPutString;

    QByteArray ServerHeader = reply->header(QNetworkRequest::ServerHeader).toByteArray();
    QByteArray ContentLengthHeader = reply->header(QNetworkRequest::ContentLengthHeader).toByteArray();
    QByteArray ContentTypeHeader = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();

    return QString(call + "<br/><br/>" +
        "HTTP Response: " +
        QString::number(httpResponse) + "<br/>" + 
        "Server Header: " + ServerHeader + "<br/>" + 
        "Content Length Header: " + ContentLengthHeader + "<br/>" + 
        "Content Type Header: " + ContentTypeHeader + "<br/>" +
        "X-HOSTNAME: " + reply->rawHeader("X-HOSTNAME") + "<br/><br/>" + 
        " Raw: <br/>" +
        rawResponse);
}

QString CodeRedemptionDebugView::parseJsonForOfferId(const QByteArray& response)
{
    QJsonParseError jsonResult;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);
    if(jsonResult.error == QJsonParseError::NoError)
    {
        // Example expected response
        // {
        //   "productType" : {
        //     "productId" : "DR:224887200",
        //     "code" : "HDDD-X3AH-479F-RG4K-9AW2",
        //     "type" : "Base Game",
        //     "catalogType" : "Offer",
        //     "mappedProducts" : {
        //       "product" : [ {
        //         "productId" : "DR:224887200",
        //         "type" : "Base Game"
        //       } ]
        //     }
        //   }
        // }
        // If productType or productId doesn't exist in this response, then QString offerId is empty.
        QJsonObject obj = jsonDoc.object();
        QString offerId = obj["productType"].toObject()["productId"].toString();
        return offerId;
    }
    return QString();
}
}
}