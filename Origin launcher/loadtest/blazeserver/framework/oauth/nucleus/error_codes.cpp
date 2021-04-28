#include "framework/blaze.h"
#include "framework/oauth/nucleus/error_codes.h"
#include "nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
namespace Nucleus
{

/* Public methods *******************************************************************************/    
void ErrorCodes::add(int16_t httpErrorCode, NucleusCode::Code error, NucleusField::Code field, NucleusCause::Code cause, BlazeRpcError errorCode)
{
    ErrorsByErrorCode* errorsByErrorCode = find(errors, httpErrorCode);
    if (errorsByErrorCode == nullptr)
    {
        errorsByErrorCode = BLAZE_NEW ErrorsByErrorCode();
        errors->insert(ErrorMap::value_type(httpErrorCode,errorsByErrorCode));
    }

    ErrorsByField* errorsByField = find(errorsByErrorCode, error);
    if (errorsByField == nullptr)
    {
        errorsByField = BLAZE_NEW ErrorsByField();
        errorsByErrorCode->insert(ErrorsByErrorCode::value_type(error,errorsByField));
    }
    
    ErrorsByCause* errorsByCause = find(errorsByField, field);
    if (errorsByCause == nullptr)
    {
        errorsByCause = BLAZE_NEW ErrorsByCause();
        errorsByField->insert(ErrorsByField::value_type(field,errorsByCause));
    }

    errorsByCause->insert(ErrorsByCause::value_type(cause,errorCode));
}

void ErrorCodes::add(int16_t httpErrorCode, NucleusCode::Code error, NucleusField::Code field, BlazeRpcError errorCode)
{
    add(httpErrorCode, error, field, NucleusCause::UNKNOWN, errorCode);
}


BlazeRpcError ErrorCodes::getError(int16_t httpErrorCode, NucleusCode::Code error, NucleusField::Code field, NucleusCause::Code cause) const
{
    ErrorsByErrorCode* errorsByErrorCode = getErrorsByKey(errors, httpErrorCode, int16_t(-1));
    if (errorsByErrorCode == nullptr)
    {
        return mGenericError;
    }
    
    ErrorsByField* errorsByField = getErrorsByKey(errorsByErrorCode, error, NucleusCode::UNKNOWN);
    if (errorsByField == nullptr)
    {
        return mGenericError;
    }

    ErrorsByCause* errorsByCause = getErrorsByKey(errorsByField, field, NucleusField::UNKNOWN);
    if (errorsByCause == nullptr)
    {
        return mGenericError;
    }
    
    BlazeRpcError errorObj = getErrorsByKey(errorsByCause, cause, NucleusCause::UNKNOWN);
    
    return errorObj;
}

BlazeRpcError ErrorCodes::getError(NucleusCode::Code error, NucleusField::Code field, NucleusCause::Code cause) const
{
    return getError(int16_t(-1), error, field, cause);
}

BlazeRpcError ErrorCodes::getError(NucleusCode::Code error) const
{
    return getError(int16_t(-1), error, NucleusField::UNKNOWN, NucleusCause::UNKNOWN);
}

BlazeRpcError ErrorCodes::getError(const NucleusIdentity::ErrorDetails &error) const
{
    NucleusField::Code nucleusField = NucleusField::UNKNOWN;
    NucleusCause::Code nucleusCause = NucleusCause::UNKNOWN;
    NucleusCode::Code nucleusCode = NucleusCode::UNKNOWN;
    if (!error.getFailure().empty())
    {
        NucleusField::ParseCode(error.getFailure().front()->getField(), nucleusField);
        NucleusCause::ParseCode(error.getFailure().front()->getCause(), nucleusCause);
        NucleusCode::ParseCode(error.getCode(), nucleusCode);
    }

    return getError(int16_t(-1), nucleusCode, nucleusField, nucleusCause);
}

/* Private methods *******************************************************************************/    
ErrorsByErrorCode* ErrorCodes::find(ErrorMap* errorMap, int16_t key) const
{
    ErrorMap::iterator itr = errorMap->find(key);
    if (itr == errorMap->end())
    {
        return nullptr;
    }
    else
    {
        return itr->second;
    }
}

ErrorsByField* ErrorCodes::find(ErrorsByErrorCode* errorMap, NucleusCode::Code key) const
{
    ErrorsByErrorCode::iterator itr = errorMap->find(key);
    if (itr == errorMap->end())
    {
        return nullptr;
    }
    else
    {
        return itr->second;
    }
}

ErrorsByCause* ErrorCodes::find(ErrorsByField* errorMap, NucleusField::Code key) const
{
    ErrorsByField::iterator itr = errorMap->find(key);
    if (itr == errorMap->end())
    {
        return nullptr;
    }
    else
    {
        return itr->second;
    }
}

BlazeRpcError ErrorCodes::find(ErrorsByCause* errorMap, NucleusCause::Code key) const
{
    ErrorsByCause::iterator itr = errorMap->find(key);
    if (itr == errorMap->end())
    {
        return ERR_SYSTEM;
    }
    else
    {
        return itr->second;
    }
}

ErrorsByErrorCode* ErrorCodes::getErrorsByKey(ErrorMap* errorMap, int16_t key, int16_t defaultKey) const
{
    ErrorsByErrorCode* errs = find(errorMap, key);
    if (errs == nullptr)
    {
        errs = find(errorMap, defaultKey);
    }
    return errs;
}

ErrorsByField* ErrorCodes::getErrorsByKey(ErrorsByErrorCode* errorMap, NucleusCode::Code key, NucleusCode::Code defaultKey) const
{
    ErrorsByField* errs = find(errorMap, key);
    if (errs == nullptr)
    {
        errs = find(errorMap, defaultKey);
    }
    return errs;
}

ErrorsByCause* ErrorCodes::getErrorsByKey(ErrorsByField* errorMap, NucleusField::Code key, NucleusField::Code defaultKey) const
{
    ErrorsByCause* errs = find(errorMap, key);
    if (errs == nullptr)
    {
        errs = find(errorMap, defaultKey);
    }
    return errs;
}

BlazeRpcError ErrorCodes::getErrorsByKey(ErrorsByCause* errorMap, NucleusCause::Code key, NucleusCause::Code defaultKey) const
{
    BlazeRpcError err = find(errorMap, key);
    if (err == ERR_SYSTEM)
    {
        err = find(errorMap, defaultKey);
    }
    return err;
}

} // Authentication
} // Blaze
