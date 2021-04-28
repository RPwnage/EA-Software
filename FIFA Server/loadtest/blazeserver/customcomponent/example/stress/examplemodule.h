/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_EXAMPLEMODULE_H
#define BLAZE_STRESS_EXAMPLEMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "stressmodule.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "example/rpc/exampleslave.h"
#include "framework/rpc/usersessionsslave.h"


namespace Blaze
{
namespace Stress
{

class ExampleInstance;

class ExampleModule : public StressModule
{
    NON_COPYABLE(ExampleModule);
public:
    static StressModule* create() { return BLAZE_NEW ExampleModule(); }

    typedef BlazeRpcError (ExampleInstance::*ActionFunctor)();
    struct ActionDescriptor
    {
        ActionDescriptor(const char8_t* _name, ActionFunctor _action) : name(_name), action(_action), weightRangeMin(0), weightRangeMax(0) {}
        const char8_t* name;
        ActionFunctor action;
        int32_t weightRangeMin;
        int32_t weightRangeMax;
    };

    typedef eastl::vector<ActionDescriptor> ActionList;

public:
    ExampleModule();
    ~ExampleModule() override;

    const char8_t* getModuleName() override { return "example"; }
    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;

    StressInstance* createInstance(StressConnection* connection, Login* login) override;

    const ActionList& getActionList() const { return mActionList; }
    int32_t getTotalWeight() const { return mTotalWeight; }

private:
    ActionList mActionList;

    int32_t mTotalWeight;
};

class ExampleInstance : public StressInstance
{
    NON_COPYABLE(ExampleInstance);

public:
    ExampleInstance(ExampleModule& owner, StressConnection* connection, Login* login) :
        StressInstance(&owner, connection, login, Example::ExampleSlave::LOGGING_CATEGORY),
        mProxy(*getConnection())
    {
    }

    ~ExampleInstance() override
    {
    }

    ExampleModule& getowner() const { return *static_cast<ExampleModule*>(StressInstance::getOwner()); }

    BlazeRpcError poke();
    BlazeRpcError rpcPassthrough();
    BlazeRpcError requestToUserSessionOwner();

    const char8_t *getName() const override { return Fiber::getCurrentContext(); }

protected:
    BlazeRpcError execute() override;

private:
    Example::ExampleSlave mProxy;
};

} // Stress
} // Blaze

#endif //BLAZE_STRESS_EXAMPLEMODULE_H
