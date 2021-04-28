#ifndef DIPUPDATE_H
#define DIPUPDATE_H

#include "engine/downloader/ContentInstallFlowContainers.h"
#include "services/downloader/Parameterizer.h"
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

#include <QMutex>
#include <QRunnable>
#include <QStateMachine>

namespace Origin
{

namespace Downloader
{
	class ContentInstallFlowDiP;

    class ActiveContentWatcherKillSwitch : public QObject
    {
        Q_OBJECT

    public:
        ActiveContentWatcherKillSwitch();
        ~ActiveContentWatcherKillSwitch();

        void kill();
        bool isKilled();

        QMutex& getLock();

    private:
        volatile bool mKilled;
        QMutex mLock;
    };

    /// \brief Directs the finalization phase of updating DiP content.
	class ORIGIN_PLUGIN_API DiPUpdate : public QStateMachine
	{
		Q_OBJECT

		public:

            /// \brief DiPUpdate contructor.
            ///
            /// \param parent The content install flow state machine which will use this object to run the finalization stage.
			DiPUpdate(ContentInstallFlowDiP* parent);

            ~DiPUpdate();

            /// \brief Determines if the content's installer file was updated.
            ///
            /// \return bool True if the content's installer file was updated.
			bool InstallerChanged() const;

            /// \brief Resumes a finalization state machine when it has paused because the user is playing the content.
			void SetContentInactive();

            /// \brief Determines if the state machine is currently in a cancelable state.
            ///
            /// \return bool True if the finalization state machine can be canceled.
            bool CanCancel();

		public slots:

            /// \brief Stops the update finalization state machine.
			void onCancel();

		signals:

            /// \brief Emitted when it has been determined that the user is not playing the updating content.
			void eligibleForUpdate();

            /// \brief Emitted during the EULA stage when no EULAs where updated.
			void eulaInputReceived();

            /// \brief Emitted when it has been determined that the user is currently playing the updating content.
			void pauseUpdate();

            /// \brief Emitted when the client needs to display dialogs for updated EULA files.
			void pendingUpdatedEulas(const Origin::Downloader::EulaStateRequest&);

            /// \brief Emitted when the state machine has been paused due to playing content and the user has stopped playing.
			void resumeUpdate();

            /// \brief Emitted when the user has satisfied the requirements and the update is ready to finish (destage, install chunks, etc)
            void finalizeUpdate();

		private slots:

            /// \brief Executed when the update finalization has finished.
            void onComplete();

            /// \brief Pauses the update finalization if the user is playing the content.
			void onEligibilityCheck();

            /// \brief Instucts the protocol to destage all files.
			void onFinalizeUpdate();

            /// \brief Starts a DiPUpdate::ActiveContentWatcher when the update finalization is paused due to playing content.
			void onPaused();

            /// \brief Determines if any EULAs have been updated.
			void onPendingEulaInput();

            /// \brief Stops the update finalization state machine if the protocol emits an error signal.
			void onFlowError();

		private:

            /// \brief Creates the states and transitions of the update finalization state machine.
			void InitializeUpdateStateMachine();

            /// \brief The content install flow state machine which will use this object to run the finalization stage.
			ContentInstallFlowDiP* mDipFlow;

            /// \brief The request that is emitted to the client when updated EULA files should be shown.
			EulaStateRequest mEulaStateRequest;

            /// \brief A flag to indicate if the installer for the content changed.
			bool mInstallerChanged;

            /// \brief A flag to indicate if the update finalization state machine can be canceled.
            bool mCanCancel;

            /// \brief Protects mCanCancel.
            QMutex mMutex;

            QSharedPointer<ActiveContentWatcherKillSwitch> mContentWatcherKillSwitch;

            /// \brief Used to detect when a user has stopped playing content.
			class ORIGIN_PLUGIN_API ActiveContentWatcher : public QRunnable
			{
				public:

                    /// \brief ActiveContentWatcher constructor.
                    ///
                    /// \param parent The DiPUpdate object which should be informed when the content is no longer being played.
                    /// \param content The content to watch.
                    ActiveContentWatcher(DiPUpdate* parent, QSharedPointer<ActiveContentWatcherKillSwitch> killSwitch, const Origin::Engine::Content::EntitlementRef content);

                    /// \brief Begin watching.
					virtual void run();

				private:

                    /// \brief The DiPUpdate object which should be informed when the content is no longer being played.
					DiPUpdate* mParent;

                    /// \brief The kill switch that the parent DiPUpdate flow can use to kill this QRunnable.
                    QSharedPointer<ActiveContentWatcherKillSwitch> mKillSwitch;

                    /// \brief The content to watch.
                    const Origin::Engine::Content::EntitlementRef mContent;
			};

	};

} // namespace Downloader
} // namespace Origin

#endif // CONTENTINSTALLFLOWDIPUPDATE_H
