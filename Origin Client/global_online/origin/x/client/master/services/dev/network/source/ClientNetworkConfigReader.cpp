#include "ClientNetworkConfigReader.h"
#include "services/network/NetworkAccessManager.h"
#include "services/heartbeat/Heartbeat.h"
#include "ProductVersion.h"
#include "version/version.h"
#include "services/debug/DebugService.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QTimer>

namespace Origin
{
    namespace Services
    {
        ClientNetworkConfigReader::ClientNetworkConfigReader(const QUrl &source, QObject *parent, int timeoutValue) 
            : QObject(parent),
            mTimeout(NULL)
        {
            Origin::Services::NetworkAccessManager *manager = Origin::Services::NetworkAccessManager::instance();
            mReply = manager->get(QNetworkRequest(source));

            ORIGIN_VERIFY_CONNECT(mReply, SIGNAL(finished()), this, SLOT(requestFinished()));

            // Give up pretty quickly
            if (timeoutValue > 0)
            {
                mTimeout = new QTimer(this);
                ORIGIN_VERIFY_CONNECT(mTimeout, SIGNAL(timeout()), this, SLOT(timeout()));
                mTimeout->setSingleShot(true);
                mTimeout->start(1500);
            }
        }

        void ClientNetworkConfigReader::timeout()
        {
            // This will call requestFinished() for us
            mReply->abort();
        }

        void ClientNetworkConfigReader::requestFinished()
        {
            // Liberate the reply
            mReply->deleteLater();
            // Don't time out anymore
            delete mTimeout;

            QNetworkReply::NetworkError err = mReply->error();

            if (err != QNetworkReply::NoError)
            {
                emit(failed());
                return;
            }

            // Note: Only tested with 302
            int nHTTPCode = mReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (nHTTPCode==302 || nHTTPCode==301 || nHTTPCode==307)
            {
                // Get redirected URL
                Origin::Services::NetworkAccessManager *manager = Origin::Services::NetworkAccessManager::instance();
                QUrl url = mReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                mReply = manager->get(QNetworkRequest(url));
                // Reconnect this request to the same handler
                ORIGIN_VERIFY_CONNECT(mReply, SIGNAL(finished()), this, SLOT(requestFinished()));
                // Set a new timer
                mTimeout = new QTimer(this);
                ORIGIN_VERIFY_CONNECT(mTimeout, SIGNAL(timeout()), this, SLOT(timeout()));
                mTimeout->setSingleShot(true);
                mTimeout->start(1500);
                return;
            }


            QDomDocument doc;

            if (!doc.setContent(mReply))
            {
                emit(failed());
                return;
            }

            if (!parseNetworkConfig(doc))
            {
                emit(failed());
                return;
            }
        }

        bool ClientNetworkConfigReader::parseNetworkConfig(const QDomDocument &doc)
        {
            ClientNetworkConfig config;
            ProductVersion ourVersion(EBISU_VERSION_STR, ",");

            if (doc.documentElement().tagName() != "ClientConfiguration")
            {
                return false;
            }

            // Find the default config first
            QDomElement defaultEl = doc.documentElement().firstChildElement("Default");
            if (!defaultEl.isNull())
            {
                config = applyVersionElement(config, defaultEl);
            }

            // XXX: Do versioning using ProductVersion
            QDomNodeList childNodes = doc.documentElement().childNodes();
            for(int i = 0; i < childNodes.length(); i++)
            {
                QDomElement childEl = childNodes.at(i).toElement();
                if (childEl.isNull())
                {
                    // Probably not an element
                    continue;
                }

                if (childEl.tagName() != "VersionedConfig")
                {
                    // Not what we want either
                    continue;
                }

                if (childEl.hasAttribute("minVersion"))
                {
                    ProductVersion minVersion(childEl.attribute("minVersion"));

                    if (minVersion > ourVersion)
                    {
                        continue;
                    }
                }

                if (childEl.hasAttribute("maxVersion"))
                {
                    ProductVersion maxVersion(childEl.attribute("maxVersion"));

                    if (maxVersion < ourVersion)
                    {
                        continue;
                    }
                }
                // If we meet the version requirements or there are none, process the elements
                config = applyVersionElement(config, childEl);
            }

            emit(succeeded(config));
            return true;
        }

        ClientNetworkConfig ClientNetworkConfigReader::applyVersionElement(ClientNetworkConfig config, const QDomElement &el)
        {
            //	Heartbeat config information
            QDomElement heartbeatEl = el.firstChildElement("Heartbeat");
            if (heartbeatEl.isNull() == false)
            {
                //	Get message server name
                config.mProperties.remove(ClientNetworkConfig::HeartbeatMessageServer);
                QDomElement ServerEl = heartbeatEl.firstChildElement("Server");
                if (ServerEl.isNull() == false)
                {
                    if (ServerEl.hasAttribute("name"))
                    {
                        //	ie:  name="http://blah.server.ea.com/pulse/"
                        config.mProperties[ClientNetworkConfig::HeartbeatMessageServer] = ServerEl.attribute("name");
                    }
                }

                //	Get message interval
                config.mProperties.remove(ClientNetworkConfig::HeartbeatMessageInterval);
                QDomElement IntervalEl = heartbeatEl.firstChildElement("Interval");
                if (IntervalEl.isNull() == false)
                {
                    if (IntervalEl.hasAttribute("time"))
                    {
                        //	ie:  time="60000" (60,000ms = 60s)
                        config.mProperties[ClientNetworkConfig::HeartbeatMessageInterval] = IntervalEl.attribute("time");
                    }
                }
            }

            //	URLS config information
            QDomElement urlsEl = el.firstChildElement("URLS");
            if (urlsEl.isNull() == false)
            {
                //	Get Release Notes URL
                config.mProperties.remove(ClientNetworkConfig::ReleaseNotesURL);
                QDomElement releaseNotesEl = urlsEl.firstChildElement("ReleaseNotes");
                if (releaseNotesEl.isNull() == false)
                {
                    config.mProperties[ClientNetworkConfig::ReleaseNotesURL] = releaseNotesEl.text();
                }
            }

            return config;
        }

    }
}