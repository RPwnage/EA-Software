/////////////////////////////////////////////////////////////////////////////
// ContentInstallFlowState.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CONTENTINSTALLFLOWSTATE_H
#define CONTENTINSTALLFLOWSTATE_H

#include <QMetaType>
#include <QObject>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Downloader
{
    /// \brief Defines all possible public states that an IContentInstallFlow derived class may employ.
    ///
    /// A public state is one that is exposed to outside Origin components through the
    /// IContentInstallFlow::stateChanged signal.
    ///
    class ORIGIN_PLUGIN_API ContentInstallFlowState : public QObject
    {
        Q_OBJECT
        Q_ENUMS(value)

        public:

            /// \brief Potential content install flow states.
            enum value
            {
                kInvalid = -1,                  ///< The install flow has not been initialized.
                kError,                         ///< The install flow encountered an error.
                kPaused,                        ///< The install flow has halted file transfer and must be told to continue.
                kPausing,                       ///< The install flow is in the process of transitioning to kPaused.
                kCanceling,                     ///< The install flow was told to stop in the middle of an install and is in the process of transitioning to kReadyToStart.
                kReadyToStart,                  ///< The install flow is ready to be activated.
                kInitializing,                  ///< The install flow is gathering information it will need to perform the download/install, update, or repair.
			    kResuming,                      ///< The install flow is in the process of re-initializing and will soon continue.
                kPreTransfer,                   ///< The install flow is about to begin transferring files.
                kPendingInstallInfo,            ///< The install flow is waiting for the user to supply information required for content installation.
                kPendingEulaLangSelection,      ///< The install flow is waiting for the user to choose an install locale.
                kPendingEula,                   ///< The install flow is waiting for the user to accept end user license agreements.
                kEnqueued,                      ///< The install flow is in the operation queue
                kTransferring,                  ///< The install flow is transferring files.
                kPendingDiscChange,             ///< The install flow is waiting for the user to change discs during ITO.
                kPostTransfer,                  ///< The install flow has completed file transfer and is cleaning up.
                kMounting,                      ///< The install flow is mounting a disk image
                kUnmounting,                    ///< The install flow is unmounting a disk image
                kUnpacking,                     ///< DEPRECATED.
                kDecrypting,                    ///< DEPRECATED.
                kReadyToInstall,                ///< The install flow is ready to execute the content's installer and will not do so until told to resume.
                kPreInstall,                    ///< The install flow is waiting for information needed to run the installer (i.e. 3PDD install dialog).
                kInstalling,                    ///< The content's installer is executing.
                kPostInstall,                   ///< The install flow is wrapping up installer execution.
                kCompleted                      ///< The install flow is cleaning up and about to transition back to kReadyToStart.
            }; // 'kCompleted' must remain the last entry in this enumeration.

            /// \brief Constructor.
            ContentInstallFlowState();

            /// \brief Constructor.
            ///
            /// \param v The value of this state.
            ContentInstallFlowState(enum value v);

            /// \brief Constructor.
            ///
            /// \param v Integer representation for the value of this state, i.e. -1 for ContentInstallFlowState::kInvalid
            ContentInstallFlowState(qint64 v);

            /// \brief Constructor.
            ///
            /// Invalid strings will result in a value of kInvalid
            ///
            /// \param name String representation of the value for this state, i.e. \"kInvalid\" for ContentInstallFlowState::kInvalid.
            ContentInstallFlowState(const QString& name);

            /// \brief Copy constructor.
            ///
            /// \param other The ContentInstallFlowState to copy.
            ContentInstallFlowState(const ContentInstallFlowState& other);

            /// \brief Typecast to integer representation.
            operator int() const;

            /// \brief Typecast to c style string representation.
            operator const char*() const;

            /// \brief Typecast to QString representation.
            operator QString() const;

            /// \brief Assignment operator.
            ///
            /// \param other The ContentInstallFlowState to assign.
            /// \return ContentInstallFlowState A reference to this ContentInstallFlowState.
            const ContentInstallFlowState& operator=(const ContentInstallFlowState& other);

            /// \brief Check for equality between this and a ContentInstallFlowState::value.
            ///
            /// \param v The value to compare.
            /// \return bool True if this and the ContentInstallFlowState::value are equivalent.
            bool operator==(enum value v) const;

            /// \brief Check for equality between this and another ContentInstallFlowState object.
            ///
            /// \param other The ContentInstallFlowState object in which to compare this.
            /// \return bool True if both objects represent the same state.
            bool operator==(const ContentInstallFlowState& other) const;

            /// \brief Check for inequality between this and a ContentInstallFlowState::value.
            ///
            /// \param v The value to compare.
            /// \return bool True if this and the ContentInstallFlowState::value are not equivalent.
            bool operator!=(enum value v) const;

            /// \brief Check for inequality between this and another ContentInstallFlowState object.
            ///
            /// \param other The ContentInstallFlowState object in which to compare this.
            /// \return bool  True if the objects do not represent the same state.
            bool operator!=(const ContentInstallFlowState& other) const;

            /// \brief Less than operator.
            ///
            /// \param v The value to compare.
            /// \return bool True if mValue < v.
            bool operator<(enum value v) const;

            /// \brief Less than operator.
            ///
            /// \param other The ContentInstallFlowState object to compare.
            /// \return bool True if mValue < other.mValue.
            bool operator<(const ContentInstallFlowState& other) const;

            /// \brief Retrieve the enum component of this ContentInstallFlowState.
            ///
            /// \return ContentInstallFlowState::value The enum component of this ContentInstallFlowState.
            enum value getValue() const;

            /// \brief Create a string representation of this ContentInstallFlowState.
            ///
            /// \return QString The string representation of this ContentInstallFlowState, i.e. \"kInvalid\" for ContentInstallFlowState::kInvalid.
            QString toString() const;

        private:

            /// \brief The enum state value that this object represents.
            enum value mValue;

    };

} // namespace Downloader
} // namespace Origin

inline uint qHash(Origin::Downloader::ContentInstallFlowState state)
{
    return qHash(static_cast<int>(state));
}

Q_DECLARE_METATYPE(Origin::Downloader::ContentInstallFlowState);

#endif // CONTENTINSTALLFLOWSTATE_H
