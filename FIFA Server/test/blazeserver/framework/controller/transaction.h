/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_TRANSACTION_H
#define BLAZE_TRANSACTION_H

#include "EASTL/string.h"
#include "framework/system/frameworkresource.h"
#include "EATDF/time.h"
#include "framework/tdf/transaction_server.h"

#include "framework/blazedefines.h"

namespace Blaze
{

class TransactionContext : public FrameworkResource 
{
    NON_COPYABLE(TransactionContext);

public:
    const char* getTransactionDescription() const { return mDescription.c_str(); }
    EA::TDF::TimeValue getCreationTimestamp() const { return mCreationTimestamp; } 
    EA::TDF::TimeValue getTimeout() const { return mTimeout; }
    inline TransactionContextId getTransactionContextId() const { return mTransactionContextId; }

    const char8_t *getTypeName() const override;
    inline bool isActive() const { return mIsActive; }

private:
    friend class Component;   

    void setTimeout(EA::TDF::TimeValue timeout) { mTimeout = timeout; }
    void setTransactionContextId(TransactionContextId id) { mTransactionContextId = id; }

protected:

    TransactionContext(const char* description);
    ~TransactionContext() override { };

    void assignInternal(void) override { }
    void releaseInternal(void) override;

    // transaction methods that need to be implemented
    virtual BlazeRpcError DEFINE_ASYNC_RET(commit()) = 0;   
    virtual BlazeRpcError DEFINE_ASYNC_RET(rollback()) = 0; 

private:
    eastl::string mDescription;   
    TransactionContextId mTransactionContextId;
    EA::TDF::TimeValue mCreationTimestamp;
    EA::TDF::TimeValue mTimeout;
    bool mIsActive;
    
    // the following are invoked only by Component class   
    BlazeRpcError DEFINE_ASYNC_RET(commitInternal());   
    BlazeRpcError DEFINE_ASYNC_RET(rollbackInternal()); 
};

typedef FrameworkResourcePtr<TransactionContext> TransactionContextPtr;


// Transaction client uses this class to reference transaction
class TransactionContextHandle : public FrameworkResource 
{ 
    NON_COPYABLE(TransactionContextHandle);
public:
    operator TransactionContextId() { return mId; }       
    const char8_t* getTypeName() const override { return "TransactionContextHandle"; }

    // dissociate handle from transaction's state
    void dissociate() { mAssociated = false; } 

private:   
    friend class Component;

    TransactionContextHandle(Component &component, TransactionContextId id) 
        : mId (id), mComponent(component), mAssociated(true) { }

    void assignInternal() override { }
    void releaseInternal() override;

private:    
    TransactionContextId mId;   
    Component &mComponent;
    bool mAssociated; // if true then transaction state is rolled back when the object is destroyed
}; 

typedef FrameworkResourcePtr<TransactionContextHandle> TransactionContextHandlePtr;

} // Blaze

#endif // BLAZE_TRANSACTION_H

