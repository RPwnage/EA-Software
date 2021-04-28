///////////////////////////////////////////////////////////////////////////////
// OdtActivation.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ODTACTIVATION_H
#define ODTACTIVATION_H

#include <QObject>
#include <QString>

namespace Origin
{
namespace Plugins
{
namespace DeveloperTool
{

/// \brief Enables or disables Origin Developer Mode on the user's machine.
class OdtActivation : public QObject
{
	Q_OBJECT

public:

    enum FailureReason
    {
        FailureReason_None = 0,
        FailureReason_EscalationFailed,
        FailureReason_DirectoryDeleteFailed,
        FailureReason_EmptyMachineHash,
        FailureReason_FileCreationFailed,
        FailureReason_DirectoryCreationFailed,
        FailureReason_FileNotFound,
        FailureReason_EncryptionFailed,
        FailureReason_DecryptionFailed,
        FailureReason_FileEmpty
    };

    /// \brief Executes ODT activation flow
    /// \return True if ODT has been activated, false otherwise
	static bool activate(FailureReason* failureReason);

    enum DeactivationMode
    {
        Deactivation_PromptUser,
        Deactivation_Force
    };

    /// \brief Executes ODT de-activation flow
    static void deactivate(const DeactivationMode mode = Deactivation_PromptUser);

private:

    /// \brief The OdtActivation constructor.
	OdtActivation();
    
    /// \brief The OdtActivation destructor.
	~OdtActivation();

    /// \brief Builds the file path to the activation file.
    /// \return The file path to the activation file.
    QString buildActivationFilePath() const;
    
    /// \brief Builds the path to the ODT app data folder.
    /// \return The path to the ODT app data folder.
    QString appDataPath() const;

    /// \brief Format string for display in a message box.
    /// \param title The title.  This will be bolded.
    /// \param description The description.  This will appear normally.
    /// \return The formatted string.
    QString formatDisplay(const QString& title, const QString& description) const;
    
    /// \brief Deletes the Access (OOA) license file.
    /// \return The failure reason, if any.
    FailureReason deleteLicenseFile();

    /// \brief Deletes the ODT activation file.
    /// \return The failure reason, if any.
    FailureReason deleteActivationFile(const DeactivationMode mode);

    /// \brief Writes the ODT activation file.
    /// \return The failure reason, if any.
    FailureReason writeActivationFile();

#if DEBUG_ODT_ENCRYPTION
    /// \brief Decrypts the activation file and displays the result.  Intended for debugging only.
    /// \return The failure reason, if any.
    FailureReason testEncryption();
#endif

    QString mPlatform; ///< The current platform.
};

} // namespace DeveloperTool
} // namespace Plugins
} // namespace Origin

#endif