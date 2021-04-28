///////////////////////////////////////////////////////////////////////////////
// InstallProgress.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef INSTALLPROGRESS_H
#define INSTALLPROGRESS_H

#include <QObject>
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>

namespace Origin
{

namespace Downloader
{
    /// \brief Name of the touchup installer pipe.
    const QString kTOUCHUP_NAME ( "EATOUCHUP_PIPE_NAME" );
#if defined(ORIGIN_PC)
    /// \brief Pipe name prefix.
	const QString kPIPE_NAME_PREFIX ( "\\\\.\\pipe\\" );
    /// \brief Final pipe name.
	const QString kINSTALL_PROGRESS_PIPE_NAME = kPIPE_NAME_PREFIX + kTOUCHUP_NAME;
#else
    /// \brief Final pipe name.
	const QString kINSTALL_PROGRESS_PIPE_NAME = kTOUCHUP_NAME;
#endif

	/// \brief Server that listens for updates from a custom touch-up installer.
	class InstallProgressServer : public QObject
	{
		Q_OBJECT
	public:
        /// \brief The InstallProgressServer constructor.
		InstallProgressServer();

        /// \brief The InstallProgressServer destructor.
		~InstallProgressServer();

        /// \brief Gets the current progress of the touch-up installer, ranging from 0.0f to 1.0f inclusive.
        /// \return The current progress of the touch-up installer.
		float GetProgress() const { return mProgress; }

	public slots:
        /// \brief Initializes the server.
		void Init();

        /// \brief Shuts down the server.
		void Shutdown();

        /// \brief Slot is triggered when a new server is created.  Initializes the socket.
		void OnNewConnection();

        /// \brief Slot that is triggered when there is new data to read.
		void OnReadyRead();

        /// \brief Slot that is triggered when the socket and touch-up installer are disconnected.
		void OnDisconnected();

signals:
        /// \brief Emitted when the touch-up installer and the socket have been connected.
		void Connected();

        /// \brief Emitted when we receive any sort of status from the touch-up installer.
        /// \param progress The current progress of the touch-up installer, ranging from 0.0f to 1.0f inclusive.
        /// \param description A summary of the current install step.
		void ReceivedStatus(float progress, QString description);

        /// \brief Emitted when the connection between the touch-up installer and socket has been ended.
		void Disconnected();

	private:
        /// \brief Server dedicated to listening for touch-up installer progress updates.
		QLocalServer * mServer;

        /// \brief Socket used for listening for touch-up installer progress updates.
		QLocalSocket * mSocket;

        /// \brief The current progress of the touch-up installer, ranging from 0.0f to 1.0f inclusive.
		float mProgress;

        /// \brief A summary of the current install step.
		QString mDescription;
	};


	/// \brief Wrapper for the install progress server and its thread manager.
	class InstallProgress : public QObject
	{
		Q_OBJECT
	public slots:
        /// \brief Slot that is triggered when the touch-up installer and the socket have been connected.
		void OnConnected() { mConnected = true; emit Connected(); }
        
        /// \brief Slot that is triggered when the touch-up installer and the socket have been disconnected.
		void OnDisconnected() { mConnected = false; emit Disconnected(); }

	signals:
        /// \brief Emitted when the touch-up installer and the socket have been connected.
		void Connected();
        
        /// \brief Emitted when we receive any sort of status from the touch-up installer.
        /// \param progress The current progress of the touch-up installer, ranging from 0.0f to 1.0f inclusive.
        /// \param description A summary of the current install step.
		void ReceivedStatus(float progress, QString description);
        
        /// \brief Emitted when the touch-up installer and the socket have been disconnected.
		void Disconnected();

	public:
        /// \brief The InstallProgress constructor.
		InstallProgress();

        /// \brief The InstallProgress destructor.
		~InstallProgress();

        /// \brief Starts listening to the install progress server thread.
		void StartMonitor();

        /// \brief Stops listening to the install progress server thread.
		void StopMonitor();
        
        /// \brief Check if the touch-up installer and the socket have been connected.
        /// \return True if the touch-up installer and the socket have been connected.
		bool IsConnected() const { return mConnected; }

        /// \brief Gets the current progress of the touch-up installer, ranging from 0.0f to 1.0f inclusive.
        /// \return The current progress of the touch-up installer.
		float GetProgress();

	private:
        /// \brief The thread that the install progress server runs on.
		QThread *mServerThread;

		/// \briedf The install progress server.
		InstallProgressServer *mInstallProgressServer;
        
        /// \brief True if the touch-up installer and the socket have been connected.
		bool mConnected;
	};

} // Downloader
} // Origin
#endif