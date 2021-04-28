// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#if EADPSDK_USE_STD_STL
#include <functional>
#else
#include <EASTL/functional.h>
#endif

#include <eadp/foundation/Error.h>

namespace eadp
{
namespace foundation
{

template <typename>
class CallbackT;

/*!
 * @brief Callback base class
 *
 * Callback is a mechanism for returning result of an asynchronous API call.
 * It is required for game to drive the callback mechansim using either Hub.invokeCallbacks()
 * or Hub.invokeCallbacks(groupId).
 * If game doesn't need callbacks to be grouped by thread or other attribute, it can use Hub.invokeCallbacks()
 * to trigger all callbacks.
 * If game uses Callback::GroupId to group callback per thread or other way, it need to call
 * Hub.invokeCallbacks(groupId) to trigger callbacks for specific group id.
 * Normally, we recommend to do invokeCallbacks() call(s) right after Hub.idle().
 *
 * The base class defines related class/type and provides the basic accessor to important attributes of callback.
 */
class EADPSDK_API Callback
{
public:
    /*!
     * @brief Callback persistence is for managing the callback longevity
     *
     * The persistence object helps to guard against calling back the client code when the caller has been
     * destroyed after submitting the request but before the callback is invoked. The persistence object
     * shared pointer is hold by the caller, and the weak pointer of the same persistence object is hold by
     * callback class. The callback will verify the weak pointer before triggering the callback function to
     * avoid invoking an invalid callback. The client can share the persistence object across various callbacks
     * or provide a persistence object per callback.
     */
    class EADPSDK_API Persistence {};
    using PersistencePtr = SharedPtr<Persistence>;
    using PersistenceWeakPtr = WeakPtr<Persistence>;

    /*!
     * @brief Callback group id is used to group callbacks for invoking.
     *
     * The Callback class allows the callback to be tagged with a callback group id, so game can invoke callback
     * group by group.
     * A common use case of callback group id is the caller wants to receive the response in a specific thread
     * context, so he creates the callback with a group id assocaited to that response thread, and then he
     * calls the Hub.invokeCallbacks(groupId) from that thread with the corresponding group id.
     * An example of such system is job_manager based systems where jobs may change the OS thread frame to frame.
     * Note that the different invocation of the same rpc can use different callback group Id.
     *
     * The caller should not share the same group id across multiple jobs/threads to invoke callbacks otherwise
     * you may end up with out of order streaming responses.
     */
    using GroupId = uint32_t;

    /*
     * @brief The callback group id which will be invoked automatically by Hub.idle()
     *
     * This is the group id used by eadp-sdk for internal callback, and those callbacks will be invoked
     * automatically in the Hub.idle(), and no explicit Hub.invokeCallbacks() call needed for them.
     * The game can also use this group id if automtic invoking is appreciated.
     */
    static const GroupId kIdleInvokeGroupId = 0xffff;
    
    /*!
     * @brief GroupId get accessor
     */
    GroupId getGroupId() const
    {
        return m_groupId;
    }

    /*!
     * @brief GroupId set accessor
     */
    void setGroupId(GroupId groupId)
    {
        m_groupId = groupId;
    }

    /*!
     * @brief Persistence object get accessor
     */
    const PersistenceWeakPtr& getPersistenceObject() const
    {
        return m_persistenceObject;
    }
    
    /*!
     * @brief Check if the callback is still valid
     */
    bool isValid() const
    {
        return !m_persistenceObject.expired();
    }

    /*!
    * @brief Invalidate the existing callback.
    */
    virtual void reset()
    {
        m_persistenceObject = PersistenceWeakPtr();
        m_groupId = 0;
    }

protected:
    Callback(PersistenceWeakPtr&& persistenceObject, GroupId groupId)
        : m_persistenceObject(EADPSDK_MOVE(persistenceObject)), m_groupId(groupId)
    {
    }

    virtual ~Callback() = default;

    PersistenceWeakPtr m_persistenceObject;
    GroupId m_groupId;
};

/*!
 * @brief Callback template class
 *
 * The callback class is templated of callback function signature.
 */
template<typename Result, typename... Arguments>
class CallbackT<Result(Arguments...)> : public Callback
{
public:
#if EADPSDK_USE_STD_STL
    using CallbackFunctionType = std::function<Result(Arguments...)>;
#else
    using CallbackFunctionType = eastl::function<Result(Arguments...)>;
#endif

    /*!
     * @brief Constructor for empty callback from nullptr
     */
    CallbackT(std::nullptr_t)
    : Callback(PersistenceWeakPtr(), kIdleInvokeGroupId)
    {
    }

    /*!
     * @brief Callback constructor
     *
     * @param callback The eastl::function functor which callback class wraps.
     * @param persistenceObject The persistence object which is bound to the validity of the callback function.
     * @param groupId The group id assigned to the callback.
     * @see Persistence, GroupId
     */
    CallbackT(CallbackFunctionType callback, PersistenceWeakPtr persistenceObject, GroupId groupId)
        : Callback(EADPSDK_MOVE(persistenceObject), groupId)
        , m_callback(EADPSDK_MOVE(callback))
    {
    }

    /*!
     * @brief Invoke the callback function with arguments.
     */
    Result operator()(Arguments... args) const
    {
        if (m_callback && !m_persistenceObject.expired())
        {
            return m_callback(EADPSDK_FORWARD<Arguments>(args)...);
        }
        else
        {
            return Result();
        }
    }

    /*!
    * @brief return whether the callback is empty.
    */
    bool isEmpty() const
    {
        return !m_callback;
    }

    /*!
    * @brief Invalidate the existing callback.
    */
    void reset() override
    {
        m_callback = nullptr;
        Callback::reset();
    }

private:
    CallbackFunctionType m_callback;
};

/*!
 * @brief RPC Callback template class
 *
 * The RPC callback class is templated for an RPC request and response type.
 */
template<typename Request, typename Response>
class RpcCallback : public Callback
{
public:
#if EADPSDK_USE_STD_STL
    using CallbackFunctionType = std::function<void(UniquePtr<Response>, ErrorPtr)>;
#else
    using CallbackFunctionType = eastl::function<void(UniquePtr<Response>, ErrorPtr)>;
#endif
    
    /*!
     * @brief Callback constructor
     *
     * @param callback The eastl::function functor which callback class wraps.
     * @param persistenceObject The persistence object which is bound to the validity of the callback function.
     * @param groupId The group id assigned to the callback.
     * @see Persistence, GroupId
     */
    RpcCallback(CallbackFunctionType callback, PersistenceWeakPtr persistenceObject, GroupId groupId)
    : Callback(EADPSDK_MOVE(persistenceObject), groupId)
    , m_callback(EADPSDK_MOVE(callback))
    {
    }
    
    /*!
     * @brief Invoke the callback function with standard argument signature.
     */
    void operator()(UniquePtr<Response> response, ErrorPtr error) const
    {
        if (m_callback && !m_persistenceObject.expired())
        {
            m_callback(EADPSDK_MOVE(response), EADPSDK_MOVE(error));
        }
    }
    
private:
    CallbackFunctionType m_callback;
};

}
}
