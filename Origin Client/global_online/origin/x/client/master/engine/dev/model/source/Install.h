///////////////////////////////////////////////////////////////////////////////
// Install.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
// Author: Hans van Veenendaal
///////////////////////////////////////////////////////////////////////////////
#ifndef INSTALL_H
#define INSTALL_H

#include "ExecuteInstallerRequest.h"

#include <QMetaType>
#include <QObject>
#include "services/process/IProcess.h"
#include "engine/content/ContentTypes.h"

namespace Origin
{
    namespace Downloader
    {
        /// \brief Manages the install process of individual content.
        class Install : public QObject
        {
            Q_OBJECT
        public:
            /// \brief The Install constructor.
            /// \param entitlement A pointer to the entitlement to install.
            explicit Install(Origin::Engine::Content::EntitlementRef entitlement);
            
            /// \brief Starts the installation.
            /// \param req An ExecuteInstallerRequest object that contains all of the data needed to install (i.e. installer path, installer parameters, etc.)
            /// \return True if the install was started successfully.
            bool start(const Origin::Downloader::ExecuteInstallerRequest& req);

        signals:
            /// \brief Emitted when all components are installed.
            void installFinished();

            /// \brief Emitted when a component fails to install.
            /// \param exitCode -1 if the install failed, otherwise the exit code described by Services::IProcess::ExitStatus.
            void installFailed(qint32 exitCode);

        public slots:
            /// \brief Slot that gets triggered when the install finishes.
            /// \param exitCode The exit code that the process finished with.
            /// \param status The status of the install, described by Services::IProcess::ExitStatus.
	        void onProcessFinished(uint32_t pid, int exitCode, Origin::Services::IProcess::ExitStatus status);

        private slots:
            /// \brief Slot that gets triggered when the install encounters an error.
            /// \param error The error that the installer encountered, described by Services::IProcess::ProcessError.
            /// \param systemErrorValue The OS error code that caused the error.
	        void onProcessError(Origin::Services::IProcess::ProcessError error, quint32 systemErrorValue);

        private:
            /// \brief The entitlement associated with this install.
            Origin::Engine::Content::EntitlementRef mEntitlement;

            /// \brief The process running the installer.
            class Origin::Services::IProcess *mInstalling;

        };
    }
}
#endif // INSTALL_H
