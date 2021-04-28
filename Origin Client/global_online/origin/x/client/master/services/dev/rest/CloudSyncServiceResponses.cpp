#include <QDomDocument>
#include <QNetworkReply>
#include <QFile>
#include <QDebug>
#include <algorithm>

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/CloudSyncServiceResponses.h"

namespace Origin
{
    namespace Services
    {
        void CloudSyncLockExtensionResponse::processReply()
        {
            QNetworkReply::NetworkError networkError = reply()->error();

            if ((networkError == QNetworkReply::NoError) && processLockHeaders(reply()))
            {
                emit success();
            }
            else
            {
                emit error();
            }
        }

        void CloudSyncLockPromotionResponse::processReply()
        {
            QNetworkReply::NetworkError networkError = reply()->error();

            if ((networkError == QNetworkReply::NoError) && processLockHeaders(reply()))
            {
                emit success();
            }
            else
            {
                emit error();
            }
        }

        bool CloudSyncLockingResponse::processLockHeaders(const QNetworkReply *reply)
        {
            if (!reply->hasRawHeader(SyncLockTtlHeader))
            {
                // We must always get a TTL
                return false;
            }

            m_lock.timeToLiveSeconds = reply->rawHeader(SyncLockTtlHeader).toInt();

            if (reply->hasRawHeader(SyncLockNameHeader))
            {
                m_lock.lockName = reply->rawHeader(SyncLockNameHeader);
                return true;
            }
            else
            {
                // When we extend the lock we should already have the lock name
                return !m_lock.lockName.isNull();
            }
        }

        void CloudSyncManifestDiscoveryResponse::processReply()
        {
            QNetworkReply::NetworkError networkError = reply()->error();

            if ((networkError == QNetworkReply::NoError) && 
                parseSuccessBody(reply()))
            {
                emit success();
            }
            else
            {
                emit error(UnknownError);
            }
        }

        bool CloudSyncManifestDiscoveryResponse::parseSuccessBody(QIODevice *dev)
        {
            QDomDocument manifestDoc;

            if (manifestDoc.setContent(dev))
            {
                if (manifestDoc.documentElement().tagName() == "manifest")
                {
                    QDomElement urlNode = manifestDoc.documentElement().firstChildElement("url");
                    if (!urlNode.isNull())
                    {
                        mManifestUrl = QUrl::fromEncoded(urlNode.text().toLatin1(), QUrl::StrictMode);
                        return mManifestUrl.isValid();
                    }
                }
            }

            return false;
        }

        void CloudSyncSyncInitiationResponse::processReply()
        {
            QNetworkReply::NetworkError networkError = reply()->error();

            if ((networkError == QNetworkReply::NoError) && 
                processLockHeaders(reply()) && 
                parseSuccessBody(reply()))
            {
                emit success();
            }
            else
            {
                if (reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 409)
                {
                    mError = ManifestLockedError;
                }
                else
                {
                    mError = UnknownError;
                }

                emit error(mError);
            }
        }

        bool CloudSyncSyncInitiationResponse::parseSuccessBody(QIODevice *dev)
        {
            QDomDocument syncDoc;

            if (syncDoc.setContent(dev))
            {
                // Should be:
                // <sync>
                //    <host>https://cloudsave-dev.s3.amazonaws.com/</host>
                //    <root>/12292111576/dragonage_na</root>
                //    <manifest>/2183474389/dragonage_na/manifest.xml?AWSAccessKeyId=0PN5J17HBGZHT7JJ3X82&Signature=rucSbH0yNEcP9oM2XNlouVI3BH4%3D&Expires=1175139620</manifest>
                // <sync>

                if (syncDoc.documentElement().tagName() == "sync")
                {
                    QDomElement hostNode = syncDoc.documentElement().firstChildElement("host");

                    if (hostNode.isNull())
                    {
                        return false;
                    }

                    QDomElement rootNode = syncDoc.documentElement().firstChildElement("root");

                    if (rootNode.isNull())
                    {
                        return false;
                    }

                    QUrl rootUrl(QUrl::fromEncoded(rootNode.text().toLatin1(), QUrl::StrictMode));
                    mSignatureRoot = QUrl(hostNode.text()).resolved(rootUrl);

                    QDomElement manifestNode = syncDoc.documentElement().firstChildElement("manifest");
                    if (!manifestNode.isNull())
                    {
                        mManifestUrl = QUrl::fromEncoded(manifestNode.text().toLatin1(), QUrl::StrictMode);
                        return mManifestUrl.isValid();
                    }
                }
            }

            return false;
        }

        void CloudSyncUnlockResponse::processReply()
        {
            QNetworkReply::NetworkError networkError = reply()->error();

            if (networkError == QNetworkReply::NoError)
            {
                emit success();
            }
            else
            {
                emit error();
            }
        }

        void CloudSyncServiceAuthenticationResponse::processReply()
        {
            using namespace Origin::Services;
            QNetworkReply::NetworkError networkError = reply()->error();

            if (networkError == QNetworkReply::NoError)
            {
                if(processLockHeaders(reply()) && parseSuccessBody(reply()))
                {
                    emit success();
                }
                else
                {
                    ORIGIN_LOG_ERROR << "Improper response headers or expected body";
                    emit error(ImproperResponse);
                }
            }
            else
            {
                HttpStatusCode returnCode = static_cast<HttpStatusCode>(reply()->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
                switch (returnCode)
                {
                case Http400ClientErrorBadRequest:
                    ORIGIN_LOG_ERROR << QString("authtoken or lock is missing from the request - error [%1] QNetworkReply error [%2]").arg(returnCode).arg(networkError );
                    emit error(MissingAuthtokenOrLock);
                    break;
                case Http403ClientErrorForbidden:
                    ORIGIN_LOG_ERROR << QString("readlock held, user does not currently hold a lock or user+lock pair is not valid  - error [%1] QNetworkReply error [%2]").arg(returnCode).arg(networkError );
                    emit error(InvalidLock);
                    break;
                default:
                    ORIGIN_LOG_ERROR << QString("Unexpected error  - error [%1] QNetworkReply error [%2]").arg(returnCode).arg(networkError );
                    emit error(UnknownError);
                    break;
                }
            }
        }

        static bool parseAuth(const QDomElement& myElem, CloudSyncServiceSignedTransfer& transfer)
        {
            // Up to here the element is valid. Scan the children to find the data
            for (QDomNode children = myElem.firstChild(); !children.isNull(); children = children.nextSibling())
            {
                QDomElement childElem = children.toElement(); // try to convert the node to an element.

                bool ok(!childElem.isNull());
                ORIGIN_ASSERT(ok);
                if (!ok)
                    return false;

                QString tagName = childElem.tagName(); 

                if (tagName != "url" && tagName != "headers")
                {
                    // Invalid XML document
                    ORIGIN_ASSERT(false);
                    return false;
                }

                if ("url" == tagName) // found element url
                {
                    QUrl url = QUrl::fromEncoded(childElem.text().toLatin1(), QUrl::StrictMode);
                    transfer.setUrl(url);
                } 
                else // found header elements
                {
                    HeadersMap headersMap;
                    for (QDomNode headers = childElem.firstChild(); !headers.isNull(); headers = headers.nextSibling())
                    {
                        QDomElement header = headers.toElement(); // try to convert the node to an element.

                        ok = !header.isNull();
                        ORIGIN_ASSERT(ok);
                        if (!ok)
                            return false;

                        tagName = header.tagName(); 

                        if (tagName != "header")
                        {
                            // Invalid XML document
                            ORIGIN_ASSERT(false);
                            return false;
                        }

                        ok = header.hasAttribute("key");
                        ORIGIN_ASSERT(ok);
                        QByteArray key;
                        if (ok)
                        {
                            key = header.attribute("key").toUtf8();
                        }

                        ok = header.hasAttribute("value");
                        ORIGIN_ASSERT(ok);
                        QByteArray value;
                        if (ok)
                        {
                            value = header.attribute("value").toUtf8();
                        }
                        headersMap[key] = value;
                    } // for (QDomNode headers 
                    transfer.setHeaders(headersMap);
                } // if ("url" == tagName) // found element url

            } // for (QDomNode children
            return true;
        }

        bool CloudSyncServiceAuthenticationResponse::parseSuccessBody(QIODevice *iodev)
        {
            QDomDocument doc;
            QString errorMsg;
            int errorline(-1);
            int errorcol(-1);

            if (!doc.setContent(iodev, &errorMsg, &errorline,&errorcol)) 
            {          
                ORIGIN_LOG_ERROR << QString("QDomdocument-XML Parser: Couldn't parse document. Error [%1]: line [%2] column [%3]").arg(errorMsg).arg(errorline).arg(errorcol);
                ORIGIN_ASSERT(false);
                return false;      
            }      

            QDomElement docElem = doc.documentElement(); 
            QString tagName = docElem.tagName();
            if (tagName != "authorizations")
            {
                // Not the document we're looking for
                return false;
            }

            for (QDomNode authTransfers = docElem.firstChild(); !authTransfers.isNull(); authTransfers = authTransfers.nextSibling())
            {
                // Container of request container and manifest container
                CloudSyncServiceSignedTransfer cssst;

                QDomElement myElem = authTransfers.toElement();

                bool ok (!myElem.isNull());
                ORIGIN_ASSERT(ok);
                if (!ok)
                    return false;

                tagName = myElem.tagName(); 

                if (tagName != "request" && tagName != "manifest")
                {
                    // Invalid XML document
                    ORIGIN_LOG_ERROR << QString("WEIRD: Found WRONG element [%1]. Out of here!").arg(tagName);
                    ORIGIN_ASSERT(false);
                    return false;
                }

                // Request or manifest element
                ok = myElem.hasAttribute("id");
                if (!ok)
                {
                    // Invalid XML document
                    ORIGIN_LOG_ERROR << QString("This element [%1] does not contain valid information (no Id!). Out of here!").arg(tagName);
                    ORIGIN_ASSERT(false);
                    return false;
                }

                const int elemId(myElem.attribute("id").toInt(&ok, 10));
                if (!ok)
                {
                    ORIGIN_ASSERT(false);
                    return false;
                }

                // Associate with the original request
                ORIGIN_ASSERT(elemId < mAuthReqList.size());
                cssst.setAuthorizationRequest(mAuthReqList.at(elemId));

                ORIGIN_ASSERT("request" == tagName);

                if (!parseAuth(myElem, cssst))
                {
                    return false;
                }

                // All good. Set value in our container
                mSignedTransfers << cssst;
            } // for (QDomNode authTransfers 

            return true;
        }

        void CloudSyncLockTypeResponse::processReply()
        {
            // Default in case we don't get a response back
            m_isWriting = false;

            QNetworkReply* networkReply = reply();
            
            int status = networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == Http200Ok)
            {
                QString lockType;
                if (networkReply->hasRawHeader(SyncLockTypeHeader))
                {
                    lockType = networkReply->rawHeader(SyncLockTypeHeader);

                    if (lockType.compare("read", Qt::CaseInsensitive) == 0)
                        m_isWriting = false;
                    else if (lockType.compare("write", Qt::CaseInsensitive) == 0)
                        m_isWriting = true;
                }
            }
        }
    }
}
