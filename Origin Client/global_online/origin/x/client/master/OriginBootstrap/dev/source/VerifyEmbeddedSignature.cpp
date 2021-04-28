//-------------------------------------------------------------------
// Copyright (C) Microsoft.  All rights reserved.
// Example of verifying the embedded signature of a PE file by using 
// the WinVerifyTrust function.

#define _UNICODE 1
#define UNICODE 1

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include "LogService_win.h"
#include "VerifyEmbeddedSignature.h"

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)
#define ORIGIN_CODESIGNING_SUBJECT_NAME L"Electronic Arts, Inc."

#pragma comment(lib, "wintrust.lib")	//for exe signing verification
#pragma comment(lib, "crypt32.lib")		//for cert info verification


bool VerifyEmbeddedSignatureOfFile(LPCWSTR pwszSourceFile)
{

    LONG lStatus;
    DWORD dwLastError;

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

    bool ok = false;

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
            ORIGINBS_LOG_EVENT << "The file is signed.";
            ok = TRUE;
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
                ORIGINBS_LOG_ERROR << "The file is not signed. - " << pwszSourceFile;
            } 
            else 
            {
                ORIGINBS_LOG_ERROR << "Could not verify file signature. - " << pwszSourceFile;
            }
            break;

        case TRUST_E_EXPLICIT_DISTRUST:
            // The hash that represents the subject or the publisher 
            // is not allowed by the admin or user.
            ORIGINBS_LOG_ERROR << "The executable signature for is disallowed. - " << pwszSourceFile;
            break;

        case TRUST_E_SUBJECT_NOT_TRUSTED:
            // The user clicked "No" when asked to install and run.
            ORIGINBS_LOG_ERROR << "User declined binary signature exception - " << pwszSourceFile;
            break;

        case CRYPT_E_SECURITY_SETTINGS:
        default:
            // The UI was disabled in dwUIChoice or the admin policy 
            // has disabled user trust. lStatus contains the 
            // publisher or time stamp chain error.
            ORIGINBS_LOG_ERROR << "File signature verification error. - " << pwszSourceFile;
            break;
    }
    return ok;
}

// GetCertificateContext
//
// Given an path and exe grab the certificate info
void getCertificateContext(LPCWSTR szFileName,  PCCERT_CONTEXT &pCertContext)
{
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL; 

    BOOL fResult;   
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
    if (fResult == FALSE)
        goto cert_cleanup;

    // Get signer information size.
    fResult = CryptMsgGetParam(hMsg, 
        CMSG_SIGNER_INFO_PARAM, 
        0, 
        NULL, 
        &dwSignerInfo);
    if (fResult == FALSE)
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
    if (fResult == FALSE)
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

// isEACertificate
// 
// Compares specified binary certificate to see if it is an EA certificate
bool isValidEACertificate(LPCWSTR szBinaryFileName)
{
    // To begin with, the embedded signature must be valid
    if (!VerifyEmbeddedSignatureOfFile(szBinaryFileName))
    {
        return false;
    }

    // Next, get the certificate context and see if it is actually an EA certificate
    bool validated = false;
    
    PCCERT_CONTEXT binaryCert = NULL;

    // Get the certificate context for specified binary
    // This will allocate memory for the returned context so we must free it below
    getCertificateContext(szBinaryFileName, binaryCert);

    if (!binaryCert)
    {
        ORIGINBS_LOG_ERROR << "Unable to read certificate for file: " << szBinaryFileName;
        return false;
    }

    wchar_t szSubjectNameString[256] = {0};
    if(CertGetNameString(binaryCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szSubjectNameString, sizeof(szSubjectNameString)-1))
    {
        // Make sure it is an EA certificate
        if (_wcsicmp(szSubjectNameString, ORIGIN_CODESIGNING_SUBJECT_NAME) == 0)
        {
            validated = true;
        }
    }
    else
    {
        ORIGINBS_LOG_ERROR << "Unable to read certificate subject name from file: " << szBinaryFileName;
    }

    if (validated)
    {
        ORIGINBS_LOG_EVENT << "The file has a valid EA certificate: " << szBinaryFileName;
    }
    else
    {
        ORIGINBS_LOG_ERROR << "Certificate was valid but incorrect subject name (i.e. wrong certificate) for file: " << szBinaryFileName;
    }

    // Free the context
    CertFreeCertificateContext(binaryCert);

    return validated;
}