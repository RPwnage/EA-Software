#ifndef CLIENTNETWORKCONFIGREADER_H
#define CLIENTNETWORKCONFIGREADER_H

#include <QObject>
#include <QUrl>
#include "ClientNetworkConfig.h"

#include "services/plugin/PluginAPI.h"

class QNetworkReply;
class QDomDocument;
class QDomElement;
class QTimer;

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API ClientNetworkConfigReader : public QObject
        {
            Q_OBJECT

        public: 
            ClientNetworkConfigReader(const QUrl &source, QObject *parent = NULL, int timeoutValue = -1);

        signals:
            void succeeded(const ClientNetworkConfig &);
            void failed();

        protected slots:
            void requestFinished();
            void timeout();

        protected:
            bool parseNetworkConfig(const QDomDocument &);
            ClientNetworkConfig applyVersionElement(ClientNetworkConfig, const QDomElement &);
            QNetworkReply *mReply;
            QTimer *mTimeout;
        };
    }
}

#endif