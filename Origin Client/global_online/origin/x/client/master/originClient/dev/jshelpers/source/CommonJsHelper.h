#ifndef COMMONJSHELPER_H
#define COMMONJSHELPER_H

#include <QObject>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API CommonJsHelper : public QObject
        {
	        Q_OBJECT
        public:
	        CommonJsHelper(QObject *parent = NULL);

        public slots:
	        void launchExternalBrowser(QString url);
        #ifdef _WIN32
	        void launchInternetExplorer(QString url);
        #endif
            QString selectFile(QString startingDirectory, QString filter, QString operation);
	        QString ebisuVersion();
	        void closePopUp();
	        void doRetry();
            void closeWindow();

        signals:
	        void retry();
            void closeWebWindow();
        };
    }
}



#endif