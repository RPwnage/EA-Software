/*************************************************************************************************/
/*!
    \file   gdprcomplianceslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GDPR_COMPLIANCE_SLAVEIMPL_H
#define BLAZE_GDPR_COMPLIANCE_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "gdprcompliance/rpc/gdprcomplianceslave_stub.h"
#include "framework/replication/replicationcallback.h"

#include "framework/controller/controller.h"
#include "framework/database/dbrow.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"

// gdprcompliance includes
#include "gdprcompliance/tdf/gdprcompliancetypes.h"
#include "gdprcompliance/tdf/gdprcompliancetypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace GdprCompliance
{

class GdprComplianceSlaveImpl : public GdprComplianceSlaveStub
{
    NON_COPYABLE(GdprComplianceSlaveImpl);
public:
    GdprComplianceSlaveImpl();
    ~GdprComplianceSlaveImpl() override;

    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }
    BlazeRpcError createGDPRTable();

private:
    bool onActivate() override;
    bool onValidateConfig(GdprComplianceConfig& config, const GdprComplianceConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onConfigure() override;
    bool onResolve() override;

    uint64_t insertGDPRRecord(int64_t accountId, const char8_t* status, const char8_t* requestType, const char8_t* requester);
    BlazeRpcError updateGDPRRequestStatus(uint64_t recordId, BlazeRpcError err);

    BlazeRpcError pullUserDataFromDBsByBlazeId(BlazeId blazeId, UserData* userData);  
    BlazeRpcError pullUserDataFromDBsByAccountId(AccountId accountId, UserData* userData);

    bool isUserDeleteAlreadyInProgress(int64_t accountId);

    bool checkIfNucleusDeleteUser(AccountId id);
    void schedulePeriodicDeleteProcess();
    void doUserDelete();

    typedef eastl::hash_map<uint32_t, DataPullDbTableList> DataPullDbTablesByDbIdMap;
    bool validateDataPullDbTables(uint32_t dbId, DataPullDbTableList& list, eastl::string& failingTable);

    struct UserDataResult
    {
        ClientPlatformType platform;
        eastl::string persona;
    };
    typedef eastl::hash_map<BlazeId, UserDataResult> UserDataResultMap;
    BlazeRpcError getUserDataResults(UserDataResultMap& resultMap, const BlazeIdVector* blazeIds, AccountId accountId = INVALID_ACCOUNT_ID);

    BlazeRpcError customComponentGetUserData(const UserDataResultMap &userMap, AccountId accountId, GetUserDataResponse &response);
    BlazeRpcError customComponentDeactivateUserData(const UserDataResultMap &userMap, AccountId accountId);

#ifdef TARGET_associationlists
    BlazeRpcError pullFromAssociationlist(BlazeId blazeId, UserData* userData);
    BlazeRpcError deleteUserAssociationList(BlazeId id, ClientPlatformType platform);
#endif
    BlazeRpcError deactivateUserInfoTable(BlazeId id, ClientPlatformType platform);
    BlazeRpcError deactivateAccountInfoTable(AccountId id);
    BlazeRpcError pullFromUserInfo(BlazeId blazeId, UserData* userData);

    CheckRecordProgressError::Error processCheckRecordProgress(const CheckRecordProgressRequest &request, CheckRecordProgressResponse &response, const Message* message) override;
    DeactivateUserDataError::Error processDeactivateUserData(const DeactivateUserDataRequest &request, DeactivateUserDataResponse &response, const Message* message) override;
    GetUserDataError::Error processGetUserData(const GetUserDataRequest &request, GetUserDataResponse &response, const Message* message) override;

    struct DataPullQuery
    {
        eastl::string tableDesc;
        eastl::vector<eastl::string> columnDescList; // used as the 1st row per table
        eastl::string queryStr;
    };
    typedef eastl::hash_map<uint32_t, eastl::vector<DataPullQuery>> DataPullQueriesByDBIdMap;
    BlazeRpcError runDataPullQueries(int64_t userId, const DataPullQueriesByDBIdMap& dataPullMap, UserData* userData);

#ifdef TARGET_gamemanager
    void expireGameManagerData();
#endif

    TimerId mPeriodicDeleteProcessTimerId;
};

} // GdprCompliance
} // Blaze

#endif // BLAZE_GDPR_COMPLIANCE_SLAVEIMPL_H

