/*! ************************************************************************************************/
/*!
\file wmemanager.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/rete/retesubstringtrie.h"
#include "gamemanager/rete/wmemanager.h"

#include "framework/util/shared/blazestring.h"
#include "framework/debug.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{
        const char8_t* WMEManager::WMEAnyProvider::WME_ANY_STRING = "WME_ANY_HASH"; 

        /*! ************************************************************************************************/
        /*! \brief ctor
        ***************************************************************************************************/
        WMEManager::WMEManager(ReteNetwork* reteNetwork)
            : mReteNetwork(reteNetwork)
            , mWorkingMemoryElements(BlazeStlAllocator("WMEManager::WME", GameManagerSlave::COMPONENT_MEMORY_GROUP))
        {
        }

        /*! ************************************************************************************************/
        /*! \brief destructor
        ***************************************************************************************************/
        WMEManager::~WMEManager()
        {
            for (auto wmeByIdItr : mWorkingMemoryElements)
            {
                removeAllWME(wmeByIdItr.second.mWmeValueByNameMap);
            }
        }

        void WMEManager::registerMatchAnyWME(WMEId id)
        {
            WMEAnyProvider provider;
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].registerMatchAnyWME id " << id);
            insertWorkingMemoryElement(id, WME_ANY_HASH, WME_ANY_HASH, provider);
        }

        void WMEManager::unregisterMatchAnyWME(WMEId id)
        {
            WMEAnyProvider provider;
            BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].unregisterMatchAnyWME id " << id);
            removeWorkingMemoryElement(id, WME_ANY_HASH, WME_ANY_HASH, provider);
        }

        void WMEManager::unregisterWMEId(WMEId id)
        {
            mWorkingMemoryElements.erase(id);
        }

        void WMEManager::eraseWorkingMemoryElement(WMEId id, WMEAttribute wmeName, WMEAttribute wmeValue, bool removeFromRete, const char8_t* stringKey)
        {
            auto wmeItr = mWorkingMemoryElements.find(id);
            if (wmeItr == mWorkingMemoryElements.end())
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].eraseWorkingMemoryElement Attempt to erase non existant wme(" << id << ", "
                    << mStringTable.getStringFromHash(wmeName) << "(" << wmeName << ")," << mStringTable.getStringFromHash(wmeValue) << "("
                    << wmeValue << ") by id.");

                return;
            }

            auto iter = wmeItr->second.mWmeValueByNameMap.find(wmeName);
            if (iter == wmeItr->second.mWmeValueByNameMap.end())
            {

                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].eraseWorkingMemoryElement Attempt to erase non existant wme(" << id << ", "
                    << mStringTable.getStringFromHash(wmeName) << "(" << wmeName << ")," << mStringTable.getStringFromHash(wmeValue) << "("
                    << wmeValue << ") by name.");

                return;
            }

            auto valueItr = iter->second.find(wmeValue);
            if (valueItr != iter->second.end())
            {
                auto& wme = *valueItr->second;
                EA_ASSERT(wme.getValue() == wmeValue);
                if (removeFromRete)
                {
                    if (stringKey == nullptr)
                    {
                        mReteNetwork->removeWorkingMemoryElement(wme);
                    }
                    else
                    {
                        mReteNetwork->removeWorkingMemoryElement(wme, stringKey);
                    }
                }

                iter->second.erase(valueItr);
                if (iter->second.empty())
                {
                    wmeItr->second.mWmeValueByNameMap.erase(iter);
                    if (wmeItr->second.mWmeValueByNameMap.empty())
                    {
                        mWorkingMemoryElements.erase(wmeItr);
                    }
                }
                
                mWMEFactory.deleteWorkingMemoryElement(wme);
            }
            else
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].eraseWorkingMemoryElement: Expected wme " << id << ", name "
                    << mStringTable.getStringFromHash(wmeName) << "(" << wmeName << "), value " << mStringTable.getStringFromHash(wmeValue) << "(" << wmeValue
                    << ") but actual value was not found!");
            }
        }

        void WMEManager::removeAllWME(WmeValueByNameMap& map)
        {
            for (auto wmeValuesItr : map)
            {
                for (auto wmeValueItr : wmeValuesItr.second)
                {
                    auto& wme = *wmeValueItr.second;
                    if (mReteNetwork->getAlphaNetwork().isInAlphaMemory(wme.getName(), wme.getValue()))
                    {
                        mReteNetwork->removeAlphaMemory(wme);
                    }
                    else
                    {
                        // remove all substring facts
                        mReteNetwork->removeWorkingMemoryElement(wme, mReteNetwork->getSubstringTrieWordTable().getStringFromHash(wme.getValue()));
                    }
                    mWMEFactory.deleteWorkingMemoryElement(wme);
                }
            }
            map.clear();
        }

        bool WMEManager::addWorkingMemoryElement(WorkingMemoryElement &wme)
        {
            auto& wmeMap = mWorkingMemoryElements[wme.getId()].mWmeValueByNameMap;
            auto& wmeValueMap = wmeMap[wme.getName()];
            auto ret = wmeValueMap.insert(eastl::make_pair(wme.getValue(), &wme));
            if (ret.second)
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].addWorkingMemoryElement wme(" << wme.getId() << ","
                    << mStringTable.getStringFromHash(wme.getName()) << "(" << wme.getName() << "),"
                    << mStringTable.getStringFromHash(wme.getValue()) << "(" << wme.getValue() << ")).");

                return true;
            }

            if (ret.first->second == &wme)
            {
                BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].addWorkingMemoryElement exact wme already exists, wme(" << wme.getId() << ","
                    << mStringTable.getStringFromHash(wme.getName()) << "(" << wme.getName() << "),"
                    << mStringTable.getStringFromHash(wme.getValue()) << "(" << wme.getValue() << ")).");
            }
            else
            {
                BLAZE_WARN_LOG(mReteNetwork->getLogCategory(), "[WMEManager].addWorkingMemoryElement wme with the same value already exists, wme(" << wme.getId() << ","
                    << mStringTable.getStringFromHash(wme.getName()) << "(" << wme.getName() << "),"
                    << mStringTable.getStringFromHash(wme.getValue()) << "(" << wme.getValue() << ")).");
            }
                
            return false;
        }

        WMEValueMap* WMEManager::getWorkingMemoryElements(WMEId id, WMEAttribute wmeName)
        {
            auto wmeItr = mWorkingMemoryElements.find(id);
            if (wmeItr != mWorkingMemoryElements.end())
            {
                auto itr = wmeItr->second.mWmeValueByNameMap.find(wmeName);
                if (itr != wmeItr->second.mWmeValueByNameMap.end())
                {
                    return &itr->second;
                }
            }

            return nullptr;
        }

        // DEPRECATED interface - not removing due to possibility of custom rules using it.
        void WMEManager::insertWorkingMemoryElement(WMEId id, WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider)
        {
            // INSERT WME
            WorkingMemoryElement& newWME = mWMEFactory.createWorkingMemoryElement();
            newWME.setId(id);
            newWME.setName(name);
            newWME.setValue(value);

            if (addWorkingMemoryElement(newWME))
            {
                mReteNetwork->addWorkingMemoryElement(newWME);
            }
            else
            {
                mWMEFactory.deleteWorkingMemoryElement(newWME);
            }

        }

        void WMEManager::insertWorkingMemoryElement(WMEId id, const char8_t* name, WMEAttribute value, const ConditionProviderDefinition& provider, bool isGarbageCollectable/* = false*/)
        {
            insertWorkingMemoryElement(id, Condition::getWmeName(name, provider, isGarbageCollectable), value, provider);
        }

        /*! ************************************************************************************************/
        /*! \brief inserts the substring facts for stringKey
        *************************************************************************************************/
        void WMEManager::insertWorkingMemoryElementToSubstringTrie(WMEId id, WMEAttribute name, const char8_t *stringKey)
        {
            // INSERT WME
            WorkingMemoryElement& newWME = mWMEFactory.createWorkingMemoryElement();
            newWME.setId(id);
            newWME.setName(name);
            newWME.setValue(mReteNetwork->getSubstringTrieWordTable().reteHash(stringKey));

            if (addWorkingMemoryElement(newWME))
            {
                mReteNetwork->addWorkingMemoryElement(newWME, stringKey);
            }
            else
            {
                mWMEFactory.deleteWorkingMemoryElement(newWME);
            }
        }

        void WMEManager::upsertWorkingMemoryElement(WMEId id, const char8_t* name, WMEAttribute value, const ConditionProviderDefinition& provider, bool isGarbageCollectable/* = false*/)
        {
            upsertWorkingMemoryElement(id, Condition::getWmeName(name, provider, isGarbageCollectable), value, provider);
        }

        // DEPRECATED interface - not removing due to possibility of custom rules using it.
        void WMEManager::upsertWorkingMemoryElement(WMEId id, WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider)
        {
            auto* valueMap = getWorkingMemoryElements(id,  name);

            if (valueMap == nullptr)
            {
                insertWorkingMemoryElement(id, name, value, provider);
            }
            else
            {
                // NOTE: Inserting multiple WME id/name pairs with different values is not a use case that is needed by the search slave which is the only user of upsertWorkingMemoryElement() API. The game packer never modifies WMEs once they are inserted.
                EA_ASSERT_MSG(valueMap->size() == 1, "Upserting multi-value WME not supported!");
                auto* wme = valueMap->begin()->second;
                // UPDATE WME
                // If the WME value has changed then we need to update.
                if (wme->getValue() == value)
                {
                    BLAZE_SPAM_LOG(mReteNetwork->getLogCategory(), "[WMEManager].upsertWorkingMemoryElement skip updating wme(" << wme->getId() << ","
                        << mStringTable.getStringFromHash(wme->getName()) << "(" << wme->getName() << "),"
                        << mStringTable.getStringFromHash(wme->getValue()) << "(" << wme->getValue() << ")) with "
                        << mStringTable.getStringFromHash(value) << "(" << value << ").");
                }
                else
                {
                    BLAZE_TRACE_LOG(mReteNetwork->getLogCategory(), "[WMEManager].upsertWorkingMemoryElement updating wme(" << wme->getId() << ","
                        << mStringTable.getStringFromHash(wme->getName()) << "(" << wme->getName() << "),"
                        << mStringTable.getStringFromHash(wme->getValue()) << "(" << wme->getValue() << ")) with "
                        << mStringTable.getStringFromHash(value) << "(" << value << ").");

                    valueMap->clear();
                    WMEAttribute oldValue = wme->getValue();
                    wme->setValue(value);
                    (*valueMap)[value] = wme;
                    mReteNetwork->upsertWorkingMemoryElement(oldValue, *wme);
                }
            }
        }


        void WMEManager::removeWorkingMemoryElement(WMEId id, const char8_t* name, WMEAttribute value, const ConditionProviderDefinition& provider, bool isGarbageCollectable/* = false*/)
        {
            removeWorkingMemoryElement(id, Condition::getWmeName(name, provider, isGarbageCollectable), value, provider);
        }

        // DEPRECATED interface - not removing due to possibility of custom rules using it.
        void WMEManager::removeWorkingMemoryElement(WMEId id, WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider)
        {
            eraseWorkingMemoryElement(id, name, value, true);
        }

        /*! ************************************************************************************************/
        /*! \brief removes the substring facts for stringKey
        *************************************************************************************************/
        void WMEManager::removeWorkingMemoryElementToSubstringTrie(WMEId id, WMEAttribute name, const char8_t* stringKey)
        {
            WMEAttribute value = UINT64_MAX;
            if (mReteNetwork->getSubstringTrieWordTable().findHash(stringKey, value))
            {
                eraseWorkingMemoryElement(id, name, value, true, stringKey);
            }
            else
            {
                BLAZE_WARN_LOG(mReteNetwork->getLogCategory(), "[WMEManager].removeWorkingMemoryElementToSubstringTrie: Expected wme " << id << ", name"
                    << mStringTable.getStringFromHash(name) << "(" << name << ")," << " string (" << stringKey << ") not found in word table.");
            }
        }

        void WMEManager::removeAllWorkingMemoryElements(WMEId id)
        {
            auto itr = mWorkingMemoryElements.find(id);
            if (itr != mWorkingMemoryElements.end())
            {
                removeAllWME(itr->second.mWmeValueByNameMap);
            }
        }

        void WMEManager::removeWMEProvider(WMEAttribute name, WMEAttribute value, const ConditionProviderDefinition& provider)
        {
            if (mReteNetwork->getAlphaNetwork().isInAlphaMemory(name, value))
            {
                // first, this will mark it as providerless, and erase all the WMEs if the listeners are already cleaned up
                mReteNetwork->getAlphaNetwork().removeAlphaMemoryProvider(name, value);

            }
        }


} // namespace Rete
} // namespace GameManager
} // namespace Blaze

