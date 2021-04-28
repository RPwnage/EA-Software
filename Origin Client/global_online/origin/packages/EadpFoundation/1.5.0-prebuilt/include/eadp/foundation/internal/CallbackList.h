// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Callback.h>
#include <eadp/foundation/Hub.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

template<typename CallbackType>
class CallbackList
{
public:
    /*!
    * @brief CallbackList constructor
    *
    * @param hub The hub to use for allocations and response handling.
    */
    explicit CallbackList(IHub* hub)
        : m_hub(hub)
        , m_callbacks(hub->getAllocator().make<Vector<CallbackType>>())
    {
    }

    /*!
     *  @brief Add a callback to the list.
     */
    void add(const CallbackType& callback)
    {
        m_callbacks.push_back(callback);
    }

    /*!
     *  @brief Add a callback to the list.
     */
    void add(CallbackType&& callback)
    {
        m_callbacks.emplace_back(EADPSDK_MOVE(callback));
    }

    /*!
     *  @brief Clear the callback list
     */
    void clear()
    {
        m_callbacks.clear();
    }

    /*!
    * @brief Add responses to the hub with specified arguments.
    */
    template<typename ResponseType, typename... ResponseArguments>
    void respond(const char* responseName, ResponseArguments... args)
    {
        auto iter = m_callbacks.begin();
        auto liveIter = m_callbacks.begin();
        while (iter != m_callbacks.end())
        {
            // check if callback is still valid; if yes, invoke it; otherwise clean it
            if (iter->isValid())
            {
                auto& allocator = m_hub->getAllocator();
                m_hub->addResponse(allocator.makeUnique<ResponseType>(allocator,
                                                                      responseName,
                                                                      *iter,
                                                                      args...));

                // reposition live callback
                if (iter != liveIter)
                {
                    *liveIter = *iter;
                }
                liveIter++;
            }
            iter++;
        }

        // remove dead callbacks
        if (liveIter != iter)
        {
            m_callbacks.erase(liveIter, iter);
        }
    }

private:
    IHub* m_hub;
    Vector<CallbackType> m_callbacks;
};

}
}
}
