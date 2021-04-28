/**************************************************************************************************/
/*! 
    \file nucleusstringtocode.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/
#include "framework/blaze.h"

#include "framework/oauth/nucleus/nucleusstringtocode.h"

namespace Blaze
{

namespace Nucleus
{

namespace AccountStatus
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "ACTIVE") == 0) return AccountStatus::ACTIVE;
    if(blaze_strcmp(str, "BANNED") == 0) return AccountStatus::BANNED;
    if(blaze_strcmp(str, "CHILD_APPROVED") == 0) return AccountStatus::CHILD_APPROVED;
    if(blaze_strcmp(str, "CHILD_PENDING") == 0) return AccountStatus::CHILD_PENDING;
    if(blaze_strcmp(str, "DEACTIVATED") == 0) return AccountStatus::DEACTIVATED;
    if(blaze_strcmp(str, "DELETED") == 0) return AccountStatus::DELETED;
    if(blaze_strcmp(str, "DISABLED") == 0) return AccountStatus::DISABLED;
    if(blaze_strcmp(str, "PENDING") == 0) return AccountStatus::PENDING;
    if(blaze_strcmp(str, "TENTATIVE") == 0) return AccountStatus::TENTATIVE;
    return AccountStatus::UNKNOWN;
}
}

namespace EmailStatus
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "BAD") == 0) return EmailStatus::BAD;
    if(blaze_strcmp(str, "VERIFIED") == 0) return EmailStatus::VERIFIED;
    if(blaze_strcmp(str, "GUEST") == 0) return EmailStatus::GUEST;
    if(blaze_strcmp(str, "ANONYMOUS") == 0) return EmailStatus::ANONYMOUS;
    return EmailStatus::UNKNOWN;
}
}

namespace PersonaStatus
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "PENDING") == 0) return PersonaStatus::PENDING;
    if(blaze_strcmp(str, "ACTIVE") == 0) return PersonaStatus::ACTIVE;
    if(blaze_strcmp(str, "DEACTIVATED") == 0) return PersonaStatus::DEACTIVATED;
    if(blaze_strcmp(str, "DISABLED") == 0) return PersonaStatus::DISABLED;
    if(blaze_strcmp(str, "DELETED") == 0) return PersonaStatus::DELETED;
    if(blaze_strcmp(str, "BANNED") == 0) return PersonaStatus::BANNED;
    return PersonaStatus::UNKNOWN;
}
}

namespace EntitlementStatus
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "ACTIVE") == 0) return EntitlementStatus::ACTIVE;
    if(blaze_strcmp(str, "DISABLED") == 0) return EntitlementStatus::DISABLED;
    if(blaze_strcmp(str, "PENDING") == 0) return EntitlementStatus::PENDING;
    if(blaze_strcmp(str, "DELETED") == 0) return EntitlementStatus::DELETED;
    if(blaze_strcmp(str, "BANNED") == 0) return EntitlementStatus::BANNED;
    return EntitlementStatus::UNKNOWN;
}
}

namespace EntitlementType
{
Code stringToCode(const char8_t* str)
{
    if (blaze_strcmp(str, "DEFAULT") == 0) return EntitlementType::DEFAULT;
    if (blaze_strcmp(str, "ONLINE_ACCESS") == 0) return EntitlementType::ONLINE_ACCESS;
    if (blaze_strcmp(str, "PARENTAL_APPROVAL") == 0) return EntitlementType::PARENTAL_APPROVAL;
    if (blaze_strcmp(str, "SUBSCRIPTIONS") == 0) return EntitlementType::SUBSCRIPTIONS;
    if (blaze_strcmp(str, "TRIAL_ONLINE_ACCESS") == 0) return EntitlementType::TRIAL_ONLINE_ACCESS;
    if (blaze_strcmp(str, "ORIGIN_DOWNLOAD") == 0) return EntitlementType::ORIGIN_DOWNLOAD;
    return EntitlementType::UNKNOWN;
}
}

namespace ProductCatalog
{
Code stringToCode(const char8_t* str)
{
    if (blaze_strcmp(str, "OFB") == 0) return ProductCatalog::OFB;
    if (blaze_strcmp(str, "SKUD") == 0) return ProductCatalog::SKUD;
    return ProductCatalog::UNKNOWN;
}
}

namespace StatusReason
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "NONE") == 0) return StatusReason::NONE;
    if(blaze_strcmp(str, "REACTIVATED_CUSTOMER") == 0) return StatusReason::REACTIVATED_CUSTOMER;
    if(blaze_strcmp(str, "INVALID_EMAIL") == 0) return StatusReason::INVALID_EMAIL;
    if(blaze_strcmp(str, "PRIVACY_POLICY") == 0) return StatusReason::PRIVACY_POLICY;
    if(blaze_strcmp(str, "PARENTS_REQUEST") == 0) return StatusReason::PARENTS_REQUEST;
    if(blaze_strcmp(str, "PARENTAL_REQUEST") == 0) return StatusReason::PARENTAL_REQUEST;
    if(blaze_strcmp(str, "SUSPENDED_MISCONDUCT_GENERAL") == 0) return StatusReason::SUSPENDED_MISCONDUCT_GENERAL;
    if(blaze_strcmp(str, "SUSPENDED_MISCONDUCT_HARASSMENT") == 0) return StatusReason::SUSPENDED_MISCONDUCT_HARASSMENT;
    if(blaze_strcmp(str, "SUSPENDED_MISCONDUCT_MACROING") == 0) return StatusReason::SUSPENDED_MISCONDUCT_MACROING;
    if(blaze_strcmp(str, "SUSPENDED_MISCONDUCT_EXPLOITATION") == 0) return StatusReason::SUSPENDED_MISCONDUCT_EXPLOITATION;
    if(blaze_strcmp(str, "CUSTOMER_OPT_OUT") == 0) return StatusReason::CUSTOMER_OPT_OUT;
    if(blaze_strcmp(str, "CUSTOMER_UNDER_AGE") == 0) return StatusReason::CUSTOMER_UNDER_AGE;
    if(blaze_strcmp(str, "EMAIL_CONFIRMATION_REQUIRED") == 0) return StatusReason::EMAIL_CONFIRMATION_REQUIRED;
    if(blaze_strcmp(str, "MISTYPED_ID") == 0) return StatusReason::MISTYPED_ID;
    if(blaze_strcmp(str, "ABUSED_ID") == 0) return StatusReason::ABUSED_ID;
    if(blaze_strcmp(str, "DEACTIVATED_EMAIL_LINK") == 0) return StatusReason::DEACTIVATED_EMAIL_LINK;
    if(blaze_strcmp(str, "DEACTIVATED_CS") == 0) return StatusReason::DEACTIVATED_CS;
    if(blaze_strcmp(str, "CLAIMED_BY_TRUE_OWNER") == 0) return StatusReason::CLAIMED_BY_TRUE_OWNER;
    if(blaze_strcmp(str, "BANNED") == 0) return StatusReason::BANNED;
    if(blaze_strcmp(str, "AFFILIATE_ACCOUNT_DELETED") == 0) return StatusReason::AFFILIATE_ACCOUNT_DELETED;
    if(blaze_strcmp(str, "BILLING") == 0) return StatusReason::BILLING;
    if(blaze_strcmp(str, "COMPENSATION") == 0) return StatusReason::COMPENSATION;
    if(blaze_strcmp(str, "DEACTIVATED_AFFILIATE") == 0) return StatusReason::DEACTIVATED_AFFILIATE;
    if(blaze_strcmp(str, "FRAUD") == 0) return StatusReason::FRAUD;
    if(blaze_strcmp(str, "FRAUD_REVIEW") == 0) return StatusReason::FRAUD_REVIEW;
    if(blaze_strcmp(str, "REGISTERED") == 0) return StatusReason::IS_REGISTERED;
    if(blaze_strcmp(str, "MISCONDUCT") == 0) return StatusReason::MISCONDUCT;
    if(blaze_strcmp(str, "OPT_OUT") == 0) return StatusReason::OPT_OUT;
    if(blaze_strcmp(str, "PROMOTIONAL") == 0) return StatusReason::PROMOTIONAL;
    if(blaze_strcmp(str, "REPLACEMENT") == 0) return StatusReason::REPLACEMENT;
    if(blaze_strcmp(str, "SUBSCRIPTION_CREATED") == 0) return StatusReason::SUBSCRIPTION_CREATED;
    if(blaze_strcmp(str, "SUBSCRIPTION_EXPIRED") == 0) return StatusReason::SUBSCRIPTION_EXPIRED;
    if(blaze_strcmp(str, "SUNSET") == 0) return StatusReason::SUNSET;
    if(blaze_strcmp(str, "TECHNICAL") == 0) return StatusReason::TECHNICAL;
    if(blaze_strcmp(str, "UNREGISTERED") == 0) return StatusReason::UNREGISTERED;
    if(blaze_strcmp(str, "USER_DELETED") == 0) return StatusReason::USER_DELETED;
    return StatusReason::UNKNOWN;
}
}

namespace NucleusCode
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "VALIDATION_FAILED") == 0) return NucleusCode::VALIDATION_FAILED;
    if(blaze_strcmp(str, "NO_SUCH_USER") == 0) return NucleusCode::NO_SUCH_USER;
    if(blaze_strcmp(str, "NO_SUCH_COUNTRY") == 0) return NucleusCode::NO_SUCH_COUNTRY;
    if(blaze_strcmp(str, "NO_SUCH_PERSONA") == 0) return NucleusCode::NO_SUCH_PERSONA;
    if(blaze_strcmp(str, "NO_SUCH_EXTERNAL_REFERENCE") == 0) return NucleusCode::NO_SUCH_EXTERNAL_REFERENCE;
    if(blaze_strcmp(str, "NO_SUCH_NAMESPACE") == 0) return NucleusCode::NO_SUCH_NAMESPACE;
    if(blaze_strcmp(str, "INVALID_PASSWORD") == 0) return NucleusCode::INVALID_PASSWORD;
    if(blaze_strcmp(str, "CODE_ALREADY_USED") == 0) return NucleusCode::CODE_ALREADY_USED;
    if(blaze_strcmp(str, "INVALID_CODE") == 0) return NucleusCode::INVALID_CODE;
    if(blaze_strcmp(str, "PARSE_EXCEPTION") == 0) return NucleusCode::PARSE_EXCEPTION;
    if(blaze_strcmp(str, "NO_SUCH_GROUP") == 0) return NucleusCode::NO_SUCH_GROUP;
    if(blaze_strcmp(str, "NO_SUCH_GROUP_NAME") == 0) return NucleusCode::NO_SUCH_GROUP_NAME;
    if(blaze_strcmp(str, "NO_ASSOCIATED_PRODUCT") == 0) return NucleusCode::NO_ASSOCIATED_PRODUCT;
    if(blaze_strcmp(str, "CODE_ALREADY_DISABLED") == 0) return NucleusCode::CODE_ALREADY_DISABLED;
    if(blaze_strcmp(str, "NO_SUCH_OPTIN") == 0) return NucleusCode::NO_SUCH_OPTIN;
    if(blaze_strcmp(str, "NO_SUCH_PERSONA_REFERENCE") == 0) return NucleusCode::NO_SUCH_PERSONA_REFERENCE;
    if(blaze_strcmp(str, "NO_SUCH_AUTH_DATA") == 0) return NucleusCode::NO_SUCH_AUTH_DATA;
    return NucleusCode::UNKNOWN;
}
}

namespace NucleusField
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "password") == 0) return NucleusField::PASSWORD;
    if(blaze_strcmp(str, "email") == 0) return NucleusField::EMAIL;
    if(blaze_strcmp(str, "parentalEmail") == 0) return NucleusField::PARENTAL_EMAIL;
    if(blaze_strcmp(str, "displayName") == 0) return NucleusField::DISPLAY_NAME;
    if(blaze_strcmp(str, "status") == 0) return NucleusField::STATUS;
    if(blaze_strcmp(str, "dob") == 0) return NucleusField::DOB;
    if(blaze_strcmp(str, "token") == 0) return NucleusField::TOKEN;
    if(blaze_strcmp(str, "expiration") == 0) return NucleusField::EXPIRATION;
    if(blaze_strcmp(str, "optInType") == 0) return NucleusField::OPTINTYPE;
    if(blaze_strcmp(str, "application") == 0) return NucleusField::APPLICATION;
    if(blaze_strcmp(str, "source") == 0) return NucleusField::SOURCE;
    if(blaze_strcmp(str, "baseName") == 0) return NucleusField::BASENAME;
    if(blaze_strcmp(str, "total") == 0) return NucleusField::NUMBERSUGGESTIONS;
    return NucleusField::UNKNOWN;
}
}

namespace NucleusCause
{
Code stringToCode(const char8_t* str)
{
    if(blaze_strcmp(str, "INVALID_VALUE") == 0) return NucleusCause::INVALID_VALUE;
    if(blaze_strcmp(str, "ILLEGAL_VALUE") == 0) return NucleusCause::ILLEGAL_VALUE;
    if(blaze_strcmp(str, "MISSING_VALUE") == 0) return NucleusCause::MISSING_VALUE;
    if(blaze_strcmp(str, "DUPLICATE_VALUE") == 0) return NucleusCause::DUPLICATE_VALUE;
    if(blaze_strcmp(str, "INVALID_EMAIL_DOMAIN") == 0) return NucleusCause::INVALID_EMAIL_DOMAIN;
    if(blaze_strcmp(str, "SPACES_NOT_ALLOWED") == 0) return NucleusCause::SPACES_NOT_ALLOWED;
    if(blaze_strcmp(str, "TOO_SHORT") == 0) return NucleusCause::TOO_SHORT;
    if(blaze_strcmp(str, "TOO_LONG") == 0) return NucleusCause::TOO_LONG;
    if(blaze_strcmp(str, "TOO_YOUNG") == 0) return NucleusCause::TOO_YOUNG;
    if(blaze_strcmp(str, "TOO_OLD") == 0) return NucleusCause::TOO_OLD;
    if(blaze_strcmp(str, "ILLEGAL_FOR_COUNTRY") == 0) return NucleusCause::ILLEGAL_FOR_COUNTRY;
    if(blaze_strcmp(str, "BANNED_COUNTRY") == 0) return NucleusCause::BANNED_COUNTRY;
    if(blaze_strcmp(str, "TOKEN_EXPIRED") == 0) return NucleusCause::TOKEN_EXPIRED;
    if(blaze_strcmp(str, "NOT_ALLOWED") == 0) return NucleusCause::NOT_ALLOWED;
    if(blaze_strcmp(str, "TOO_MANY") == 0) return NucleusCause::TOO_MANY;
    if(blaze_strcmp(str, "NO_SUCH_PID") == 0) return NucleusCause::NO_SUCH_PID;
    if(blaze_strcmp(str, "PID_DOES_NOT_MATCH") == 0) return NucleusCause::PID_DOES_NOT_MATCH;
    if(blaze_strcmp(str, "NO_SUCH_PERSONA") == 0) return NucleusCause::NO_SUCH_PERSONA;
    if(blaze_strcmp(str, "NO_SUCH_USER") == 0) return NucleusCause::NO_SUCH_USER;
    if(blaze_strcmp(str, "NO_SUCH_GROUP") == 0) return NucleusCause::NO_SUCH_GROUP;
    if(blaze_strcmp(str, "NO_SUCH_DEVICE") == 0) return NucleusCause::NO_SUCH_DEVICE;
    if(blaze_strcmp(str, "NO_SUCH_ENTITLEMENT") == 0) return NucleusCause::NO_SUCH_ENTITLEMENT;
    if(blaze_strcmp(str, "INVALID_TYPE_FOR_CATEGORY") == 0) return NucleusCause::INVALID_TYPE_FOR_CATEGORY;
    if(blaze_strcmp(str, "LINK_ALREADY_EXISTS") == 0) return NucleusCause::LINK_ALREADY_EXISTS;
    if(blaze_strcmp(str, "INVALID_DATE_RANGE") == 0) return NucleusCause::INVALID_DATE_RANGE;
    if(blaze_strcmp(str, "DATE_STATUS_MISMATCH") == 0) return NucleusCause::DATE_STATUS_MISMATCH;
    if(blaze_strcmp(str, "TERMINATION_DATE_BEFORE_GRANT_DATE") == 0) return NucleusCause::TERMINATION_DATE_BEFORE_GRANT_DATE;
    if(blaze_strcmp(str, "TERMINATION_DATE_RANGE_INVALID") == 0) return NucleusCause::TERMINATION_DATE_RANGE_INVALID;
    if(blaze_strcmp(str, "GRANT_DATE_RANGE_INVALID") == 0) return NucleusCause::GRANT_DATE_RANGE_INVALID;
    if(blaze_strcmp(str, "RESULT_SET_TOO_LARGE") == 0) return NucleusCause::RESULT_SET_TOO_LARGE;
    if(blaze_strcmp(str, "NO_SUCH_OPTIN") == 0) return NucleusCause::NO_SUCH_OPTIN;
    if(blaze_strcmp(str, "USER_DOES_NOT_MATCH") == 0) return NucleusCause::USER_DOES_NOT_MATCH;
    if(blaze_strcmp(str, "STALE_VERSION") == 0) return NucleusCause::STALE_VERSION;
    if(blaze_strcmp(str, "INVALID_CLIENT_NAMESPACE") == 0) return NucleusCause::INVALID_CLIENT_NAMESPACE;
    if(blaze_strcmp(str, "INVALID_CLIENT_GROUP") == 0) return NucleusCause::INVALID_CLIENT_GROUP;
    return NucleusCause::UNKNOWN;
}
}

} // Nucleus
} // Blaze
