///////////////////////////////////////////////////////////////////////////////
// EscalationServiceWin.cpp
//
// Created by Paul Pedriana & Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// References:
//
// http://www.kinook.com/blog/?p=10
//
///////////////////////////////////////////////////////////////////////////////

#include "EscalationServiceWin.h"
#include "CommandProcessorWin.h"

#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "wintrust.lib")	//for exe signing verification
#pragma comment(lib, "crypt32.lib")		//for cert info verification

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

namespace Origin
{

    namespace Escalation
    {
        static QMap<ProcessId, QString> sProcessKillList;

        void EscalationServiceWin::addToMonitorListInternal(QString response)
        {
            QStringList monitorProcessResponseList = response.split(",");
            if(monitorProcessResponseList.size() == 3)
            {
                qint32 pid = monitorProcessResponseList.at(1).toInt();
                QString exeName = monitorProcessResponseList.at(2);
                sProcessKillList[pid] = exeName;
            }
            else
            {
                ORIGIN_LOG_ERROR << "addToMonitorListInternal -- incorrect response format";
            }
        }

        void EscalationServiceWin::removeFromMonitorListInternal(QString response)
        {
            QStringList monitorProcessResponseList = response.split(",");
            if(monitorProcessResponseList.size() == 2)
            {
                qint32 pid = monitorProcessResponseList.at(1).toInt();
                sProcessKillList.remove(pid);
            }
            else
            {
                ORIGIN_LOG_ERROR << "removeFromMonitorListInternal -- incorrect response format";
            }

        }

        EscalationServiceWin::EscalationServiceWin(bool runningAsService, QStringList serviceArgs, QObject* parent) : IEscalationService(runningAsService, serviceArgs, parent), mRunningAsService(runningAsService)
        {
            // we watch the Origin process to determinate, when the OriginEscalationClient.exe will go away
            hProcessWatchingThread = 0;

            if (mCallerProcessId != 0)
            {
                hProcessWatchingThread = CreateThread(0, 0, EscalationServiceWin::processWatcher, (void*)mCallerProcessId, 0, 0);
            }
        }

        EscalationServiceWin::~EscalationServiceWin()
        {
            if(hProcessWatchingThread)
            {
                CloseHandle(hProcessWatchingThread);
                hProcessWatchingThread = NULL;
            }
        }

        CommandError EscalationServiceWin::validateCaller(const quint32 clientPid)
        {
            CommandError result = kCommandErrorUnknown;

            // Set the caller process pid (which we retrieved from the named pipe)
            mCallerProcessId = clientPid;

            if(mCallerProcessId)
            {
                // Get the caller process path.
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, mCallerProcessId);

                if(hProcess)
                {
                    wchar_t processPath[MAX_PATH] = { 0 };
                    qint32 dwLength = GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH);		// returns an 8.3 path but long EXE name (odd that)

                    if(dwLength > (Origin::Escalation::ORIGIN_APP_NAME+".exe").length())		// must be at least this length to be valid
                    {
                        wchar_t longProcessPath[MAX_PATH] = { 0 };
                        dwLength = GetLongPathName(processPath, longProcessPath, MAX_PATH);

                        // Here we validate the process or process binary.

                        // Validate that the caller is one of our known good processes
                        // Check for a signature and also check for matching certificates between
                        // the calling process and the OriginClientService.exe
                        QString sProcessPath = QString::fromUtf16((const ushort*)&longProcessPath[0], dwLength);

                        long nProcessPathLength = sProcessPath.length();
                        if (sProcessPath.mid( nProcessPathLength - (Origin::Escalation::ORIGIN_APP_NAME+".exe").length() ) == (Origin::Escalation::ORIGIN_APP_NAME+".exe"))
                        {
                            //if we cannot find the processId in the set, the processId must be validated -- first by name, then check if its signed
                            if(verifyEmbeddedSignature(sProcessPath.utf16()) && validateCert(sProcessPath.utf16()))
                            {
                                ORIGIN_LOG_EVENT << "Successfully validated caller PID: " << mCallerProcessId << " Path: " << sProcessPath;

                                result = kCommandErrorNone;

                                //lets save off the processId so we can skip this check later
                            }
                            else
                            {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                                ORIGIN_LOG_ERROR << "Failed to verify embedded signature or cert for process path: " << sProcessPath;
#endif
                                result = kCommandErrorCallerInvalid;
                            }
                        }
                        else
                        {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                            ORIGIN_LOG_ERROR << "Process path [" << sProcessPath << "] didn't include expected Origin application name [" << (Origin::Escalation::ORIGIN_APP_NAME+".exe") << "]";
#endif
                            result = kCommandErrorCallerInvalid;
                        }
                    }
                    else
                    {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                        ORIGIN_LOG_ERROR << "Process path length [" << dwLength << "] was less than required [" << (Origin::Escalation::ORIGIN_APP_NAME+".exe").length() << "]";
#endif
                        result = kCommandErrorCallerInvalid;
                    }

                    CloseHandle(hProcess);
                }
                else
                {
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_ERROR << "Failed to open process [" << mCallerProcessId << "] in order to query information.";
#endif
                    result = kCommandErrorCallerInvalid;
                }
                
            }
            else
                result = kCommandErrorCallerInvalid;

            return result;
        }


        DWORD WINAPI EscalationServiceWin::processWatcher(LPVOID data)
        {
            ProcessId processId   = (ProcessId)data;
            HANDLE hProcess    = OpenProcess(SYNCHRONIZE, FALSE, processId);
            ProcessId  returnValue = -1;

            if(hProcess)
            {
                WaitForSingleObject(hProcess, INFINITE);

                for(QMap<ProcessId, QString>::ConstIterator it = sProcessKillList.constBegin();
                    it != sProcessKillList.constEnd();
                    it++)
                {
                    ProcessId gamePid = it.key();
                    QString gameExeName = it.value();

                    HANDLE hGameProcess    = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, gamePid);
                    // Different operating systems behave differently with this call.  XP doesn't support PROCESS_QUERY_LIMITED_INFORMATION,
                    // but in some situations Windows 7 fails
                    if (!hGameProcess)
                        hGameProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, gamePid);

                    if(hGameProcess)
                    {
                        const int UNICODE_MAX_PATH = 32767;
                        TCHAR exeString[UNICODE_MAX_PATH];
                        GetProcessImageFileName(hGameProcess, exeString, MAX_PATH);
                        QString exePath = QString::fromWCharArray(exeString);

                        if(exePath.endsWith(gameExeName, Qt::CaseInsensitive))
                        {
                            ::TerminateProcess(hGameProcess, 0xf291);
                        }
                        ORIGIN_LOG_WARNING << "WATCHER: " << exePath <<" PID: " << gamePid <<" SAVED EXE: " << gameExeName;
                    }
                }

                ORIGIN_LOG_EVENT << "Origin exited.  Shutting down.";

                QCoreApplication::exit(0);
                returnValue = 0; // success.
            }

            return returnValue;
        }

        // VerifyEmbeddedSignature
        //
        // Takes the path of a file and checks to see if it has a valid signature
        //
        // http://msdn.microsoft.com/en-us/library/ff554705%28VS.85%29.aspx
        // http://msdn.microsoft.com/en-us/library/aa382384(v=VS.85).aspx
        bool EscalationServiceWin::verifyEmbeddedSignature(LPCWSTR pwszSourceFile)
        {
#if !defined(CHECK_SIGNATURES)
            return true;
#else
            LONG lStatus;
            quint32 dwLastError;
            bool verified = false;
            // Initialize the WINTRUST_FILE_INFO structure.

            WINTRUST_FILE_INFO FileData;
            memset(&FileData, 0, sizeof(FileData));
            FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
            FileData.pcwszFilePath = pwszSourceFile;
            FileData.hFile = NULL;
            FileData.pgKnownSubject = NULL;

            /*
            WVTPolicyGUID specifies the policy to apply on the file
            WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:

            1) The certificate used to sign the file chains up to a root 
            certificate located in the trusted root certificate store. This 
            implies that the identity of the publisher has been verified by 
            a certification authority.

            2) In cases where user interface is displayed (which this example
            does not do), WinVerifyTrust will check for whether the  
            end entity certificate is stored in the trusted publisher store,  
            implying that the user trusts content from this publisher.

            3) The end entity certificate has sufficient permission to sign 
            code, as indicated by the presence of a code signing EKU or no 
            EKU.
            */

            GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
            WINTRUST_DATA WinTrustData;

            // Initialize the WinVerifyTrust input data structure.

            // Default all fields to 0.
            memset(&WinTrustData, 0, sizeof(WinTrustData));

            WinTrustData.cbStruct = sizeof(WinTrustData);

            // Use default code signing EKU.
            WinTrustData.pPolicyCallbackData = NULL;

            // No data to pass to SIP.
            WinTrustData.pSIPClientData = NULL;

            // Disable WVT UI.
            WinTrustData.dwUIChoice = WTD_UI_NONE;

            // No revocation checking.
            WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE; 

            // Verify an embedded signature on a file.
            WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

            // Default verification.
            WinTrustData.dwStateAction = 0;

            // Not applicable for default verification of embedded signature.
            WinTrustData.hWVTStateData = NULL;

            // Not used.
            WinTrustData.pwszURLReference = NULL;

            // Only verify that the signer matches. This allows builds with dev certificates to be
            // tested by QA outside the EA domain.
            WinTrustData.dwProvFlags = WTD_HASH_ONLY_FLAG;

            // This is not applicable if there is no UI because it changes 
            // the UI to accommodate running applications instead of 
            // installing applications.
            WinTrustData.dwUIContext = 0;

            // Set pFile.
            WinTrustData.pFile = &FileData;

            // WinVerifyTrust verifies signatures as specified by the GUID 
            // and Wintrust_Data.
            lStatus = WinVerifyTrust(
                NULL,
                &WVTPolicyGUID,
                &WinTrustData);

            switch (lStatus) 
            {
            case ERROR_SUCCESS:
                /*
                Signed file:
                - Hash that represents the subject is trusted.

                - Trusted publisher without any verification errors.

                - UI was disabled in dwUIChoice. No publisher or 
                time stamp chain errors.

                - UI was enabled in dwUIChoice and the user clicked 
                "Yes" when asked to install and run the signed 
                subject.
                */
#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_EVENT << "Successfully verified that file [" << pwszSourceFile << "] was signed and signature is verified.";
#endif

#ifdef SIGN_VERIFY_DEBUG_MSG
                wprintf_s(L"The file \"%s\" is signed and the signature "
                    L"was verified.\n",
                    pwszSourceFile);
#endif
                verified = true;
                break;

            case TRUST_E_NOSIGNATURE:
                // The file was not signed or had a signature 
                // that was not valid.

                // Get the reason for no signature.
                dwLastError = GetLastError();
                if (TRUST_E_NOSIGNATURE == dwLastError ||
                    TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
                    TRUST_E_PROVIDER_UNKNOWN == dwLastError) 
                {
                    // The file was not signed.
#ifdef SIGN_VERIFY_DEBUG_MSG
                    wprintf_s(L"The file \"%s\" is not signed.\n",
                        pwszSourceFile);
#endif

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_ERROR << "Failed to verify embedded signature for file [" << pwszSourceFile << "]: File was not signed [last error: " << dwLastError << "].";
#endif
                } 
                else 
                {
                    // The signature was not valid or there was an error 
                    // opening the file.
#ifdef SIGN_VERIFY_DEBUG_MSG
                    wprintf_s(L"An unknown error occurred trying to "
                        L"verify the signature of the \"%s\" file.\n",
                        pwszSourceFile);
#endif

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                    ORIGIN_LOG_ERROR << "Failed to verify embedded signature for file [" << pwszSourceFile << "]: Unknown error occurred attempting to verify.";
#endif
                }

                break;

            case TRUST_E_EXPLICIT_DISTRUST:
                // The hash that represents the subject or the publisher 
                // is not allowed by the admin or user.
#ifdef SIGN_VERIFY_DEBUG_MSG
                wprintf_s(L"The signature is present, but specifically "
                    L"disallowed.\n");
#endif

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to verify embedded signature for file [" << pwszSourceFile << "]: Signature is present, but specifically disallowed.";
#endif
                break;

            case TRUST_E_SUBJECT_NOT_TRUSTED:
                // The user clicked "No" when asked to install and run.
#ifdef SIGN_VERIFY_DEBUG_MSG
                wprintf_s(L"The signature is present, but not "
                    L"trusted.\n");
#endif

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to verify embedded signature for file [" << pwszSourceFile << "]: Signature is present, but not trusted.";
#endif
                break;

            case CRYPT_E_SECURITY_SETTINGS:
                /*
                The hash that represents the subject or the publisher 
                was not explicitly trusted by the admin and the 
                admin policy has disabled user trust. No signature, 
                publisher or time stamp errors.
                */
#ifdef SIGN_VERIFY_DEBUG_MSG
                wprintf_s(L"CRYPT_E_SECURITY_SETTINGS - The hash "
                    L"representing the subject or the publisher wasn't "
                    L"explicitly trusted by the admin and admin policy "
                    L"has disabled user trust. No signature, publisher "
                    L"or timestamp errors.\n");
#endif

#ifdef ENABLE_ESCALATION_SERVICE_LOGGING
                ORIGIN_LOG_ERROR << "Failed to verify embedded signature for file [" << pwszSourceFile << "]: The hash " <<
                    "representing the subject or the publisher wasn't " <<
                    "explicitly trusted by the admin and admin policy " <<
                    "has disabled user trust. No signature, publisher " <<
                    "or timestamp errors.";
#endif

                break;

            default:
                // The UI was disabled in dwUIChoice or the admin policy 
                // has disabled user trust. lStatus contains the 
                // publisher or time stamp chain error.
#ifdef SIGN_VERIFY_DEBUG_MSG
                wprintf_s(L"Error is: 0x%lx.\n",
                    lStatus);
#endif
                break;
            }

            return verified;
#endif
        }

        // ValidateCert
        // 
        // Compares the calling process certificate vs the OriginClientService.exe cert
        bool EscalationServiceWin::validateCert(LPCWSTR szCallingProcessFileName)
        {
#if !defined(CHECK_SIGNATURES)
            return true;
#else
            bool validated = false;
            wchar_t processPath[MAX_PATH] = { 0 };
            qint32 dwLength = GetModuleFileNameW( NULL, processPath, MAX_PATH);		// returns an 8.3 path but long EXE name (odd that)
            PCCERT_CONTEXT callingProcessCert=NULL, originClientServiceCert=NULL;

            memset(&callingProcessCert, 0, sizeof(callingProcessCert));
            memset(&originClientServiceCert, 0, sizeof(originClientServiceCert));

            if(dwLength > (Origin::Escalation::ESCALATION_SERVICE_NAME+".exe").length())		// must be at least this length to be valid
            {
                wchar_t longProcessPath[MAX_PATH] = { 0 };
                dwLength = GetLongPathName(processPath, longProcessPath, MAX_PATH);
                QString sProcessPath = QString::fromUtf16((const ushort*)&longProcessPath[0], dwLength);

                long nProcessPathLength = sProcessPath.length();
                if (sProcessPath.mid( nProcessPathLength - (Origin::Escalation::ESCALATION_SERVICE_NAME+".exe").length() ) == (Origin::Escalation::ESCALATION_SERVICE_NAME+".exe"))
                {
                    //get the certificate context for the Origin.exe and OriginClientService.exe
                    //this will allocate memory for the returned context so we must free it below
                    getCertificateContext((LPCWSTR)sProcessPath.utf16(), originClientServiceCert);
                    getCertificateContext(szCallingProcessFileName, callingProcessCert);

                    if(originClientServiceCert && callingProcessCert)
                    {
                        //see if they are the same certificate
                        validated= CertCompareCertificate(ENCODING, originClientServiceCert->pCertInfo, callingProcessCert->pCertInfo);

                        //free the context
                        CertFreeCertificateContext(originClientServiceCert);
                        CertFreeCertificateContext(callingProcessCert);
                    }
                }
            }

            if (!validated)	// if this is false that means the certs didn't match.  This should never happen for end users so this error dialog should only ever be visible for internal devs only
            {
                if (!mRunningAsService)
                    MessageBox(NULL, L"Dev: Cert check failed!  Check Origin build.", L"Certificates do not match!", MB_ICONEXCLAMATION|MB_OK);
                
                ORIGIN_LOG_ERROR << "Certificate check failed!  Check Origin build.  Certificates do not match!";
            }

            return validated;
#endif
        }

        // GetCertificateContext
        //
        // Given an path and exe grab the certificate info
        void EscalationServiceWin::getCertificateContext(LPCWSTR szFileName,  PCCERT_CONTEXT &pCertContext)
        {

            HCERTSTORE hStore = NULL;
            HCRYPTMSG hMsg = NULL; 

            bool fResult;   
            DWORD dwEncoding, dwContentType, dwFormatType;
            PCMSG_SIGNER_INFO pSignerInfo = NULL;
            DWORD dwSignerInfo;    
            CERT_INFO CertInfo;

            // Get message handle and store handle from the signed file.
            fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                szFileName,
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                CERT_QUERY_FORMAT_FLAG_BINARY,
                0,
                &dwEncoding,
                &dwContentType,
                &dwFormatType,
                &hStore,
                &hMsg,
                NULL);
            if (!fResult)
                goto cert_cleanup;

            // Get signer information size.
            fResult = CryptMsgGetParam(hMsg, 
                CMSG_SIGNER_INFO_PARAM, 
                0, 
                NULL, 
                &dwSignerInfo);
            if (!fResult)
                goto cert_cleanup;

            // Allocate memory for signer information.
            pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
            if (!pSignerInfo)
                goto cert_cleanup;

            // Get Signer Information.
            fResult = CryptMsgGetParam(hMsg, 
                CMSG_SIGNER_INFO_PARAM, 
                0, 
                (PVOID)pSignerInfo, 
                &dwSignerInfo);
            if (!fResult)
                goto cert_cleanup;

            // Search for the signer certificate in the temporary certificate store.
            CertInfo.Issuer = pSignerInfo->Issuer;
            CertInfo.SerialNumber = pSignerInfo->SerialNumber;

            pCertContext = CertFindCertificateInStore(hStore,
                ENCODING,
                0,
                CERT_FIND_SUBJECT_CERT,
                (PVOID)&CertInfo,
                NULL);

cert_cleanup:
            //the calling function is responsible for cleaning up pCertContext

            //clean up
            if (pSignerInfo != NULL) 
                LocalFree(pSignerInfo);

            if (hStore != NULL) 
                CertCloseStore(hStore, 0);

            if (hMsg != NULL) 
                CryptMsgClose(hMsg);

        }
    } // namespace Escalation


} // namespace Origin
