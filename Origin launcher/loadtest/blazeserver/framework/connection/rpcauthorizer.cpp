/*************************************************************************************************/
/*!
    \file rpcauthorizer.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RpcAuthorizer

    Encapsulates functionality to whitelist/blacklist rpcs on an endpoint.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/component/componentstub.h"
#include "framework/connection/rpcauthorizer.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/component/component.h"

namespace Blaze
{

/*** Public Methods ******************************************************************************/
RpcAuthorizer::RpcAuthorizer(Metrics::MetricsCollection& metricsCollection, const char8_t* metricName)
: mMetricsCollection(metricsCollection)
, mAuthorizedCommandsList(BlazeStlAllocator("RpcAuthorizer::mAuthorizedCommandsList"))
, mAuthorizedCommandsByPlatform(BlazeStlAllocator("RpcAuthorizer::mAuthorizedCommandByPlatform"))
, mRpcAuthorizationFailureCount(mMetricsCollection, metricName)
{
   
}

RpcAuthorizer::~RpcAuthorizer()
{
  
}

void RpcAuthorizer::insertWhitelistNameForPlatforms(const ClientPlatformSet& blacklistPlatforms, const char8_t* whitelistName, PlatformSetByWhitelistMap& whitelistMap, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists)
{
    EndpointConfig::PlatformSpecificControlListsMap::const_iterator plistItr = platformSpecificControlLists.find(whitelistName);
    if (plistItr == platformSpecificControlLists.end())
    {
        whitelistMap[whitelistName].insert(blacklistPlatforms.begin(), blacklistPlatforms.end());
    }
    else
    {
        for (const auto& itr : *(plistItr->second))
        {
            ClientPlatformSet& whitelistPlatforms = whitelistMap[itr.first.c_str()];
            for (const auto& platform : *itr.second)
            {
                if (blacklistPlatforms.find(platform) != blacklistPlatforms.end())
                    whitelistPlatforms.insert(platform);
            }
        }
    }
}

void RpcAuthorizer::insertAuthorizedCommands(ComponentId cmpId, CommandId cmdId, RpcAuthorizer::CommandIdsByComponentIdMap& authorizedCommands, const RpcAuthorizer::CommandIdsByComponentIdMap& blacklistCommands)
{
    CommandIdsByComponentIdMap::const_iterator blacklistCmpItr = blacklistCommands.find(cmpId);

    // Check if the entire component is blacklisted
    if (blacklistCmpItr != blacklistCommands.end() && blacklistCmpItr->second.size() == 1 && *blacklistCmpItr->second.begin() == Component::INVALID_COMMAND_ID)
        return;

    CommandIdSet& commandIdSet = authorizedCommands[cmpId];
    if (cmdId != Component::INVALID_COMMAND_ID)
    {
        if (blacklistCmpItr == blacklistCommands.end() || blacklistCmpItr->second.find(cmdId) == blacklistCmpItr->second.end())
            commandIdSet.insert(cmdId);
    }
    else
    {
        const ComponentData* data = ComponentData::getComponentData(cmpId);
        if (data != nullptr)
        {
            for (const auto& it : data->commands)
            {
                if (blacklistCmpItr == blacklistCommands.end() || blacklistCmpItr->second.find(it.first) == blacklistCmpItr->second.end())
                    commandIdSet.insert(it.first);
            }
        }
    }
}

void RpcAuthorizer::initializeRpcAuthorizationList(const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists)
{
    ComponentId cmpId;
    CommandId cmdId;
    uint32_t authorizedCommandsListIndex = 0;

    mAuthorizedCommandsByPlatform.clear();
    mAuthorizedCommandsList.clear();

    typedef eastl::hash_map<eastl::string, PlatformSetByWhitelistMap> PlatformSetByWhitelistByBlacklistMap;
    PlatformSetByWhitelistByBlacklistMap platformSetMap(BlazeStlAllocator("Endpoint::tempPlatformSetByWhitelistByBlacklistMap"));

    EndpointConfig::PlatformSpecificControlListsMap::const_iterator plistItr = platformSpecificControlLists.find(rpcBlackListName);
    if (plistItr == platformSpecificControlLists.end())
    {
        insertWhitelistNameForPlatforms(gController->getUsedPlatforms(), rpcWhiteListName, platformSetMap[rpcBlackListName], rpcControlLists, platformSpecificControlLists);
    }
    else
    {
        // Platform-specific control lists can be used to assign blacklists to a subset of gController->getUsedPlatforms().
        // We need to track which platforms weren't given a blacklist, to make sure we still build an AuthorizedCommands map for them.
        ClientPlatformSet remainingPlatforms = gController->getUsedPlatforms();
        for (const auto& itr : *(plistItr->second))
        {
            ClientPlatformSet blacklistPlatforms;
            for (const auto& platform : *itr.second)
            {
                if (gController->isPlatformUsed(platform))
                {
                    blacklistPlatforms.insert(platform);
                    remainingPlatforms.erase(platform);
                }
            }
            insertWhitelistNameForPlatforms(blacklistPlatforms, rpcWhiteListName, platformSetMap[itr.first.c_str()], rpcControlLists, platformSpecificControlLists);
        }
        if (!remainingPlatforms.empty())
            insertWhitelistNameForPlatforms(remainingPlatforms, rpcWhiteListName, platformSetMap[""], rpcControlLists, platformSpecificControlLists);
    }

    for (const auto& blacklistItr : platformSetMap)
    {
        // Build the map of blacklisted commands
        CommandIdsByComponentIdMap  blacklistCommands(BlazeStlAllocator("Endpoint::tempBlacklistCommandsMap"));
        EndpointConfig::CommandListByNameMap::const_iterator blistItr = rpcControlLists.find(blacklistItr.first.c_str());
        if (blistItr != rpcControlLists.end())
        {
            for (const auto& cmdIter : *blistItr->second)
            {
                char8_t* rpcName = blaze_strdup(cmdIter.c_str());
                char8_t* saveptr = nullptr;
                char8_t* componentName = blaze_strtok(rpcName, "/", &saveptr);
                char8_t* cmdName = saveptr;

                cmpId = BlazeRpcComponentDb::getComponentIdByName(componentName);
                cmdId = BlazeRpcComponentDb::getCommandIdByName(cmpId, cmdName);
                BLAZE_FREE(rpcName);

                CommandIdSet& cmdIds = blacklistCommands[cmpId];
                if (cmdId == Component::INVALID_COMMAND_ID)
                {
                    // The entire component is listed. Represent this by inserting INVALID_COMMAND_ID.
                    cmdIds.clear();
                    cmdIds.insert(Component::INVALID_COMMAND_ID);
                }
                else if (cmdIds.size() != 1 || *cmdIds.begin() != Component::INVALID_COMMAND_ID)
                {
                    cmdIds.insert(cmdId);
                }
            }
        }

        for (const auto& whitelistItr : blacklistItr.second)
        {
            EndpointConfig::CommandListByNameMap::const_iterator wlistItr = rpcControlLists.find(whitelistItr.first.c_str());
            if (wlistItr != rpcControlLists.end())
            {
                // Create a new AuthorizedCommands entry for this blacklist+whitelist combination
                CommandIdsByComponentIdMap& authorizedCommands = mAuthorizedCommandsList.push_back();

                // Add all whitelisted commands that aren't in the blacklist
                for (const auto& cmdIter : *wlistItr->second)
                {
                    if (cmdIter.c_str()[0] == '*')
                    {
                        // Whitelist specified that everything is whitelisted
                        size_t componentSize = ComponentData::getComponentCount();
                        for (size_t i = 0; i < componentSize; ++i)
                        {
                            cmpId = BlazeRpcComponentDb::getComponentIdFromIndex(i);
                            insertAuthorizedCommands(cmpId, Component::INVALID_COMMAND_ID, authorizedCommands, blacklistCommands);
                        }
                        break;
                    }

                    char8_t* rpcName = blaze_strdup(cmdIter.c_str());
                    char8_t* saveptr = nullptr;
                    char8_t* componentName = blaze_strtok(rpcName, "/", &saveptr);
                    char8_t* cmdName = saveptr;

                    cmpId = BlazeRpcComponentDb::getComponentIdByName(componentName);
                    cmdId = BlazeRpcComponentDb::getCommandIdByName(cmpId, cmdName);
                    BLAZE_FREE(rpcName);

                    insertAuthorizedCommands(cmpId, cmdId, authorizedCommands, blacklistCommands);
                }

                // Associate the new AuthorizedCommands entry with the appropriate platform(s)
                for (const auto& platform : whitelistItr.second)
                    mAuthorizedCommandsByPlatform[platform] = authorizedCommandsListIndex;

                ++authorizedCommandsListIndex;
            }
        }
    }
}

bool RpcAuthorizer::isRpcAuthorized(ClientPlatformType platform, ComponentId cmpId, CommandId cmdId) const
{
    AuthorizedCommandsByPlatformMap::const_iterator platItr = mAuthorizedCommandsByPlatform.find(platform);
    if (platItr != mAuthorizedCommandsByPlatform.end())
    {
        if (platItr->second >= mAuthorizedCommandsList.size())
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[RpcAuthorizer].isRpcAuthorized : AuthorizedCommandsByPlatform map entry (" << platItr->second << ") for platform '" << ClientPlatformTypeToString(platform)
                << "' is out of bounds - AuthorizedCommandsList size is " << mAuthorizedCommandsList.size());
            return false;
        }

        const CommandIdsByComponentIdMap& cmdMap = mAuthorizedCommandsList[platItr->second];
        CommandIdsByComponentIdMap::const_iterator cmpItr = cmdMap.find(cmpId);
        if (cmpItr != cmdMap.end())
        {
            CommandIdSet::const_iterator cmdItr = cmpItr->second.find(cmdId);
            if (cmdItr != cmpItr->second.end())
                return true;
        }
    }

    return false;
}

void RpcAuthorizer::validateRpcControlLists(const EndpointConfig& config, const char8_t* endpointName, const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, CommandIdsByComponentIdByControlListMap& controlListMap, RpcControlListComboMap& controlListComboMap, ConfigureValidationErrors& validationErrors)
{
    ControlListValidationResult blacklistResult = validateRpcControlList(config, endpointName, rpcBlackListName, true, controlListMap, validationErrors);
    ControlListValidationResult whitelistResult = validateRpcControlList(config, endpointName, rpcWhiteListName, false, controlListMap, validationErrors);

    if (whitelistResult == LIST_VALID && blacklistResult == LIST_VALID)
        validateRpcControlListCombination(config, endpointName, rpcWhiteListName, rpcBlackListName, controlListMap, controlListComboMap, validationErrors);
}

RpcAuthorizer::ControlListValidationResult RpcAuthorizer::validateRpcControlList(const EndpointConfig& config, const char8_t* endpointName, const char8_t* listName, bool isBlacklist, CommandIdsByComponentIdByControlListMap& controlListMap, ConfigureValidationErrors& validationErrors)
{
    if (listName[0] == '\0')
        return LIST_EMPTY;

    EndpointConfig::CommandListByNameMap::const_iterator listItr = config.getRpcControlLists().find(listName);
    if (listItr == config.getRpcControlLists().end())
    {
        EndpointConfig::PlatformSpecificControlListsMap::const_iterator clistItr = config.getPlatformSpecificRpcControlLists().find(listName);
        if (clistItr == config.getPlatformSpecificRpcControlLists().end())
        {
            eastl::string msg;
            msg.sprintf("[RpcAuthorizer].validateRpcControlLists() : Undefined RPC %s '%s' for endpoint %s.", (isBlacklist ? "blacklist" : "whitelist"), listName, endpointName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return LIST_INVALID;
        }

        // At this point, we've already called validatePlatformSpecificRpcControlLists to ensure that each key in the PlatformSpecificControlListsMap is a defined RPC control list
        ControlListValidationResult result = LIST_EMPTY;
        for (const auto& itr : *(clistItr->second))
        {
            ControlListValidationResult curResult = validateRpcControlList(config, endpointName, itr.first.c_str(), isBlacklist, controlListMap, validationErrors);
            if (curResult == LIST_EMPTY)
                continue;
            if (result != LIST_INVALID)
                result = curResult;
        }
        return result;
    }

    if (listItr->second->empty())
        return LIST_EMPTY;

    // Early out if we've already validated this control list
    CommandIdsByComponentIdByControlListMap::const_iterator existingListItr = controlListMap.find(listName);
    if (existingListItr != controlListMap.end())
    {
        if (existingListItr->second.empty())
            return LIST_INVALID;

        if (existingListItr->second.begin()->first == Component::INVALID_COMPONENT_ID)
        {
            if (isBlacklist)
            {
                eastl::string msg;
                msg.sprintf("[RpcAuthorizer].validateRpcControlLists() : RPC blacklist for endpoint %s cannot define '*'.", endpointName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return LIST_INVALID;
            }
            return LIST_EMPTY;
        }
        return LIST_VALID;
    }

    ControlListValidationResult result = LIST_VALID;
    CommandIdsByComponentIdMap& cmdMap = controlListMap[listName];
    for (const auto& cmdIter : *(listItr->second))
    {
        if (cmdIter.c_str()[0] == '*')
        {
            // All RPCs are listed. Represent this by inserting INVALID_COMPONENT_ID
            cmdMap.clear();
            cmdMap.insert(Component::INVALID_COMPONENT_ID);
            if (isBlacklist)
            {
                // Blacklisting all RPCs is not permitted
                eastl::string msg;
                msg.sprintf("[RpcAuthorizer].validateRpcControlLists() : RPC blacklist for endpoint %s cannot define '*'.", endpointName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return LIST_INVALID;
            }
            return LIST_EMPTY; // Wildcard whitelists can be combined with any valid blacklist, so for validation purposes we treat them like empty lists
        }

        // First validate that the component and/or command are valid
        char8_t* rpcName = blaze_strdup(cmdIter.c_str());
        char8_t* saveptr = nullptr;
        char8_t* componentName = blaze_strtok(rpcName, "/", &saveptr);
        char8_t* cmdName = saveptr;

        ComponentId cmpId = BlazeRpcComponentDb::getComponentIdByName(componentName);
        CommandId cmdId = Component::INVALID_COMMAND_ID;
        if (cmpId == Component::INVALID_COMPONENT_ID)
        {
            eastl::string msg;
            msg.sprintf("[RpcAuthorizer].validateRpcControlLists() : Invalid component name '%s' specified in RPC control list '%s'.", componentName, listName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            result = LIST_INVALID;
        }
        else if (*cmdName == '\0')
        {
            // The entire component is listed. Represent this by inserting INVALID_COMMAND_ID.
            CommandIdSet& commandSet = cmdMap[cmpId];
            commandSet.clear();
            commandSet.insert(Component::INVALID_COMMAND_ID);
        }
        else
        {
            cmdId = BlazeRpcComponentDb::getCommandIdByName(cmpId, cmdName);
            if (cmdId == Component::INVALID_COMMAND_ID)
            {
                eastl::string msg;
                msg.sprintf("[RpcAuthorizer].validateRpcControlLists() : Invalid command name '%s' specified for component '%s' in RPC control list '%s'.", cmdName, componentName, listName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                result = LIST_INVALID;
            }
            else
            {
                CommandIdSet& commandSet = cmdMap[cmpId];
                // Check that we haven't already listed the entire component
                if (commandSet.size() != 1 || *commandSet.begin() != Component::INVALID_COMMAND_ID)
                    commandSet.insert(cmdId);
            }
        }
        BLAZE_FREE(rpcName);
    }

    if (result == LIST_INVALID)
        cmdMap.clear();

    return result;
}

const char8_t* RpcAuthorizer::getValidationErrorForControlListCombination(const EndpointConfig& config, const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, const CommandIdsByComponentIdByControlListMap& controlListMap, RpcControlListComboMap& controlListComboMap)
{
    // Early out if we've already validated this control list combination
    RpcControlListComboMap::const_iterator comboItr = controlListComboMap.find(rpcBlackListName);
    if (comboItr != controlListComboMap.end())
    {
        ValidationErrorByWhitelistNameMap::const_iterator itr = comboItr->second.find(rpcWhiteListName);
        if (itr != comboItr->second.end())
            return itr->second.c_str();
    }

    // Platform-specific RPC control lists aren't in the CommandIdsByComponentIdByControlListMap.
    // If either the whitelist or the blacklist is platform-specific, return nullptr and let the caller iterate through the per-platform whitelist/blacklist combinations.
    CommandIdsByComponentIdByControlListMap::const_iterator wItr = controlListMap.find(rpcWhiteListName);
    if (wItr == controlListMap.end())
        return nullptr;
    CommandIdsByComponentIdByControlListMap::const_iterator bItr = controlListMap.find(rpcBlackListName);
    if (bItr == controlListMap.end())
        return nullptr;

    // We need to check against what's in our blacklist and apply the following rules
    //   1. If an entire component is whitelisted, it can't also be blacklisted
    //   2. If a specific RPC command is whitelisted, it can't also be blacklisted
    //   3. If a specific RPC command is whitelisted, its component can't be blacklisted
    eastl::string& msg = controlListComboMap[rpcBlackListName][rpcWhiteListName];
    for (const auto& cmpItr : wItr->second)
    {
        CommandIdsByComponentIdMap::const_iterator blacklistItr = bItr->second.find(cmpItr.first);
        if (blacklistItr != bItr->second.end())
        {
            const CommandIdSet& blacklistCmdIds = blacklistItr->second;
            if (blacklistCmdIds.size() == 1 && *blacklistCmdIds.begin() == Component::INVALID_COMMAND_ID)
            {
                // This component is in the whitelist in some fashion, so this entire component can't be in the blacklist
                msg.sprintf("Component '%s'", ComponentData::getComponentData(cmpItr.first)->name);
                return msg.c_str();
            }
            else
            {
                for (const auto& cmdItr : cmpItr.second)
                {
                    if (blacklistItr->second.find(cmdItr) != blacklistItr->second.end())
                    {
                        // This component/command is in the whitelist, so it can't be in the blacklist
                        msg.sprintf("Command '%s'", ComponentData::getCommandInfo(cmpItr.first, cmdItr)->context);
                        return msg.c_str();
                    }
                }
            }
        }
    }

    // No conflicts were found -- return an empty validation error string
    return "";
}

void RpcAuthorizer::validateRpcControlListCombination(const EndpointConfig& config, const char8_t* endpointName, const char8_t* rpcWhiteListName, const char8_t* rpcBlackListName, const CommandIdsByComponentIdByControlListMap& controlListMap, RpcControlListComboMap& controlListComboMap, ConfigureValidationErrors& validationErrors)
{
    const char8_t* validationError = getValidationErrorForControlListCombination(config, rpcWhiteListName, rpcBlackListName, controlListMap, controlListComboMap);
    if (validationError != nullptr)
    {
        if (validationError[0] != '\0')
        {
            eastl::string msg;
            msg.sprintf("[RpcAuthorizer].validateRpcControlLists() : %s is specified in both RPC whitelist and RPC blacklist for endpoint '%s'.", validationError, endpointName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        return;
    }

    // If we got here, then this is a control list combination we haven't validated yet, and the whitelist and/or blacklist is a platform-specific control list.
    // Validate each unique blacklist/whitelist combination for every used platform.
    EndpointConfig::PlatformSpecificControlListsMap::const_iterator pwlItr = config.getPlatformSpecificRpcControlLists().find(rpcWhiteListName);
    EndpointConfig::PlatformSpecificControlListsMap::const_iterator pblItr = config.getPlatformSpecificRpcControlLists().find(rpcBlackListName);
    eastl::string& validationErr = controlListComboMap[rpcBlackListName][rpcWhiteListName];
    for (const auto& platform : gController->getUsedPlatforms())
    {
        const char8_t* whitelistName = pwlItr == config.getPlatformSpecificRpcControlLists().end() ? rpcWhiteListName : getControlListForPlatform(platform, *pwlItr->second);
        const char8_t* blacklistName = pblItr == config.getPlatformSpecificRpcControlLists().end() ? rpcBlackListName : getControlListForPlatform(platform, *pblItr->second);
        if (whitelistName == nullptr || blacklistName == nullptr)
            continue;

        eastl::string errStr = getValidationErrorForControlListCombination(config, whitelistName, blacklistName, controlListMap, controlListComboMap);
        if (!errStr.empty())
        {
            validationErr.sprintf("%s (on platform %s)", errStr.c_str(), ClientPlatformTypeToString(platform));
            eastl::string msg;
            msg.sprintf("[RpcAuthorizer].validateRpcControlLists() : %s is specified in both RPC whitelist and RPC blacklist for endpoint '%s'.", validationErr.c_str(), endpointName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
        }
    }
}

const char8_t* RpcAuthorizer::getControlListForPlatform(ClientPlatformType platform, const EndpointConfig::PlatformsByControlListMap& platformsMap)
{
    for (const auto& listItr : platformsMap)
    {
        for (const auto& platItr : *(listItr.second))
        {
            if (platItr == platform)
                return listItr.first.c_str();
        }
    }
    return nullptr;
}

bool RpcAuthorizer::validatePlatformSpecificRpcControlLists(const EndpointConfig& config, ConfigureValidationErrors& validationErrors)
{
    size_t existingErrors = validationErrors.getErrorMessages().size();
    for (const auto& listItr : config.getPlatformSpecificRpcControlLists())
    {
        if (config.getRpcControlLists().find(listItr.first.c_str()) != config.getRpcControlLists().end())
        {
            eastl::string msg;
            msg.sprintf("[RpcAuthorizer].validatePlatformSpecificRpcControlLists() : Platform-specific RPC control list name '%s' conflicts with existing RPC control list.", listItr.first.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        ClientPlatformSet platformSet;
        bool platformConflict = false;
        for (const auto& cmdsItr : *(listItr.second))
        {
            if (config.getRpcControlLists().find(cmdsItr.first.c_str()) == config.getRpcControlLists().end())
            {
                eastl::string msg;
                msg.sprintf("[RpcAuthorizer].validatePlatformSpecificRpcControlLists() : Platform-specific RPC control list '%s' references undefined RPC control list '%s'", listItr.first.c_str(), cmdsItr.first.c_str());
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            ClientPlatformTypeList::const_iterator platItr = cmdsItr.second->begin();
            for ( ; !platformConflict && platItr != cmdsItr.second->end(); ++platItr )
            {
                if (!platformSet.insert(*platItr).second)
                    platformConflict = true;
            }
        }
        if (platformConflict)
        {
            eastl::string msg;
            msg.sprintf("[RpcAuthorizer].validatePlatformSpecificRpcControlLists() : Found duplicate platform in platform-specific RPC control list '%s'.", listItr.first.c_str());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }
    return validationErrors.getErrorMessages().size() == existingErrors;
}

} // namespace Blaze

