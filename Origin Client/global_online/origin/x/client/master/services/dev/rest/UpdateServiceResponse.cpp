///////////////////////////////////////////////////////////////////////////////
// UpdateServiceResponse.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QDomDocument>
#include "services/rest/UpdateServiceResponse.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    namespace Services
    {
		UpdateQueryResponse::UpdateQueryResponse(QNetworkReply* reply)
            : OriginServiceResponse(reply)
		{
            ORIGIN_VERIFY_CONNECT(this, SIGNAL(error(Origin::Services::restError)), this, SLOT(deleteLater()));
		}

		bool UpdateQueryResponse::parseSuccessBody(QIODevice *body)
		{
            QDomDocument doc;
            if (!doc.setContent(body))
                return false;

            QString test = doc.toString();
            QDomNodeList childNodes = doc.documentElement().childNodes();

            for (int i = 0; i < childNodes.length(); ++i)
            {
                QDomElement element = childNodes.at(i).toElement();
                if (element.isNull())
                {
                    continue;
                }
                QString tag = element.tagName();
                int type = element.nodeType();
                Q_UNUSED(type);
                if (tag == "version")
                {
                    mUpdate.version = element.childNodes().at(0).nodeValue();
                }
                else if (tag == "buildType")
                {
                    mUpdate.buildType = element.childNodes().at(0).nodeValue();
                }
                else if (tag == "downloadURL")
                {
                    mUpdate.downloadURL = element.childNodes().at(0).nodeValue();
                }
                else if (tag == "updateRule")
                {
                    QString rule = element.childNodes().at(0).nodeValue();
                    if (rule.compare("OPTIONAL") == 0)
                    {
                        mUpdate.updateRule = UpdateRule_Optional;
                    }
                    else if (rule.compare("REQUIRED") == 0)
                    {
                        mUpdate.updateRule = UpdateRule_Required;
                    }
                    else if (rule.compare("MANDATORY") == 0)
                    {
                        mUpdate.updateRule = UpdateRule_Mandatory;
                    }
                }
                else if (tag == "eulaVersion")
                {
                    mUpdate.eulaVersion = element.childNodes().at(0).nodeValue();
                }
                else if (tag == "eulaURL")
                {
                    mUpdate.eulaURL = element.childNodes().at(0).nodeValue();
                }
                else if (tag == "activationDate")
                {
                    QString activationDate = element.childNodes().at(0).nodeValue();
                    mUpdate.activationDate = QDateTime::fromString(activationDate, Qt::ISODate);

                    mUpdate.activationDate.setTimeSpec(Qt::UTC);
                }
            }

            return true;
        }
    }
}
