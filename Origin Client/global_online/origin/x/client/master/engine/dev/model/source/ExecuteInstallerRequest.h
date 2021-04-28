///////////////////////////////////////////////////////////////////////////////
// ExecuteInstallerRequest.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef EXECUTEINSTALLERREQUEST_H
#define EXECUTEINSTALLERREQUEST_H

#include <QString>

namespace Origin
{
namespace Downloader
{
/// \brief Contains information required to start an install process.
class ExecuteInstallerRequest
{
	public:
        /// \brief The ExecuteInstallerRequest constructor.
        /// \param installerAbsolutePath The absolute path to the installer.
        /// \param commandLineArguments The parameters to send to the installer.
        /// \param currentDirectory The current directory.
        /// \param showUI True if the UI should be shown for this install.
        /// \param useProxy True if the proxy installer should be used.
        /// \param waitForExit True if we should wait for the installer to exit.
		ExecuteInstallerRequest(const QString& installerAbsolutePath, const QString& commandLineArguments,
			const QString& currentDirectory, bool showUI = true, bool useProxy = true, bool waitForExit = true);

        /// \brief The ExecuteInstallerRequest copy constructor.
        /// \param other The ExecuteInstallerRequest object to copy from.
		ExecuteInstallerRequest(const ExecuteInstallerRequest& other);

        /// \brief Gets the absolute path to the installer.
        /// \return The absolute path to the installer.
        QString getInstallerPath() const { return mInstallerPath; }
        
        /// \brief Gets the parameters to send to the installer.
        /// \return The parameters to send to the installer.
        QString getCommandLineArguments() const { return mCommandLineArguments; }
        
        /// \brief Gets the current directory.
        /// \return The current directory.
        QString getCurrentDirectory() const { return mCurrentDirectory; }
        
        /// \brief Check if the UI should be shown for this install.
        /// \return True if the UI should be shown for this install.
        bool showUI() const { return mShowUI; }
        
        /// \brief Check if the proxy installer should be used.
        /// \return True if the proxy installer should be used.
        bool useProxy() const { return mUseProxy; }
        
        /// \brief Check if we should wait for the installer to exit.
        /// \return True if we should wait for the installer to exit.
        bool waitForExit() const { return mWaitForExit; }

        /// \brief Overridden "=" operator.
        /// \param other The ExecuteInstallerRequest to copy from.
        /// \return The ExecuteInstallerRequest that was copied to.
		ExecuteInstallerRequest& operator=(const ExecuteInstallerRequest& other);

	private:
        /// \brief The absolute path to the installer.
		QString mInstallerPath;

        /// \brief The parameters to send to the installer.
		QString mCommandLineArguments;

        /// \brief The current directory.
		QString mCurrentDirectory;
        
        /// \brief True if the UI should be shown for this install.
		bool mShowUI;
        
        /// \brief True if the proxy installer should be used.
		bool mUseProxy;
        
        /// \brief True if we should wait for the installer to exit.
		bool mWaitForExit;
};

} // namespace Downloader
} // namespace Origin

#endif // EXECUTEINSTALLERREQUEST_H
