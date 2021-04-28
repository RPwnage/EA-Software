#include "framework/blaze.h"
#include "arson/rpc/arsonslave/testexternaldatasourcemergelists_stub.h"
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"

#include "framework/controller/controller.h"
#include "gamemanager/externaldatamanager.h"
#include "gamemanager/gamemanagerslaveimpl.h"

namespace Blaze
{

namespace Arson
{

class TestExternalDataSourceMergeListsCommand : public TestExternalDataSourceMergeListsCommandStub
{
public:
    TestExternalDataSourceMergeListsCommand(Message* message, ExternalDataSourceMergeListsRequest *req, ArsonSlaveImpl* componentImp)
        : TestExternalDataSourceMergeListsCommandStub(message, req)
    {
    }

    ~TestExternalDataSourceMergeListsCommand() override { }

    template<typename T>
    bool validateList(const char8_t* attrName, const EA::TDF::TdfVectorBase& result, EA::TDF::TdfVectorBase& expected)
    {
        if (result.getValueType() != expected.getValueType())
        {
            ERR_LOG("[TestExternalDataSourceMergeListsCommand].validateList: Expected template attribute '" << attrName << "' to be a list of " << expected.getValueTypeDesc().getFullName() <<
                "; instead, got a list of " << result.getValueTypeDesc().getFullName());
            return false;
        }

        bool isMatch = true;
        size_t resultSize = result.vectorSize();
        size_t expectedSize = expected.vectorSize();
        if (resultSize != expectedSize)
            isMatch = false;

        StringBuilder resultStr("[");
        StringBuilder expectedStr("[");
        typedef typename EA::TDF::TdfPrimitiveVector<T> TdfPrimitiveVectorType;
        const TdfPrimitiveVectorType* resultList = (TdfPrimitiveVectorType*)(&result);
        TdfPrimitiveVectorType* expectedList = (TdfPrimitiveVectorType*)(&expected);
        typename TdfPrimitiveVectorType::const_iterator resItr = resultList->begin();
        typename TdfPrimitiveVectorType::iterator expItr = expectedList->begin();
        for ( ; resItr != resultList->end() && expItr != expectedList->end() ; )
        {
            if (isMatch)
            {
                expItr = expectedList->findAsSet(*resItr);
                if (expItr == expectedList->end())
                {
                    isMatch = false;
                    expItr = expectedList->begin();
                }
                else
                {
                    expectedStr << *expItr << ",";
                    expItr = expectedList->erase(expItr);
                }
            }

            if (resItr != resultList->end())
            {
                resultStr << *resItr << ",";
                ++resItr;
            }
            if (!isMatch && expItr != expectedList->end())
            {
                expectedStr << *expItr << ",";
                ++expItr;
            }
        }

        // Trim the trailing commas
        if (resultSize != 0)
            resultStr.trim(1);
        if (expectedSize != 0)
            expectedStr.trim(1);
        if (isMatch)
        {
            INFO_LOG("[TestExternalDataSourceMergeListsCommand].validateList: Parsed result for template attribute '" << attrName << "' matches expected result: " << resultStr.c_str() << "]");
        }
        else
        {
            ERR_LOG("[TestExternalDataSourceMergeListsCommand].validateList: Parsed result for template attribute '" << attrName << "': " << resultStr.c_str() << "] does not match expected result: " << expectedStr.c_str() << "]");
        }
        return isMatch;
    }

    TestExternalDataSourceMergeListsCommandStub::Errors execute() override
    {
        BlazeRpcError error;
        Blaze::GameManager::GameManagerSlaveImpl* gmComp = static_cast<Blaze::GameManager::GameManagerSlaveImpl*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, true, true, &error));
        if (gmComp == nullptr)
        {
            ERR_LOG("[TestExternalDataSourceMergeListsCommand].execute: internal unexpected error " << Blaze::ErrorHelp::getErrorName(error) << " obtaining GameManager component.");
            return ARSON_ERR_GAMEMANAGER_COMPONENT_NOT_FOUND;
        }

        // The ExternalDataManager will only permit the "leader" player to set template attributes
        Blaze::GameManager::PerPlayerJoinDataList dummyList;
        Blaze::GameManager::PerPlayerJoinData& leader = *dummyList.pull_back();
        leader.getUser().setBlazeId(gCurrentLocalUserSession->getBlazeId());
        Blaze::GameManager::TemplateAttributes templateAttributes;
        if (!gmComp->getExternalDataManager().fetchAndPopulateExternalData(mRequest.getExternalDataSourceApiNameList(), templateAttributes, dummyList))
        {
            WARN_LOG("[TestExternalDataSourceMergeListsCommand].execute: Failed to fetch and populate external data");
        }

        bool success = true;
        for (ExternalDataSourceMergeListsRequest::TemplateAttributeValueByNameMap::iterator itr = mRequest.getExpectedResults().begin(); itr != mRequest.getExpectedResults().end(); ++itr)
        {
            Blaze::GameManager::TemplateAttributes::const_iterator resItr = templateAttributes.find(itr->first);
            if (resItr == templateAttributes.end())
            {
                ERR_LOG("[TestExternalDataSourceMergeListsCommand].execute: Template attribute '" << itr->first.c_str() << "' missing from results.");
                success = false;
                continue;
            }

            // If there's an issue with the test parameters, just bail
            EA::TDF::TdfGenericReference expectedValue;
            if (!itr->second->getActiveMemberValue(expectedValue))
            {
                ERR_LOG("[TestExternalDataSourceMergeListsCommand].execute: Failed to get active member of TemplateAttributeValue union used to define expected result for template attribute '" << itr->first.c_str() << "'");
                return TestExternalDataSourceMergeListsCommandStub::ARSON_ERR_INVALID_PARAMETER;
            }

            // Normally, a call to fetchAndPopulateExternalData would eventually be followed by a call to GameManager::applyTemplateAttributes,
            // which would convert the template attributes fetched from the external data sources to the appropriate tdf types.
            // For this test, we do not define tdf types for any of our template attributes (since in order to do this, we'd have to introduce a new
            // matchmaking rule or CreateGameRequest field for a list of bools). This means that our template attributes are always returned as
            // TDF_ACTUAL_TYPE_UNKNOWN, and we have to set the type manually to avoid dereferencing a null pointer in cases where we expect the
            // external data sources to return an empty list.
            EA::TDF::GenericTdfType& result = *resItr->second;
            if (result.get().asAny() == nullptr)
                result.setType(expectedValue.getTypeDescription());

            switch (itr->second->getActiveMember())
            {
            case ExternalDataSourceMergeListsRequest::TemplateAttributeValue::MEMBER_BOOLLIST:
            {
                success = validateList<bool>(itr->first.c_str(), result.get().asList(), expectedValue.asList()) && success;
                break;
            }
            case ExternalDataSourceMergeListsRequest::TemplateAttributeValue::MEMBER_INTLIST:
            {
                success = validateList<int64_t>(itr->first.c_str(), result.get().asList(), expectedValue.asList()) && success;
                break;
            }
            case ExternalDataSourceMergeListsRequest::TemplateAttributeValue::MEMBER_STRINGLIST:
            {
                success = validateList<EA::TDF::TdfString>(itr->first.c_str(), result.get().asList(), expectedValue.asList()) && success;
                break;
            }
            default:
            {
                ERR_LOG("[TestExternalDataSourceMergeListsCommand].execute: Unimplemented TemplateAttributeValue union member '" << expectedValue.getFullName()
                    << "' used to define expected result for template attribute '" << itr->first.c_str() << "'");
                return TestExternalDataSourceMergeListsCommandStub::ARSON_ERR_INVALID_PARAMETER;
            }
            }
        }

        return success ? ERR_OK : TestExternalDataSourceMergeListsCommandStub::ARSON_ERR_JSON_DECODING_FAILED;
    }
};


DEFINE_TESTEXTERNALDATASOURCEMERGELISTS_CREATE()

} // namespace Arson
} // namespace Blaze

