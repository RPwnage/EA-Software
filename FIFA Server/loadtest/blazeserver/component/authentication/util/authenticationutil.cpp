/*************************************************************************************************/
/*!
    \file authenticationutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/protocol/restprotocolutil.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/oauth/oauthutil.h"

#include "authentication/authenticationimpl.h"
#include "framework/oauth/nucleus/error_codes.h"
#include "authentication/util/authenticationutil.h"
#include "authentication/nucleusresult/nucleusresult.h"

namespace Blaze
{
namespace Authentication
{

/*************************************************************************************************/
/*!
    \brief nucleusResultToFieldErrorList

    Populates a fieldValidationErrorList from the result of a nucleus call.

    \param[in]  result - the result of the nucleus call
    \param[out]  errorList - the error list to populate
*/
/*************************************************************************************************/
void AuthenticationUtil::nucleusResultToFieldErrorList(const NucleusResult* result, FieldValidateErrorList& errorList)
{
    const NucleusResult::FieldErrorList fieldErrorList = result->getFieldErrors();

    if (result->getCode() == Blaze::Nucleus::NucleusCode::VALIDATION_FAILED)
    {
        for(uint32_t i=0; i < fieldErrorList.size(); i++)
        {
            FieldValidationError* validationError = BLAZE_NEW FieldValidationError();
            validationError->setField(fieldErrorList[i].mField);
            validationError->setError(fieldErrorList[i].mCause);
            errorList.getList().push_back(validationError);

            SPAM_LOG("[AuthenticationUtil].nucleusResultToFieldErrorList(): Field " << Blaze::Nucleus::NucleusField::CodeToString(validationError->getField())
                << " failed with cause " << Blaze::Nucleus::NucleusCause::CodeToString(validationError->getError()) << ".");
        }
    }
}

/*************************************************************************************************/
/*!
    \brief parseIdentityError

    Parse IdentityError error and associated rpc error to return a more specific/suitable AUTH error

    \param[in]  error - the IdentityError error
    \param[in]  rpcError - the rpc error got from calling NucleusIdentity rpc
    \return     BlazeRpcError - AUTH error
*/
/*************************************************************************************************/
BlazeRpcError AuthenticationUtil::parseIdentityError(const NucleusIdentity::IdentityError& error, const BlazeRpcError rpcError)
{
    BlazeRpcError oAuthErr = OAuth::OAuthUtil::parseNucleusIdentityError(error, rpcError);
    BlazeRpcError authErr = authFromOAuthError(oAuthErr);
    return authErr;
}

/*************************************************************************************************/
/*!
    \brief formatExternalRef

    On the Wii platform, pad the extRef with zeros (0) to a string length of 16
*/
/*************************************************************************************************/
char8_t *AuthenticationUtil::formatExternalRef(uint64_t id, char8_t *newExtRef, size_t newExtRefBufSize)
{
    if (id != 0)
    {
        blaze_snzprintf(newExtRef, newExtRefBufSize, "%" PRIu64, id);
    }
    else
    {
        blaze_strnzcpy(newExtRef, "", newExtRefBufSize);
    }

    return newExtRef;
}

void AuthenticationUtil::parseEntitlementForResponse(PersonaId personaId, const NucleusIdentity::EntitlementInfo &info, Entitlement &responseEntitlement)
{
    responseEntitlement.setPersonaId(personaId);
    responseEntitlement.setId(info.getEntitlementId());
    responseEntitlement.setDeviceUri(info.getDeviceUri());
    responseEntitlement.setEntitlementTag(info.getEntitlementTag());

   Blaze::Nucleus::EntitlementType::Code entType = Blaze::Nucleus::EntitlementType::UNKNOWN;
    if (info.getEntitlementType()[0] != '\0' && !Blaze::Nucleus::EntitlementType::ParseCode(info.getEntitlementType(), entType))
    {
        WARN_LOG("[AuthenticationUtil].parseEntitlementForResponse: Received unexpected enum EntitlementType value " << info.getEntitlementType() << " from Nucleus");
    }
    responseEntitlement.setEntitlementType(entType);

    responseEntitlement.setGrantDate(info.getGrantDate());
    responseEntitlement.setGroupName(info.getGroupName());
    responseEntitlement.setIsConsumable(info.getIsConsumable());

    Blaze::Nucleus::ProductCatalog::Code catalog = Blaze::Nucleus::ProductCatalog::UNKNOWN;
    if(info.getProductCatalog()[0] != '\0' && !Blaze::Nucleus::ProductCatalog::ParseCode(info.getProductCatalog(), catalog))
    {
        WARN_LOG("[AuthenticationUtil].parseEntitlementForResponse: Received unexpected enum ProductCatalog value " << info.getProductCatalog() << " from Nucleus");
    }
    responseEntitlement.setProductCatalog(catalog);

    responseEntitlement.setProductId(info.getProductId());
    responseEntitlement.setProjectId(info.getProjectId());

    Blaze::Nucleus::StatusReason::Code reason = Blaze::Nucleus::StatusReason::UNKNOWN;
    if (info.getStatusReasonCode()[0] != '\0' && !Blaze::Nucleus::StatusReason::ParseCode(info.getStatusReasonCode(), reason))
    {
        WARN_LOG("[AuthenticationUtil].parseEntitlementForResponse: Received unexpected enum StatusReason value " << info.getStatusReasonCode() << " from Nucleus");
    }
    responseEntitlement.setStatusReasonCode(reason);

    Blaze::Nucleus::EntitlementStatus::Code status = Blaze::Nucleus::EntitlementStatus::UNKNOWN;
    if (info.getStatus()[0] != '\0' && !Blaze::Nucleus::EntitlementStatus::ParseCode(info.getStatus(), status))
    {
        WARN_LOG("[AuthenticationUtil].parseEntitlementForResponse: Received unexpected enum EntitlementStatus value " << info.getStatus() << " from Nucleus");
    }
    responseEntitlement.setStatus(status);

    responseEntitlement.setTerminationDate(info.getTerminationDate());
    responseEntitlement.setUseCount(static_cast<uint32_t>(info.getUseCount()));
    responseEntitlement.setVersion(info.getVersion());
}

InetAddress AuthenticationUtil::getRealEndUserAddr(const Command* callingCommand)
{
    if (callingCommand != nullptr && callingCommand->getPeerInfo() != nullptr)
    {
        return callingCommand->getPeerInfo()->getRealPeerAddress();
    }
    else if (gCurrentLocalUserSession != nullptr && gCurrentLocalUserSession->getPeerInfo() != nullptr)
    {
        return gCurrentLocalUserSession->getPeerInfo()->getRealPeerAddress();
    }
    return InetAddress();
}

ClientType AuthenticationUtil::getClientType(const PeerInfo* peerInfo)
{
    return (peerInfo != nullptr) ? peerInfo->getClientType() : gCurrentUserSession->getClientType();
}

BlazeRpcError AuthenticationUtil::authFromOAuthError(BlazeRpcError oAuthErr)
{
    if (oAuthErr == ERR_OK)
        return ERR_OK;

    if (BLAZE_ERROR_IS_SYSTEM_ERROR(oAuthErr))
        return oAuthErr;

    return BLAZE_COMPONENT_ERROR(Blaze::Authentication::AuthenticationSlaveImpl::COMPONENT_ID, BLAZE_CODE_FROM_ERROR(oAuthErr));
}

}//Authentication
}//Blaze
