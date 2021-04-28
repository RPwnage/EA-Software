///////////////////////////////////////////////////////////////////////////////
/// \file       OOALicenseManager.cpp
/// This file contains the implementation of the OOALicenseManager.  It keeps
/// saved OOA licenses up to date so the user can access their protected
/// content even if they're offline for an extended period of time.
///
///	\author     James Fairweather
/// \date       2010-2015
/// \copyright  Copyright (c) 2014 Electronic Arts. All rights reserved.

#include "engine/subscription/OOALicenseManager.h"
#include "engine/content/ContentController.h"
#include "services/session/SessionService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/platform/TrustedClock.h"

#include "OOA/Core/CorePreferences.h"
#include "OOA/Core/CoreProcess.h"
#include "OOA/Core/CoreString.h"
#include "OOA/Core/CoreXProcessState.h"
#include "OOA/Core/CoreFinder.h"
#include "OOA/DRM/AccessDRMWrapperDefs.h"
#include "OOA/AccessDigital/PublicDSAKey.h"
#include "OOA/AccessDigital/AccessDRM_AccessDigital.h"
#include "OOA/LauncherInterface.h"

#include <openSSL/evp.h>
#include <openSSL/dsa.h>

#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#include <ImageHlp.h>
#endif

using namespace std;

namespace Origin
{
    namespace Engine
    {
        namespace Subscription
        {
            bool VerifyOOALicenseSignature(S_License *license)
            {
                bool validSignature = false;
                DSA_SIG *pdsa_signature = DSA_SIG_new();

                const unsigned char* pLicSig = license->m_Signature;

                if (d2i_DSA_SIG(&pdsa_signature, &pLicSig, license->m_iSignatureLen) != NULL)
                {
                    // Create the digest of the data
                    EVP_MD_CTX mdctx;
                    const EVP_MD *md = EVP_sha1();
                    unsigned int digestLen;
                    unsigned char digest[EVP_MAX_MD_SIZE];

                    EVP_MD_CTX_init(&mdctx);
                    EVP_DigestInit(&mdctx, md);
                    EVP_DigestUpdate(&mdctx, static_cast<const void*>(license->m_LicenseEncXML), license->m_iLicenseEncXMLSize);
                    EVP_DigestFinal(&mdctx, digest, &digestLen);

                    DSA *dsa_key = DSA_new();

                    dsa_key->p = BN_bin2bn(kDSA1024_p, sizeof(kDSA1024_p), NULL);
                    dsa_key->q = BN_bin2bn(kDSA1024_q, sizeof(kDSA1024_q), NULL);
                    dsa_key->g = BN_bin2bn(kDSA1024_g, sizeof(kDSA1024_g), NULL);
                    dsa_key->pub_key = BN_bin2bn(kDSA1024_pub, sizeof(kDSA1024_pub), NULL);

                    validSignature = DSA_do_verify(digest, digestLen, pdsa_signature, dsa_key) == 1;

                    DSA_free(dsa_key);
                }

                DSA_SIG_free(pdsa_signature);

                return validSignature;
            }

#ifdef WIN32
            bool GetDenuvoGameToken(const QString& exeDir, std::wstring& gameToken)
            {
                Core::CoreXProcessState *launcherProcState = new Core::CoreXProcessState(0);
                launcherProcState->Open_MemBased(NULL, true);
                wchar_t SMOID[16];
                swprintf_s(SMOID, _countof(SMOID), L"%d", launcherProcState->GetSharedMapIdentifier());

                QString dbdataLocation(exeDir + "\\dbdata.dll");

                if (!QFile(dbdataLocation).exists())
                {
                    // This game isn't protected by Denuvo.  No GameToken is needed
                    return false;
                }

                // There is a dbdata.dll in the game executable directory, so we need to pass a Game Token
                // to the OOA license server.  Get the architecture of the DLL.
                QByteArray qbarray = dbdataLocation.toLatin1();
                LOADED_IMAGE *li = ImageLoad(qbarray.data(), NULL);

                if (!li)
                {
                    // the dbdata.dll is corrupt (send a telemetry event?)
                    return false;
                }

                char * strGetGameTokenExeName = NULL;
                if (li->FileHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
                {
                    // use the 64-bit version of the loader program
                    strGetGameTokenExeName = "GetGameToken64.exe";
                }
                else if (li->FileHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
                {
                    // use the 32-bit version of the loader program
                    strGetGameTokenExeName = "GetGameToken32.exe";
                }
                ImageUnload(li);

                char szOriginPath[MAX_PATH];
                ::GetModuleFileNameA(::GetModuleHandle(NULL), szOriginPath, MAX_PATH);
                QString strGetGameTokenExePath(szOriginPath);

                // Replace Origin executable with dbdataloader executable
                int lastSlash = strGetGameTokenExePath.lastIndexOf(L'\\');
                dbdataLocation = strGetGameTokenExePath.mid(0, lastSlash+1).append(strGetGameTokenExeName);

                if (!QFileInfo(strGetGameTokenExePath).exists())
                {
                    // The GetGameToken executable _should_ be here.  Maybe send a corrupt installation telemetry event?
                    return false;
                }

                // Finally we have our launcher executable & game install directory.  Get the Game Token.
                Core::ICoreProcess *launcherProc = new Core::CoreProcess_Windows(dbdataLocation.toStdWString().c_str(), SMOID, exeDir.toStdWString().c_str(), NULL, false);
                launcherProc->Open();
                launcherProc->WaitForExit(true, 30*1000);
                launcherProc->Close();
                delete launcherProc;

                launcherProcState->GetItem(L"GameToken", gameToken);

                delete launcherProcState;

                return !gameToken.empty();
            }
#else
            bool GetDenuvoGameToken(const QString& exeDir, std::wstring& gameToken)
            {
                return false;
            }
#endif

#ifdef WIN32
            // Find the location of the OOA-protected exe
            bool GetProtectedExePath(const QString& strOriginExePath, QString& strProtectedExePath, bool& isOOA4)
            {
                QDir dir = QFileInfo(strOriginExePath).absoluteDir();

                QString currDir = dir.dirName();

                currDir += " EN/";

                QString exeDir = dir.absolutePath();

                const QString parFileFilter = "*.par";

                const QString searchDirs[3] = 
                {
                    QString("."),
                    QString("../"),
                    QString(currDir),
                };

                QStringList qsl;
                qsl += parFileFilter;

                bool foundProtectedExe = false;

                for (int i = 0; i < 3 && !foundProtectedExe; ++i)
                {
                    QDir testDir = exeDir;
                    testDir.cd(searchDirs[i]);

                    QStringList fileslist = testDir.entryList(qsl);

                    // is there a .par file?
                    if (fileslist.count() != 0)
                    {
                        // There is a .par file in this directory.  See if theres a Core/ActivationUI program as well.
                        const QString sAppFileName = "Core/ActivationUI.exe";

                        // is there a core subdirectory?
                        if (testDir.exists(QString(sAppFileName)))
                        {
                            foundProtectedExe = true;
                        }

                        // some games wrapped with an older version of EAAccess (now OOA) use the name
                        // Activation.exe instead of ActivationUI.exe
                        const QString sOldAppFileName = "Core/Activation.exe";
                        if (testDir.exists(sOldAppFileName))
                        {
                            isOOA4 = false;
                            foundProtectedExe = true;
                        }

                        if (foundProtectedExe)
                            strProtectedExePath = testDir.absolutePath() + dir.separator() + fileslist[0].replace(".par", ".exe");
                    }
                }

                return foundProtectedExe;
            }
#endif

            bool UpdateOOALicense(bool bIsUserOnline, bool hasOfflineTimeRemaining, const wchar_t *sContentId, const QString &strExePath)
            {
                AccessDRMLicense_AccessDigitalV4 drm;

                bool bResult = false;

#ifdef __MACH__
                {
                    g_oBaseParams->SetWrappedExecutablePath(strExePath.toStdWString().c_str());
                
                    QDir exeDir = QFileInfo(strExePath).absoluteDir();
                
                    exeDir.cd("../../");
                
                    g_oBaseParams->SetWrappedAppBundlePath(exeDir.absolutePath().toStdWString().c_str());
                }
#endif

                if (!drm.LoadLicenseFromDisk(sContentId))
                {
                    ORIGIN_LOG_DEBUG << "License file for contentId " << sContentId << " is not available.";

                    // we're only going to get new licenses if the game was previously activated
                    return false;
                }

				bool isLicenseValid = true;

                if (!VerifyOOALicenseSignature(&drm.mLicense))
                {
                    ORIGIN_LOG_DEBUG << "Signature on the license file for contentId " << sContentId << " is not valid.  Refreshing existing license file.";
                    AccessDRMLicense_AccessDigitalV4::DeleteLicenseFile(sContentId);
					isLicenseValid = false;
                }

                XML::SPNODE pRootNode;
                if (!drm.Decrypt(drm.mLicense.m_LicenseEncXML, drm.mLicense.m_iLicenseEncXMLSize, pRootNode))
                {
                    ORIGIN_LOG_DEBUG << "Decryption failed for license contentId " << sContentId << ".  Refreshing the existing license file.";
					isLicenseValid = false;
                }

                qint64 iEndTimeMs = 0;
                std::wstring strMachineHash;

				if (isLicenseValid)
				{
					// TODO: make this a QA-specific feature.  It's useful for testing but it cannot be released to end-users.
					{
						std::wstring endTimeOverride;

						Core::IniFile coreIniFile(Origin::Services::PlatformService::eacoreIniFilename().toStdWString().c_str());
						coreIniFile.ReadEntry(L"LicenseEndTimes", sContentId, endTimeOverride);

						if (!endTimeOverride.empty())
						{
							iEndTimeMs = AccessDRMLicense_AccessDigitalV4::StringTimeInMilliSeconds(endTimeOverride);
						}
					}

					if (iEndTimeMs == 0)
					{
						// Get the license end time
						XML::SPNODE pNode = pRootNode->GetChild(L"EndTime");
						if (pNode->GetResult()->GetStatus() == XML::s_OK)
						{
							iEndTimeMs = AccessDRMLicense_AccessDigitalV4::StringTimeInMilliSeconds(pNode->GetChild(L"#text")->GetValue());
						}
					}

					{
						XML::SPNODE pNode = pRootNode->GetChild(L"MachineHash");
						if (pNode->GetResult()->GetStatus() == XML::s_OK)
						{
							strMachineHash = pNode->GetChild(L"#text")->GetValue();
						}
					}

					if (iEndTimeMs == 0)
					{
						// permanent license (no end time).  Take no action.
						return false;
					}
					else
					{
						{
							XML::SPNODE pNode = pRootNode->GetChild(L"LicenseType");
							if (pNode->GetResult()->GetStatus() == XML::s_OK && (pNode->GetChild(L"#text")->GetValue() == L"SESSIONAL"))
							{
								return false;
							}
						}

						// subs license.  See if it needs to be deleted or renewed.
						if (!hasOfflineTimeRemaining)
						{
							// Offline play time has expired and this is a non-permanent license obtained for a subscription-based
							// game.  Delete the OOA license file.

							// According to the subs PRD, if the user has been offline for an extended period of time any saved
							// license files should be removed as we cannot be sure they're valid any more.  The user will need
							// to go online and renew them.
							AccessDRMLicense_AccessDigitalV4::DeleteLicenseFile(sContentId);
						}
						else
						{
							// if the license expires in less than 30 days, get a new one.
							const qint64 now = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch();
							const qint64 timeRemaining = iEndTimeMs - now;


							if (timeRemaining >= SUBSCRIPTION_MAX_OFFLINE_PLAY_TIME)
							{
								// there are 30 or more days remaining until this license expires.  Just keep the
								// existing license.
								return false;
							}
						}
					}
				}
				else // !isLicenseValid
				{
					// license file is invalid (has been tampered with)
                    AccessDRMLicense_AccessDigitalV4::DeleteLicenseFile(sContentId);
				}

				// This license is corrupt or invalid, or it's a subs license that needs to be renewed so the user can continue
				// playing offline.

				ORIGIN_LOG_DEBUG << "License for contentId " << sContentId << " has expired or is close to expiring.  Refreshing it";

#ifdef WIN32
                QString protectedExePath;
                bool isOOA4 = true;
                if (!GetProtectedExePath(strExePath, protectedExePath, isOOA4))
                {
                    // Can't find the protected executable image.  Cannot refresh the
                    // license for this content.
                    return false;
                }
#else
                // Less legacy crap on OSX.  The protected executable is the one
                // Origin launched.
                QString protectedExePath = strExePath;
                bool isOOA4 = true;
#endif

                QString exeDir = QFileInfo(protectedExePath).absoluteDir().absolutePath();

                // rename the existing license file as it may still be valid for play for a while
                // and ActivationUI won't try to get a new one unless the current one is actually
                // expired.
                const QString strCurrLicensePath = QString::fromStdWString(drm.GetLicPath(sContentId));

                QString tmpLicenseFileName = strCurrLicensePath;
                tmpLicenseFileName.replace(".dlf", ".tmp");

                QFile qfCurrLicenseFile;

                if (QFile::exists(strCurrLicensePath))
                {
                    // an active license is being updated, so keep the existing one in case the refresh fails.
                    qfCurrLicenseFile.setFileName(strCurrLicensePath);
                    qfCurrLicenseFile.rename(tmpLicenseFileName);
                }

                E_LaunchFailure reLaunchFailure = kLF_Invalid_LF_State;

                if (bIsUserOnline)
                {
                    // This license is expiring in 30 days or less.  Renew it so the user can continue playing offline.
                    AccessDRM_AccessDigitalV4 accessDRM;

                    std::string sB64CipherKey;
                    int64_t ruiTimeRemaining;
                    bool bSessionOnly;
                    int riLaunchFailureReasonCode;
                    bool bGetNewLicIfNeeded = true;
                    std::wstring sDenuvoRequestToken;

                    GetDenuvoGameToken(exeDir, sDenuvoRequestToken);
                    Origin::Services::Session::AccessToken token = Origin::Services::Session::SessionService::accessToken(
                        Origin::Services::Session::SessionService::currentSession());
                    std::wstring t = token.toStdWString();
                    const wchar_t* sUserEADMAuthToken = t.c_str();

                    std::wstring pszUnentitledNucleusAuthToken;
                    std::wstring pszUnentitledEADMAuthToken;
                    bool bForceNewLicense = true;

                    g_oBaseParams->SetContentIds(sContentId);

                    accessDRM.VerifyDRM(sB64CipherKey,
                        ruiTimeRemaining,
                        bSessionOnly,
                        reLaunchFailure,
                        riLaunchFailureReasonCode,
                        bGetNewLicIfNeeded,
                        sDenuvoRequestToken.c_str(),
                        /* sUserName */ NULL,
                        /* sPassword */ NULL,
                        sUserEADMAuthToken,
                        strMachineHash.c_str(),
                        &pszUnentitledNucleusAuthToken,
                        &pszUnentitledEADMAuthToken,

                        bForceNewLicense,
                        isOOA4,
                        true
                        );
                }

                if (QFile::exists(strCurrLicensePath))
                {
                    bResult = true;
                }
                else
                {
                    ORIGIN_LOG_DEBUG << "License refresh failed for contentId " << sContentId << ".  E_LaunchFailure code: " << reLaunchFailure;
                }

                // if we have a copy of the old license file, delete it if the renewal was
                // successful (it's no longer needed), or restore it from the temporary
                // copy if the renewal failed.
                if (bResult)
                {
                    QFile::remove(tmpLicenseFileName);
                }
                else
                {
                    qfCurrLicenseFile.rename(strCurrLicensePath);
                }

                return bResult;
            }

            void RefreshOOALicenses(bool bIsUserOnline, bool hasOfflineTimeRemaining)
            {
                static bool isInitialized = false;

                if (!isInitialized)
                {
                    Core::InitGlobalPrefMgr(Origin::Services::PlatformService::eacoreIniFilename().toStdWString().c_str());
                    ::InitGlobalBaseParams();
                }

                Origin::Engine::Content::ContentController *contentController = Origin::Engine::Content::ContentController::currentUserContentController();

                QList< Origin::Engine::Content::EntitlementRef> entitlements = contentController->entitlements();

                foreach(Origin::Engine::Content::EntitlementRef entitlement, entitlements)
                {
                    if (entitlement->localContent()->installed() && entitlement->contentConfiguration()->isEntitledFromSubscription())
                    {
                        QString exePath(entitlement->localContent()->executablePath());
                        QString contentId(entitlement->contentConfiguration()->contentId());

                        if (!exePath.isEmpty() && !contentId.isEmpty())
                        {
                            UpdateOOALicense(bIsUserOnline, hasOfflineTimeRemaining, contentId.toStdWString().c_str(), exePath);

#ifdef WIN32
                            // slow down pace of license server requests
                            Sleep(1000);
#endif
                        }
                    }
                }
            }
        }
    }
}

