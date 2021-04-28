///////////////////////////////////////////////////////////////////////////////
// VoiceServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/VoiceServiceResponse.h"
#include "services/debug/DebugService.h"
#include <services/log/LogService.h>
#include <services/rest/OriginServiceValues.h>
#include <services/settings/SettingsManager.h>
#include "TelemetryAPIDLL.h"

namespace Origin
{
    namespace Services
    {
        namespace
        {
            const QString EXISTS = "Exists";
            const QString CREATE = "Create";
            const QString JOIN = "Join";
            const QString TOKEN = "Token";

        }

        ChannelExistsResponse::ChannelExistsResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void ChannelExistsResponse::processReply()
        {
			QJsonParseError jsonResult;
			QByteArray response(reply()->readAll());
			QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService ChannelExists resp: json=" << response;

            Origin::Services::HttpStatusCode status = (Origin::Services::HttpStatusCode) reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == Http200Ok)
            {
                //check if it's a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();
                    mExists = obj["exists"].toBool();
                }

                emit success();
            }
            else
            {
                /* Example JSON
                {
                    "errorCode" : 10062,
                    "errorMessage" : "No access token provided"
                }*/
                //check if it's a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();

                    mErrorCode = obj["errorCode"].toInt();
                    mErrorMessage = obj["errorMessage"].toString();

                    GetTelemetryInterface()->Metric_VC_CHANNEL_ERROR(EXISTS.toLocal8Bit(), mErrorMessage.toLocal8Bit(), mErrorCode);
                    ORIGIN_LOG_EVENT << "ChannelExists Error: " << static_cast<long>(mErrorCode) << ", response=\"" << mErrorMessage << "\"";
                }
                else
                {
                    ORIGIN_LOG_EVENT << "ChannelExists Error: " << jsonResult.error << ", " << response;
                }
                emit error(status);
            }
        }

        CreateChannelResponse::CreateChannelResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void CreateChannelResponse::processReply()
        {
			QJsonParseError jsonResult;
			QByteArray response(reply()->readAll());
			QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService CreateChannel resp: json=" << response;

            Origin::Services::HttpStatusCode status = (Origin::Services::HttpStatusCode) reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == Http200Ok)
            {
                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();
                    QJsonObject channel = obj["channel"].toObject();

                    mChannelId = channel["id"].toString();
                }

                emit success();
            }
            else
            {
                /* Example JSON
                {
                    "errorCode" : 10062,
                    "errorMessage" : "No access token provided"
                }*/
                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();

                    mErrorCode = obj["errorCode"].toInt();
                    mErrorMessage = obj["errorMessage"].toString();

                    GetTelemetryInterface()->Metric_VC_CHANNEL_ERROR(CREATE.toLocal8Bit(), mErrorMessage.toLocal8Bit(), mErrorCode);
                    ORIGIN_LOG_EVENT << "CreateChannel Error: " << static_cast<long>(mErrorCode) << ", " << mErrorMessage;
                }
                else
                {
                    ORIGIN_LOG_EVENT << "CreateChannel Error: " << jsonResult.error << ", response=\"" << response << "\"";
                }
                emit error(status);
            }
        }

        AddChannelUserResponse::AddChannelUserResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void AddChannelUserResponse::processReply()
        {
			QJsonParseError jsonResult;
			QByteArray response(reply()->readAll());
			QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService AddChannelUser resp: json=" << response;

            Origin::Services::HttpStatusCode status = (Origin::Services::HttpStatusCode) reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == Http200Ok)
            {
                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();
                    QJsonObject channel = obj["channel"].toObject();

                    mChannelId = channel["id"].toString();
                }

                emit success();
            }
            else
            {
                /* Example JSON
                {
                    "errorCode" : 10062,
                    "errorMessage" : "No access token provided"
                }*/
                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();

                    mErrorCode = obj["errorCode"].toInt();
                    mErrorMessage = obj["errorMessage"].toString();
                    if( mErrorCode == Origin::Services::restErrorVoiceServerUnderMaintenance ) // "Voice Server has been shut down for maintenance. Please try again later."
                    {
                        setError(Origin::Services::restErrorVoiceServerUnderMaintenance);
                    }

                    GetTelemetryInterface()->Metric_VC_CHANNEL_ERROR(JOIN.toLocal8Bit(), mErrorMessage.toLocal8Bit(), mErrorCode);
                    ORIGIN_LOG_EVENT << "AddChannelUser Error: " << static_cast<long>(mErrorCode) << ", " << mErrorMessage;
                }
                else
                {
                    ORIGIN_LOG_EVENT << "AddChannelUser Error: " << jsonResult.error << ", response=\"" << response << "\"";
                }
                emit error(status);
            }
        }

        GetChannelTokenResponse::GetChannelTokenResponse(AuthNetworkRequest reply)
            : OriginAuthServiceResponse(reply)
        {
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
        }

        void GetChannelTokenResponse::processReply()
        {
			QJsonParseError jsonResult;
			QByteArray response(reply()->readAll());
			QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &jsonResult);

			ORIGIN_LOG_EVENT_IF(Origin::Services::readSetting(Origin::Services::SETTING_LogVoiceServiceREST))
				<< "VoiceService GetChannelToken resp: json=" << response;

            Origin::Services::HttpStatusCode status = (Origin::Services::HttpStatusCode) reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == Http200Ok)
            {
                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();
                    QJsonObject token = obj["token"].toObject();

                    mToken = token["token"].toString();
                }

                emit success();
            }
            else
            {
                /* Example JSON
                {
                    "errorCode" : 10062,
                    "errorMessage" : "No access token provided"
                }*/
                //check if its a valid JSON response
                if(jsonResult.error == QJsonParseError::NoError)
                {
                    QJsonObject obj = jsonDoc.object();

                    mErrorCode = obj["errorCode"].toInt();
                    mErrorMessage = obj["errorMessage"].toString();

                    GetTelemetryInterface()->Metric_VC_CHANNEL_ERROR(TOKEN.toLocal8Bit(), mErrorMessage.toLocal8Bit(), mErrorCode);
                    ORIGIN_LOG_EVENT << "GetChannelToken Error: " << static_cast<long>(mErrorCode) << ", response=\"" << mErrorMessage << "\"";
                }
                else
                {
                    ORIGIN_LOG_EVENT << "GetChannelToken Error: " << jsonResult.error << ", " << response;
                }
                emit error(status);
            }
        }
    }
}
