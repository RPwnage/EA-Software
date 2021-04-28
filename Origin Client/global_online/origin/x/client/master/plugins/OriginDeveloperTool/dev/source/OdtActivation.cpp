///////////////////////////////////////////////////////////////////////////////
// OdtActivation.cpp
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "OriginDeveloperTool/OdtActivation.h"
#include "TelemetryAPIDLL.h"
#include "services/platform/PlatformService.h"
#include "services/crypto/CryptoService.h"
#include "services/settings/SettingsManager.h"
#include "services/escalation/IEscalationClient.h"
#include "services/crypto/SimpleEncryption.h"
#include "version/version.h"

#include <QMessageBox>
#include <QDir>

#if defined(Q_OS_WIN)
#include "Windows.h"
#include "WinBase.h"

#include <ShlObj.h>
#endif

#include <cassert>

namespace Origin
{
namespace Plugins
{
namespace DeveloperTool
{

#define APP_NAME OBFUSCATE_STRING("Origin Developer Mode Activation Utility")
#define CONTENT_ID_PROD OBFUSCATE_STRING("71310")
#define PRODUCT_ID_PROD OBFUSCATE_STRING("OFB-EAST:40852")
#define CONTENT_ID_INT OBFUSCATE_STRING("71310_legacy")
#define PRODUCT_ID_INT OBFUSCATE_STRING("OFB-EAST:109534718")

static const QString ODT_TOOL_NAME("Origin Developer Tool");
static const QString ODT_ORG_NAME("ea.com");

OdtActivation::OdtActivation()
{
#if defined (Q_OS_WIN)
    mPlatform = "PCWIN";
#elif defined (Q_OS_MAC)
    mPlatform = "MAC";
#else
    mPlatform = "UNKNOWN";
#endif
}

OdtActivation::~OdtActivation()
{   
    deleteLicenseFile();
}

QString OdtActivation::buildActivationFilePath() const 
{
    return (appDataPath() + QDir::separator() + "odt");
}

QString OdtActivation::appDataPath() const
{
    return Services::PlatformService::odtDirectory();
}

QString OdtActivation::formatDisplay(const QString& title, const QString& description) const
{
    return QString("<br>%1<br><br>%2<br>").arg(title).arg(description);
}

OdtActivation::FailureReason OdtActivation::deleteLicenseFile()
{
    FailureReason ret = FailureReason_None;
    QStringList licenseFiles;

    // Build the path that Access writes the license files to.
#if defined (Q_OS_WIN)
    // The Access license files are stored parallel to the "Origin" application data folder.
    // This varies by OS--for example, it is C:\ProgramData\ on Win7 but C:\Users\All Users\Application Data\ on XP.
    QString licensePath = appDataPath();

    licensePath.append("..");
    licensePath.append(QDir::separator());
    licensePath.append("Electronic Arts");
    licensePath.append(QDir::separator());
    licensePath.append("EA Services");
    licensePath.append(QDir::separator());
    licensePath.append("License");
    licensePath.append(QDir::separator());
#elif defined (Q_OS_MAC)
    // On Mac, we write into the app bundle's resource folder.
    QString licensePath = QCoreApplication::applicationDirPath();
    
    // Now points to something like OriginDeveloperTool.app/Contents/MacOS
    //
    // Strip off the Contents/MacOS
    licensePath.append("..");
    licensePath.append(QDir::separator());
    licensePath.append("..");
    licensePath.append(QDir::separator());
    licensePath.append("__Installer");
    licensePath.append(QDir::separator());
    licensePath.append("OOA");
    licensePath.append(QDir::separator());
#endif

    licenseFiles.append(licensePath + CONTENT_ID_PROD + ".dlf");
    licenseFiles.append(licensePath + CONTENT_ID_INT + ".dlf");
    licenseFiles.append(licensePath + PRODUCT_ID_PROD + ".dlf");
    licenseFiles.append(licensePath + PRODUCT_ID_INT + ".dlf");

    foreach(QString it, licenseFiles)
    {
        QFile::setPermissions(it, QFileDevice::ReadOther | QFileDevice::WriteOther);
        QFile::remove(it);
    }

    return ret;
}

OdtActivation::FailureReason OdtActivation::writeActivationFile()
{
    FailureReason ret = FailureReason_None;
    QFile::FileError error = QFile::NoError;
    QString errorString;
    QString filename = buildActivationFilePath();
    int escalationError = 0;

    quint64 machineHash = Services::PlatformService::machineHash();

    const QString& directory = appDataPath();
    QDir appDataDir(directory);
    if (!appDataDir.exists())
    {
        QString escalationReasonStr = "ODT activation";
        QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
        if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
        {
            errorString = "Escalation failed";
            ret = FailureReason_EscalationFailed;
        }
        else
        {
            escalationError = escalationClient->createDirectory(directory);
            if (!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "createDirectory", escalationError, escalationClient->getSystemError()))
            {
                escalationClient->getSystemErrorText(escalationClient->getSystemError(), errorString);
                ret = FailureReason_DirectoryDeleteFailed;
            }
        }
    }

    if (ret == FailureReason_None)
    {
        QFile activationFile(filename);
        if (activationFile.open(QIODevice::ReadWrite))
        {
            QByteArray encryptedHash;
            // TODO: Get the magic number from the server!
            QByteArray activationValue = OBFUSCATE_STRING("eaf53UaPOWVx9VnEAwxG6n3O6UevyRv8qCZpR37syioHft60GIfPWmiyTEGV9QH");
            QByteArray machineHashString = QByteArray::number(machineHash);
            if (Services::CryptoService::encryptSymmetric(encryptedHash, activationValue + machineHashString, machineHashString))
            {
                activationFile.write(encryptedHash, encryptedHash.length());
            }
            else
            {
                ret = FailureReason_EncryptionFailed;
            }
            activationFile.close();
        }
        else
        {
            ret = FailureReason_FileCreationFailed;
            error = activationFile.error();
            errorString = activationFile.errorString();
            activationFile.close();
        }
    }
    
    if(ret == FailureReason_None)
    {
        GetTelemetryInterface()->Metric_DEVMODE_ACTIVATE_SUCCESS(PLUGIN_VERSION_P_DELIMITED, mPlatform.toUtf8().data());
    }
    else
    {
        GetTelemetryInterface()->Metric_DEVMODE_ACTIVATE_ERROR(PLUGIN_VERSION_P_DELIMITED, mPlatform.toUtf8().data(), ret);
    }

    // If anything failed, inform the user.
    QMessageBox::Icon dialogIcon;
    QString dialogTitle, dialogText;

    switch(ret)
    {
        case FailureReason_None:
            dialogIcon = QMessageBox::NoIcon;
            dialogTitle = "Activation successful!";
            dialogText = "Please restart Origin to enter Developer Mode.";
            break;

        case FailureReason_FileCreationFailed:
            dialogIcon = QMessageBox::Critical;
            dialogTitle = "Activation failed!";
            dialogText = QString("An error occurred while writing file \"%1\". [%2] (%3:%4)").arg(filename).arg(errorString).arg(ret).arg(error);
            break;

        case FailureReason_EmptyMachineHash:
        case FailureReason_EncryptionFailed:
        default:
            dialogIcon = QMessageBox::Critical;
            dialogTitle = "Activation failed!";
            dialogText = QString("An unknown error occurred. (%1)").arg(ret);
            break;
    }

    QMessageBox msgBox(dialogIcon, APP_NAME, formatDisplay(dialogTitle, dialogText), QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
    
#if DEBUG_ODT_ENCRYPTION
    // Decrypt our encrypted file and show the user the result to confirm that it worked.
    testEncryption();
#endif

    return ret;
}

OdtActivation::FailureReason OdtActivation::deleteActivationFile(const DeactivationMode mode)
{
#if DEBUG_ODT_ENCRYPTION
    // Decrypt our encrypted file and show the user the result before deleting it.
    testEncryption();
#endif

    FailureReason ret = FailureReason_None;
    int error = 0;
    QString errorString;

    if (Deactivation_PromptUser == mode)
    {
        // Make sure the user actually wants to delete the activation file.
        QMessageBox askMsg(QMessageBox::NoIcon, APP_NAME, formatDisplay("Would you like to deactivate Origin Developer Mode?", "This will not take effect until you restart Origin."), QMessageBox::Yes | QMessageBox::No);
        askMsg.setDefaultButton(QMessageBox::Yes);
        askMsg.exec();

        if(askMsg.standardButton(askMsg.clickedButton()) != QMessageBox::Yes)
            return ret;
    }

    // Delete the activation file and folder.
    QString directory = appDataPath();
    QDir dirInfo(directory);
    if(dirInfo.exists())
    {
        QString escalationReasonStr = "ODT deactivation";
        QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
        if(!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, error, escalationClient))
        {
            errorString = "Escalation failed";
            ret = FailureReason_EscalationFailed;
        }
        else
        {
            error = escalationClient->deleteDirectory(directory);
            if(!Origin::Escalation::IEscalationClient::evaluateEscalationResult(escalationReasonStr, "deleteDirectory", error, escalationClient->getSystemError()))
            {
                escalationClient->getSystemErrorText(escalationClient->getSystemError(), errorString);
                ret = FailureReason_DirectoryDeleteFailed;
            }
        }
    }

    // If anything failed, inform the user.
    if(ret == FailureReason_None)
    {
        GetTelemetryInterface()->Metric_DEVMODE_DEACTIVATE_SUCCESS(PLUGIN_VERSION_P_DELIMITED, mPlatform.toUtf8().data());
    }
    else
    {
        GetTelemetryInterface()->Metric_DEVMODE_DEACTIVATE_ERROR(PLUGIN_VERSION_P_DELIMITED, mPlatform.toUtf8().data(), ret);
    }
    
    if (Deactivation_PromptUser == mode)
    {
        // If anything failed, inform the user.
        QMessageBox::Icon dialogIcon;
        QString dialogTitle, dialogText;
    
        switch(ret)
        {
            case FailureReason_None:
                dialogIcon = QMessageBox::NoIcon;
                dialogTitle = "Deactivation successful!";
                dialogText = "Please restart Origin to exit Developer Mode.";
                break;

            case FailureReason_EscalationFailed:
                dialogIcon = QMessageBox::Critical;
                dialogTitle = "Deactivation failed!";
#ifdef ORIGIN_PC
                dialogText = "Windows prevented the application from deactivating.  Please accept the UAC prompt and try again.";
#else
                dialogText = "OS X prevented the application from deactivating.  Please authenticate and try again.";
#endif
                break;

            case FailureReason_DirectoryDeleteFailed:
                dialogIcon = QMessageBox::Critical;
                dialogTitle = "Deactivation failed!";
                dialogText = QString("Could not delete directory \"%1\". [%2] (%3:%4)").arg(directory).arg(errorString).arg(ret).arg(error);
                break;

            default:
                dialogIcon = QMessageBox::Critical;
                dialogTitle = "Deactivation failed!";
                dialogText = QString("An unknown error occurred. (%1)").arg(ret);
                break;
        }
    
        QMessageBox msgBox(dialogIcon, APP_NAME, formatDisplay(dialogTitle, dialogText), QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }

    return ret;
}

#if DEBUG_ODT_ENCRYPTION
FailureReason OdtActivation::testEncryption()
{
    FailureReason ret = FailureReason_None;
    
    QByteArray decryptedValue;
    QFile activationFile(buildActivationFilePath());
    quint64 machineHash = mSystemInformation.GetMachineHash();
    if(activationFile.open(QIODevice::ReadOnly))
    {
        QByteArray encryptedHash = activationFile.readAll();
        if(!encryptedHash.isEmpty())
        {
            if(machineHash != 0)
            {
                Crypto::decryptSymmetric(decryptedValue, encryptedHash, QByteArray::number(machineHash));
            }
            else
            {
                ret = FailureReason_EmptyMachineHash;
            }
        }
        else
        {
            ret = FailureReason_FileEmpty;
        }
        activationFile.close();
    }
    else
    {
        ret = FailureReason_FileNotFound;
    }
    
    // If anything failed, inform the user.
    if(ret == FailureReason_None)
    {
        QMessageBox successMsg(QMessageBox::NoIcon, APP_NAME, formatDisplay("Decryption test successful!", 
            "The decrypted value is \"" + decryptedValue + "\".  The decryption key is \"" + QByteArray::number(machineHash) + "\"."), QMessageBox::Ok);
        successMsg.setDefaultButton(QMessageBox::Ok);
        successMsg.exec();
    }
    else
    {
        QMessageBox errorMsg(QMessageBox::Critical, APP_NAME, formatDisplay("Decryption test failed!", "Error code: " + QString::number(ret) + "."), QMessageBox::Ok);
        errorMsg.setDefaultButton(QMessageBox::Ok);
        errorMsg.exec();
    }

    return ret;
}
#endif

bool OdtActivation::activate(FailureReason* failureReason)
{
    OdtActivation activation;
    activation.deleteLicenseFile();

    // This setting should only be true if the ODT activation file has been
    // read and decrypted successfully by the client at startup.
    bool activateSuccessful = Origin::Services::readSetting(Origin::Services::SETTING_OriginDeveloperToolEnabled, Origin::Services::Session::SessionRef());

    if (!activateSuccessful)
    {
        // Prompt the user to confirm machine activation
        QString windowTitle("Confirm Activation");
        QString windowText("Would you like to enable Developer Mode for this machine?");
        QMessageBox msgBox(QMessageBox::Question, APP_NAME, activation.formatDisplay(windowTitle, windowText), QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        const QMessageBox::StandardButton dialogResult = static_cast<QMessageBox::StandardButton>(msgBox.exec());

        if (QMessageBox::Ok == dialogResult)
        {
            FailureReason& reasonRef = *failureReason;

            // Delete existing file if one exists.
            reasonRef = activation.deleteActivationFile(Deactivation_Force);
            if (reasonRef == FailureReason_None)
            {
                // Create new activation file.
                reasonRef = activation.writeActivationFile();
            }
        }
    }

    // Even if the activation file write was successful we only want to base
    // a "successful activation" off of whether the DeveloperToolEnabled
    // setting has been set to true (which only happens at startup). This
    // forces the user to reboot into developer mode.
    return activateSuccessful;
}

void OdtActivation::deactivate(const DeactivationMode mode)
{
    OdtActivation deactivation;

    deactivation.deleteLicenseFile();

    if(QFile::exists(deactivation.buildActivationFilePath()))
    {
        deactivation.deleteActivationFile(mode);
    }
    else if (Deactivation_PromptUser == mode)
    {
        QMessageBox askMsg(QMessageBox::NoIcon, APP_NAME, deactivation.formatDisplay("Origin Developer Mode already deactivated!", "This will not take effect until you restart Origin."), QMessageBox::Ok);
        askMsg.setDefaultButton(QMessageBox::Ok);
        askMsg.exec();
    }
}

} // namespace DeveloperTool
} // namespace Plugins
} // namespace Origin
