
#include "Ignition/UserExtendedData.h"
#include "BlazeSDK/shared/framework/locales.h"


namespace Ignition
{


UserExtendedData::UserExtendedData(uint32_t userIndex)
    : LocalUserUiBuilder("UserExtendedData", userIndex)
{
    getActions().add(&getActionGetUserExtendedData());
    getActions().add(&getActionGetUserExtendedDataForUser());
    getActions().add(&getActionAddClientAttribute());
    getActions().add(&getActionRemoveClientAttribute());
    getActions().add(&getActionSetUserInfoAttribute());
    getActions().add(&getActionSetCrossPlatformOptIn());
    getActions().add(&getActionLookupUsersByBlazeId());
    getActions().add(&getActionLookupUsersByPersonaNames());
    getActions().add(&getActionLookupUsersByPersonaNamePrefix());
    getActions().add(&getActionLookupUsersByNucleusAccountId());
    getActions().add(&getActionLookupUsersByOriginPersonaId());
    getActions().add(&getActionLookupUsersByOriginPersonaName());
    getActions().add(&getActionLookupUsersBy1stPartyId());
    getActions().add(&getActionLookupUsersByExternalId());
#ifdef BLAZE_USER_PROFILE_SUPPORT
    getActions().add(&getActionGetUserProfiles());
#endif
}

UserExtendedData::~UserExtendedData()
{
}


void UserExtendedData::onAuthenticated()
{
    setIsVisible(true);
}

void UserExtendedData::onDeAuthenticated()
{
    setIsVisible(false);

    getWindows().clear();
}

void UserExtendedData::initActionGetUserExtendedData(Pyro::UiNodeAction &action)
{
    action.setText("Get UserExtendedData");
    action.setDescription("Gets UserExtendedData for the current user.");
}

void UserExtendedData::initActionGetUserExtendedDataForUser(Pyro::UiNodeAction &action)
{
    action.setText("Get UserExtendedData For User");
    action.setDescription("Gets UserExtendedData for a specified product and user.");

    action.getParameters().addInt64("blazeId", 0, "Blaze ID", "", "The BlazeId of the user to get the UserExtendedData for.");
}

void UserExtendedData::actionGetUserExtendedData(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if( localUser == nullptr)
    {
        if((getUiDriver() != nullptr))
            getUiDriver()->showMessage_va( Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr"); 
        return;
    }

    parameters->addInt64("blazeId", localUser->getId());
    actionGetUserExtendedDataForUser(action, parameters);
}

void UserExtendedData::actionGetUserExtendedDataForUser(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::UserManager::FetchUserExtendedDataCb cb(this, &UserExtendedData::getUserExtendedDataCb);
    gBlazeHub->getUserManager()->fetchUserExtendedData( parameters["blazeId"], cb );
}

void UserExtendedData::getUserExtendedDataCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::UserManager::User* user)
{
    if (error == Blaze::ERR_OK)
    {
        // draw the user extended data
        Pyro::UiNodeWindow &window = getWindow("UserExtendedData");
        window.setDockingState(Pyro::DockingState::DOCK_TOP);

        Pyro::UiNodeTable &table = window.getTable("UserExtendedData Lookup Results");
        // if the table already exists, empty out the previous lookups.
        table.getRows().clear();

        table.getColumn("type").setText("Type");
        table.getColumn("comp").setText("Component");
        table.getColumn("index").setText("Index");
        table.getColumn("value").setText("Value");

        int curRow = 0;
        char buf[32] = "";
        blaze_snzprintf(buf, 32, "%d", curRow);
        Pyro::UiNodeTableRowFieldContainer &fields = table.getRow(buf).getFields();
        ++curRow;

        fields["type"] = "User Info Attribute";
        fields["comp"] = "N/A";
        fields["index"] = "N/A";
        fields["value"] = user->getExtendedData()->getUserInfoAttribute();

        fields["type"] = "Cross Platform Opt In";
        fields["comp"] = "N/A";
        fields["index"] = "N/A";
        fields["value"] = user->getExtendedData()->getCrossPlatformOptIn();

        {
            const Blaze::UserExtendedDataMap& map = user->getExtendedData()->getDataMap();
            Blaze::UserExtendedDataMap::const_iterator iter = map.begin();
            for( ;iter != map.end(); ++iter )
            {
                blaze_snzprintf(buf, 32, "%d", curRow);
                Pyro::UiNodeTableRowFieldContainer &fields2 = table.getRow(buf).getFields();
                ++curRow;

                uint16_t comp = ((iter->first>>16)&0xFFFF);
                fields2["type"] = (comp == 0) ? "Client Attribute (DataMap)" : "Internal Attribute (DataMap)";
                fields2["comp"] = gBlazeHub->getComponentManager()->getComponentName(comp);
                fields2["index"] =((iter->first)&0xFFFF);
                fields2["value"] = iter->second;
            }
        }

    }

    REPORT_BLAZE_ERROR(error);
}


void UserExtendedData::initActionSetCrossPlatformOptIn(Pyro::UiNodeAction &action)
{
    action.setText("Set Cross Platform Opt-In");
    action.setDescription("Sets Cross Platform Opt-In for the current user.");

    action.getParameters().addBool("optIn", false, "OptIn", "The setting for the cross platform opt-in.");
}

void UserExtendedData::actionSetCrossPlatformOptIn(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if (localUser == nullptr)
    {
        if ((getUiDriver() != nullptr))
            getUiDriver()->showMessage_va(Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr");
        return;
    }
    Blaze::UserSessionsComponent::SetUserCrossPlatformOptInCb cb(this, &UserExtendedData::setUserCrossPlatformOptInCb);
    localUser->setCrossPlatformOptIn(parameters["optIn"], cb);
}

void UserExtendedData::setUserCrossPlatformOptInCb(Blaze::BlazeError error, Blaze::JobId jobId)
{
    if (error == Blaze::ERR_OK)
    {
        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
        if (localUser)
        {
            Blaze::UserManager::UserManager::FetchUserExtendedDataCb cb(this, &UserExtendedData::getUserExtendedDataCb);
            gBlazeHub->getUserManager()->fetchUserExtendedData(localUser->getId(), cb);
        }
    }

    REPORT_BLAZE_ERROR(error);
}


void UserExtendedData::initActionAddClientAttribute(Pyro::UiNodeAction &action)
{
    action.setText("Add Client Attribute");
    action.setDescription("Sets a ClientAttribute for the current user.");

    action.getParameters().addInt64("attrIndex", 0, "Attribute Index", "", "The ClientAttribute index that is being set.");
    action.getParameters().addInt64("attrValue", 0, "Attribute Value", "", "The ClientAttribute value that is being set.");
}

void UserExtendedData::actionAddClientAttribute(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if( localUser == nullptr)
    {
        if((getUiDriver() != nullptr))
            getUiDriver()->showMessage_va( Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr"); 
        return;
    }

    Blaze::UserSessionsComponent::UpdateExtendedDataAttributeCb cb(this, &UserExtendedData::updateExtendedDataAttributeCb);
    localUser->setUserSessionAttribute(gBlazeHub, parameters["attrIndex"], parameters["attrValue"], cb );
}

void UserExtendedData::updateExtendedDataAttributeCb(Blaze::BlazeError error, Blaze::JobId jobId)
{
    if (error == Blaze::ERR_OK)
    {
        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
        if( localUser )
        {
            Blaze::UserManager::UserManager::FetchUserExtendedDataCb cb(this, &UserExtendedData::getUserExtendedDataCb);
            gBlazeHub->getUserManager()->fetchUserExtendedData( localUser->getId(), cb );        
        }
    }

    REPORT_BLAZE_ERROR(error);
}


void UserExtendedData::initActionRemoveClientAttribute(Pyro::UiNodeAction &action)
{
    action.setText("Remove Client Attribute");
    action.setDescription("Removes a ClientAttribute for the current user.");

    action.getParameters().addInt64("attrIndex", 0, "Attribute Index", "", "The ClientAttribute index that is being removed.");
}

void UserExtendedData::actionRemoveClientAttribute(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if( localUser == nullptr)
    {
        if((getUiDriver() != nullptr))
            getUiDriver()->showMessage_va( Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr"); 
        return;
    }

    Blaze::UserSessionsComponent::UpdateExtendedDataAttributeCb cb(this, &UserExtendedData::updateExtendedDataAttributeCb);
    localUser->removeUserSessionAttribute(gBlazeHub, parameters["attrIndex"], cb );
}

void UserExtendedData::removeExtendedDataAttributeCb(Blaze::BlazeError error, Blaze::JobId jobId)
{
    if (error == Blaze::ERR_OK)
    {
        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
        if( localUser )
        {
            Blaze::UserManager::UserManager::FetchUserExtendedDataCb cb(this, &UserExtendedData::getUserExtendedDataCb);
            gBlazeHub->getUserManager()->fetchUserExtendedData( localUser->getId(), cb );        
        }
    }

    REPORT_BLAZE_ERROR(error);
}

void UserExtendedData::initActionSetUserInfoAttribute(Pyro::UiNodeAction &action)
{
    action.setText("Set UserInfoAttribute");
    action.setDescription("Sets UserInfoAttribute for the current user.");

    action.getParameters().addInt64("userInfoAttribute", 0, "UserInfoAttribute", "", "The UserInfoAttribute value that is being set.");
}

void UserExtendedData::actionSetUserInfoAttribute(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if( localUser == nullptr)
    {
        if((getUiDriver() != nullptr))
            getUiDriver()->showMessage_va( Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr"); 
        return;
    }

    Blaze::SetUserInfoAttributeRequest request;
    request.getBlazeObjectIdList().push_back( localUser->getUser()->getBlazeObjectId() );
    request.setAttributeBits( parameters["userInfoAttribute"] );

    Blaze::UserSessionsComponent::SetUserInfoAttributeCb cb(this, &UserExtendedData::setUserInfoAttributeCb);
    gBlazeHub->getComponentManager(0)->getUserSessionsComponent()->setUserInfoAttribute(request, cb);
}

void UserExtendedData::setUserInfoAttributeCb(Blaze::BlazeError error, Blaze::JobId jobId)
{
    if (error == Blaze::ERR_OK)
    {
        Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
        if( localUser )
        {
            Blaze::UserManager::UserManager::FetchUserExtendedDataCb cb(this, &UserExtendedData::getUserExtendedDataCb);
            gBlazeHub->getUserManager()->fetchUserExtendedData( localUser->getId(), cb );        
        }
    }

    REPORT_BLAZE_ERROR(error);
}

void UserExtendedData::setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, const char8_t* rowValue)
{
    Pyro::UiNodeTableRow& childRow = parentRow.getChildRows().getById(rowId);
    childRow.setText(rowText);
    childRow.getField("value") = rowValue;
}
void UserExtendedData::setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, bool rowValue)
{
    Pyro::UiNodeTableRow& childRow = parentRow.getChildRows().getById(rowId);
    childRow.setText(rowText);
    childRow.getField("value") = rowValue ? "true" : "false";
}
void UserExtendedData::setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, uint32_t locale)
{
    Pyro::UiNodeTableRow& childRow = parentRow.getChildRows().getById(rowId);
    childRow.setText(rowText);
    char8_t buf[5];
    LocaleTokenCreateLocalityString(buf, locale);
    size_t startIdx = 0;
    while (startIdx < 4 && buf[startIdx] == '\0')
        ++startIdx;
    childRow.getField("value") = &buf[startIdx];
}
void UserExtendedData::setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, int64_t rowValue)
{
    Pyro::UiNodeTableRow& childRow = parentRow.getChildRows().getById(rowId);
    childRow.setText(rowText);
    eastl::string buf;
    buf.append_sprintf("%" PRIi64, rowValue);
    childRow.getField("value") = buf.c_str();
}
void UserExtendedData::setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, uint64_t rowValue)
{
    Pyro::UiNodeTableRow& childRow = parentRow.getChildRows().getById(rowId);
    childRow.setText(rowText);
    eastl::string buf;
    buf.append_sprintf("%" PRIu64, rowValue);
    childRow.getField("value") = buf.c_str();
}
void UserExtendedData::setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, const Blaze::EaIdentifiers& eaIds)
{
    Pyro::UiNodeTableRow& childRow = parentRow.getChildRows().getById(rowId);
    childRow.setText(rowText);

    setChildRow(childRow, "accountid", "Nucleus Account Id", eaIds.getNucleusAccountId());
    setChildRow(childRow, "originid", "Origin Persona Id", eaIds.getOriginPersonaId());
    setChildRow(childRow, "originname", "Origin Persona Name", eaIds.getOriginPersonaName());
}
void UserExtendedData::setChildRow(Pyro::UiNodeTableRow& parentRow, const char8_t* rowId, const char8_t* rowText, const Blaze::ExternalIdentifiers& externalIds)
{
    Pyro::UiNodeTableRow& childRow = parentRow.getChildRows().getById(rowId);
    childRow.setText(rowText);

    setChildRow(childRow, "xblid", "XBL Account Id", externalIds.getXblAccountId());
    setChildRow(childRow, "psnid", "PSN Account Id", externalIds.getPsnAccountId());
    setChildRow(childRow, "switchid", "Switch Id", externalIds.getSwitchId());
}

void UserExtendedData::lookupUsersCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::UserManager::UserManager::UserVector& users)
{
    if (error == Blaze::ERR_OK)
    {
        // draw the user extended data
        Pyro::UiNodeWindow &window = getWindow("UserExtendedData");
        window.setDockingState(Pyro::DockingState::DOCK_TOP);

        Pyro::UiNodeTable &table = window.getTable("User Lookup Results");
        // if the table already exists, empty out the previous lookups.
        table.getRows().clear();

        table.getColumn("user").setText("Users");
        table.getColumn("value").setText("Data");

        int curRow = 0;
        for (const auto& it : users)
        {
            Pyro::UiNodeTableRow& row = table.getRows().getById_va("%d", curRow);
            ++curRow;

            eastl::string buf = "BlazeId ";
            buf.append_sprintf("%" PRIi64, it->getId());
            row.getField("user") = buf.c_str();

            setChildRow(row, "personaname", "Persona Name", it->getName());
            setChildRow(row, "namespace", "Persona Namespace", it->getPersonaNamespace());
            setChildRow(row, "platform", "Platform", ClientPlatformTypeToString(it->getPlatformInfo().getClientPlatform()));
            setChildRow(row, "eaids", "EA Ids", it->getPlatformInfo().getEaIds());
            setChildRow(row, "externalids", "External Ids", it->getPlatformInfo().getExternalIds());
            setChildRow(row, "crossplatopt", "Cross Platform OptIn", it->getCrossPlatformOptIn());
            setChildRow(row, "isguest", "Guest User", it->isGuestUser());
            setChildRow(row, "locale", "Account Locale", it->getAccountLocale());
            setChildRow(row, "country", "Account Country", it->getAccountCountry());

            if (users.size() == 1 && curRow == 1)
                row.setCollapsed(false);
        }

    }

    REPORT_BLAZE_ERROR(error);
}

void UserExtendedData::initActionLookupUsersByBlazeId(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By BlazeId");
    action.setDescription("Looks up users by BlazeId.");

    Pyro::UiNodeParameterArrayPtr blazeIdsArray = action.getParameters().addArray("blazeIds", "Blaze Ids", "The BlazeIds of the users to look up.").getAsArray();
    Pyro::UiNodeParameter* blazeId = new Pyro::UiNodeParameter("BlazeId");
    blazeId->setText("Blaze Id");
    blazeId->setAsInt64(0);
    blazeIdsArray->setNewItemTemplate(blazeId);
}

void UserExtendedData::actionLookupUsersByBlazeId(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::UserManager::BlazeIdVector blazeIds(Blaze::MEM_GROUP_DEFAULT_TEMP, "UserExtendedData::actionLookupUsersByBlazeId");
    Pyro::UiNodeParameterArrayPtr blazeIdsArray = parameters["blazeIds"].getAsArray();
    if (blazeIdsArray != nullptr)
    {
        for (int32_t i = 0; i < (*blazeIdsArray).getCount(); ++i)
        {
            blazeIds.push_back(blazeIdsArray->at(i)->getAsInt64());
        }
    }

    Blaze::UserManager::UserManager::LookupUsersCb cb(this, &UserExtendedData::lookupUsersCb);
    gBlazeHub->getUserManager()->lookupUsersByBlazeId(&blazeIds, cb);
}

void UserExtendedData::initActionLookupUsersByPersonaNames(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By Persona Names");
    action.setDescription("Looks up users by persona names.");

    Pyro::UiNodeParameterArrayPtr personaNamesArray = action.getParameters().addArray("personaNames", "Persona Names", "The persona names of the users to look up.").getAsArray();
    Pyro::UiNodeParameter* personaName = new Pyro::UiNodeParameter("PersonaName");
    personaName->setText("Persona Name");
    personaName->setAsString("");
    personaNamesArray->setNewItemTemplate(personaName);

    action.getParameters().addBool("multiNamespace", false, "MultiNamespace", "Search for users in all namespaces");
    action.getParameters().addBool("primaryNamespaceOnly", true, "PrimaryNamespaceOnly", "Search for users in their primary namespace only  (xbox for xbl users, ps3 for psn users, etc.) Only applicable when the search includes the Origin namespace.");
    action.getParameters().addString("namespace", "cem_ea_id", "Persona Namespace", "The persona namespace to search (if MultiNamespace is false)");
}

void UserExtendedData::actionLookupUsersByPersonaNames(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    bool primaryNamespaceOnly = parameters["primaryNamespaceOnly"].getAsBool();
    bool multiNamespace = parameters["multiNamespace"].getAsBool();
    const char8_t* personaNamespace = parameters["namespace"].getAsString();
    bool lookupByOriginPersonaName = !multiNamespace && !primaryNamespaceOnly && blaze_strcmp(personaNamespace, Blaze::NAMESPACE_ORIGIN) == 0;

    Blaze::UserManager::UserManager::PersonaNameVector personaNames(Blaze::MEM_GROUP_DEFAULT_TEMP, "UserExtendedData::actionLookupUsersByPersonaName");
    Blaze::LookupUsersCrossPlatformRequest request(Blaze::MEM_GROUP_DEFAULT_TEMP, "UserExtendedData::actionLookupUsersByPersonaName");

    Pyro::UiNodeParameterArrayPtr personaNamesArray = parameters["personaNames"].getAsArray();
    if (personaNamesArray != nullptr)
    {
        if (lookupByOriginPersonaName)
            request.getPlatformInfoList().reserve((*personaNamesArray).getCount());

        for (int32_t i = 0; i < (*personaNamesArray).getCount(); ++i)
        {
            if (lookupByOriginPersonaName)
            {
                Blaze::PlatformInfoPtr platformInfo = request.getPlatformInfoList().pull_back();
                platformInfo->getEaIds().setOriginPersonaName(personaNamesArray->at(i)->getAsString());
            }
            else
                personaNames.push_back(personaNamesArray->at(i)->getAsString());
        }
    }

    Blaze::UserManager::UserManager::LookupUsersCb cb(this, &UserExtendedData::lookupUsersCb);
    if (lookupByOriginPersonaName)
    {
        request.setLookupType(Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_NAME);
        request.getLookupOpts().setBits(0);
        gBlazeHub->getUserManager()->lookupUsersCrossPlatform(request, cb);
    }
    else if (multiNamespace)
    {
        gBlazeHub->getUserManager()->lookupUsersByNameMultiNamespace(&personaNames, cb, primaryNamespaceOnly);
    }
    else
    {
        gBlazeHub->getUserManager()->lookupUsersByName(personaNamespace, &personaNames, cb);
    }
}

void UserExtendedData::initActionLookupUsersByPersonaNamePrefix(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By Persona Name Prefix");
    action.setDescription("Looks up users by persona name prefix.");

    action.getParameters().addString("prefix", "", "Persona Name Prefix", "The persona name prefix of the users to look up.");
    action.getParameters().addBool("multiNamespace", false, "MultiNamespace", "Search for users in all namespaces");
    action.getParameters().addBool("primaryNamespaceOnly", true, "PrimaryNamespaceOnly", "Search for users in their primary namespace only  (xbox for xbl users, ps3 for psn users, etc.) Only applicable when the search includes the Origin namespace.");
    action.getParameters().addString("namespace", "cem_ea_id", "Persona Namespace", "The persona namespace to search (if MultiNamespace is false)");
    action.getParameters().addUInt32("maxResultCount", 10, "Max Result Count Per Platform", "", "The maximum number of results to return per platform.", Pyro::ItemVisibility::ADVANCED);
}

void UserExtendedData::actionLookupUsersByPersonaNamePrefix(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::UserManager::LookupUsersCb cb(this, &UserExtendedData::lookupUsersCb);
    if (parameters["multiNamespace"].getAsBool())
        gBlazeHub->getUserManager()->lookupUsersByNamePrefixMultiNamespace(parameters["prefix"].getAsString(), parameters["maxResultCount"].getAsUInt32(), cb, parameters["primaryNamespaceOnly"].getAsBool());
    else
        gBlazeHub->getUserManager()->lookupUsersByNamePrefix(parameters["namespace"].getAsString(), parameters["prefix"].getAsString(), parameters["maxResultCount"].getAsUInt32(), cb, parameters["primaryNamespaceOnly"].getAsBool());
}

void UserExtendedData::setupLookupUsersCrossPlatformOptions(Pyro::UiNodeParameterStruct& parameters, const char8_t* clientPlatformOnlyIdentifier)
{
    parameters.addBool("onlineOnly", false, "OnlineOnly", "If set, lookup results will exclude offline users");
    parameters.addBool("mostRecentPlatformOnly", false, "MostRecentPlatformOnly", "If set, users will be excluded from the lookup results if there is another user linked to their Nucleus account (on a different platform) who logged in more recently");
    if (clientPlatformOnlyIdentifier != nullptr)
    {
        eastl::string buf = "If set, users will be excluded from the lookup results unless they are on the platform specified for their ";
        buf.append_sprintf("%s.", clientPlatformOnlyIdentifier);
        parameters.addBool("clientPlatformOnly", false, "ClientPlatformOnly", buf.c_str());
    }
}

void UserExtendedData::setupLookupUsersCrossPlatformCommon(Blaze::CrossPlatformLookupType lookupType, Pyro::UiNodeParameterStruct& parameters)
{
    const char8_t* idName = nullptr;
    switch (lookupType)
    {
    case Blaze::CrossPlatformLookupType::NUCLEUS_ACCOUNT_ID:
        idName = "NucleusAccountId";
        break;
    case Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_ID:
        idName = "OriginPersonaId";
        break;
    case Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_NAME:
        idName = "OriginPersonaName";
        break;
    default:
        break;
    }
    eastl::string buf;
    buf.append_sprintf("%ss", idName);
    eastl::string desc;
    desc.append_sprintf("The %s of the users to look up.", buf.c_str());

    Pyro::UiNodeParameterArrayPtr idsArray = parameters.addArray("ids", buf.c_str(), desc.c_str()).getAsArray();
    Pyro::UiNodeParameter* idStruct = new Pyro::UiNodeParameter("platformInfo");
    idStruct->getValue().setAsDataNodePtr(new Pyro::UiNodeParameterStruct());
    Pyro::UiNodeParameterStructPtr platformInfo = idStruct->getAsStruct();

    switch (lookupType)
    {
    case Blaze::CrossPlatformLookupType::NUCLEUS_ACCOUNT_ID:
        platformInfo->addInt64("nucleusAccountId", Blaze::INVALID_ACCOUNT_ID, "NucleusAccountId");
        break;
    case Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_ID:
        platformInfo->addUInt64("originPersonaId", Blaze::INVALID_ORIGIN_PERSONA_ID, "OriginPersonaId");
        break;
    case Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_NAME:
        platformInfo->addString("originPersonaName", "", "OriginPersonaName");
        break;
    default:
        break;
    }

    platformInfo->addEnum("platform", Blaze::ClientPlatformType::INVALID, "Platform");
    idsArray->setNewItemTemplate(idStruct);
    setupLookupUsersCrossPlatformOptions(parameters, idName);
}
void UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType platform, Pyro::UiNodeParameterStruct& parameters)
{
    const char8_t* idName = nullptr;
    switch (platform)
    {
    case Blaze::ClientPlatformType::pc:
        idName = "NucleusAccountId";
        break;
    case Blaze::ClientPlatformType::xone:
    case Blaze::ClientPlatformType::xbsx:
        idName = "XblAccountId";
        break;
    case Blaze::ClientPlatformType::ps4:
        idName = "PsnAccountId";
        break;
    case Blaze::ClientPlatformType::nx:
        idName = "SwitchId";
        break;
    case Blaze::ClientPlatformType::steam:
        idName = "SteamAccountId";
        break;
    case Blaze::ClientPlatformType::stadia:
        idName = "StadiaAccountId";
        break;
    default:
        break;
    }
    eastl::string buf;
    buf.append_sprintf("%ss", idName);
    eastl::string desc;
    desc.append_sprintf("The %s of the users to look up.", buf.c_str());

    Pyro::UiNodeParameterArrayPtr arr = parameters.addArray(Blaze::ClientPlatformTypeToString(platform), buf.c_str(), desc.c_str()).getAsArray();
    Pyro::UiNodeParameter* entry = new Pyro::UiNodeParameter(idName);
    entry->setText(idName);
    if (platform == Blaze::ClientPlatformType::nx)
        entry->setAsString("");
    else
        entry->setAsInt64(Blaze::INVALID_EXTERNAL_ID);
    arr->setNewItemTemplate(entry);
}

void UserExtendedData::initActionLookupUsersByNucleusAccountId(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By Nucleus Account Id");
    action.setDescription("Looks up users by Nucleus account id.");

    UserExtendedData::setupLookupUsersCrossPlatformCommon(Blaze::CrossPlatformLookupType::NUCLEUS_ACCOUNT_ID, action.getParameters());
}
void UserExtendedData::initActionLookupUsersByOriginPersonaId(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By Origin Persona Id");
    action.setDescription("Looks up users by Origin persona id.");

    UserExtendedData::setupLookupUsersCrossPlatformCommon(Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_ID, action.getParameters());
}
void UserExtendedData::initActionLookupUsersByOriginPersonaName(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By Origin Persona Name");
    action.setDescription("Looks up users by Origin persona name.");

    UserExtendedData::setupLookupUsersCrossPlatformCommon(Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_NAME, action.getParameters());
}
void UserExtendedData::initActionLookupUsersBy1stPartyId(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By 1st Party Id");
    action.setDescription("Looks up users by 1st party id.");

    UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType::pc, action.getParameters());
    UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType::ps4, action.getParameters());
    UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType::xone, action.getParameters());
    UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType::xbsx, action.getParameters());
    UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType::nx, action.getParameters());
    UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType::steam, action.getParameters());
    UserExtendedData::addParameterArrayForPlatform(Blaze::ClientPlatformType::stadia, action.getParameters());
    setupLookupUsersCrossPlatformOptions(action.getParameters(), nullptr);
}
void UserExtendedData::initActionLookupUsersByExternalId(Pyro::UiNodeAction &action)
{
    action.setText("Lookup Users By External Id");
    action.setDescription("Looks up users by External id.");

    Pyro::UiNodeParameterArrayPtr blazeIdsArray = action.getParameters().addArray("externalIds", "External Ids", "The ExternalIds of the users to look up.").getAsArray();
    Pyro::UiNodeParameter* externalId = new Pyro::UiNodeParameter("ExternalId");
    externalId->setText("External Id");
    externalId->setAsInt64(0);
    blazeIdsArray->setNewItemTemplate(externalId);
}

void UserExtendedData::actionLookupUsersByExternalId(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::UserManager::ExternalIdVector externalIds(Blaze::MEM_GROUP_DEFAULT_TEMP, "UserExtendedData::actionLookupUsersByExternalId");
    Pyro::UiNodeParameterArrayPtr externalIdsArray = parameters["externalIds"].getAsArray();
    if (externalIdsArray != nullptr)
    {
        for (int32_t i = 0; i < (*externalIdsArray).getCount(); ++i)
        {
            externalIds.push_back(externalIdsArray->at(i)->getAsInt64());
        }
    }

    Blaze::UserManager::UserManager::LookupUsersCb cb(this, &UserExtendedData::lookupUsersCb);
    gBlazeHub->getUserManager()->lookupUsersByExternalId(&externalIds, cb);
}
void UserExtendedData::doLookupUsersCrossPlatform(Blaze::CrossPlatformLookupType lookupType, Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::LookupUsersCrossPlatformRequest request(Blaze::MEM_GROUP_DEFAULT_TEMP, "UserExtendedData::doLookupUsersCrossPlatform");
    request.setLookupType(lookupType);
    request.getLookupOpts().setClientPlatformOnly(parameters["clientPlatformOnly"].getAsBool());
    request.getLookupOpts().setOnlineOnly(parameters["onlineOnly"].getAsBool());
    request.getLookupOpts().setMostRecentPlatformOnly(parameters["mostRecentPlatformOnly"].getAsBool());

    Pyro::UiNodeParameterArrayPtr idsArray = parameters["ids"].getAsArray();
    if (idsArray != nullptr)
    {
        request.getPlatformInfoList().reserve((*idsArray).getCount());
        for (int32_t i = 0; i < (*idsArray).getCount(); ++i)
        {
            Pyro::UiNodeParameterStructPtr idStruct = idsArray->at(i)->getAsStruct();
            Blaze::PlatformInfoPtr platformInfo = request.getPlatformInfoList().pull_back();
            platformInfo->setClientPlatform((*idStruct)["platform"].getAsEnum<Blaze::ClientPlatformType>());
            switch (lookupType)
            {
            case Blaze::CrossPlatformLookupType::NUCLEUS_ACCOUNT_ID:
                platformInfo->getEaIds().setNucleusAccountId((*idStruct)["nucleusAccountId"].getAsInt64());
                break;
            case Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_ID:
                platformInfo->getEaIds().setOriginPersonaId((*idStruct)["originPersonaId"].getAsUInt64());
                break;
            case Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_NAME:
                platformInfo->getEaIds().setOriginPersonaName((*idStruct)["originPersonaName"].getAsString());
                break;
            default:
                break;
            }
        }
    }

    Blaze::UserManager::UserManager::LookupUsersCb cb(this, &UserExtendedData::lookupUsersCb);
    gBlazeHub->getUserManager()->lookupUsersCrossPlatform(request, cb);
}
void UserExtendedData::actionLookupUsersByNucleusAccountId(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    UserExtendedData::doLookupUsersCrossPlatform(Blaze::CrossPlatformLookupType::NUCLEUS_ACCOUNT_ID, action, parameters);
}
void UserExtendedData::actionLookupUsersByOriginPersonaId(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    UserExtendedData::doLookupUsersCrossPlatform(Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_ID, action, parameters);
}
void UserExtendedData::actionLookupUsersByOriginPersonaName(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    UserExtendedData::doLookupUsersCrossPlatform(Blaze::CrossPlatformLookupType::ORIGIN_PERSONA_NAME, action, parameters);
}
void UserExtendedData::getPlatformInfoListForPlatform(Blaze::ClientPlatformType platform, Blaze::PlatformInfoList& platformList, Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Pyro::UiNodeParameterArrayPtr idsArray = parameters[Blaze::ClientPlatformTypeToString(platform)].getAsArray();
    if (idsArray != nullptr)
    {
        for (int32_t i = 0; i < (*idsArray).getCount(); ++i)
        {
            Blaze::PlatformInfoPtr platformInfo = platformList.pull_back();
            platformInfo->setClientPlatform(platform);
            if (platform == Blaze::ClientPlatformType::nx)
                gBlazeHub->setExternalStringOrIdIntoPlatformInfo(*platformInfo, Blaze::INVALID_EXTERNAL_ID, idsArray->at(i)->getAsString(), platform);
            else
                gBlazeHub->setExternalStringOrIdIntoPlatformInfo(*platformInfo, idsArray->at(i)->getAsInt64(), nullptr, platform);
        }
    }
}
void UserExtendedData::actionLookupUsersBy1stPartyId(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::LookupUsersCrossPlatformRequest request(Blaze::MEM_GROUP_DEFAULT_TEMP, "UserExtendedData::lookupUsersBy1stPartyId");
    request.setLookupType(Blaze::CrossPlatformLookupType::FIRST_PARTY_ID);
    request.getLookupOpts().setOnlineOnly(parameters["onlineOnly"].getAsBool());
    request.getLookupOpts().setMostRecentPlatformOnly(parameters["mostRecentPlatformOnly"].getAsBool());

    getPlatformInfoListForPlatform(Blaze::ClientPlatformType::pc, request.getPlatformInfoList(), action, parameters);
    getPlatformInfoListForPlatform(Blaze::ClientPlatformType::xone, request.getPlatformInfoList(), action, parameters);
    getPlatformInfoListForPlatform(Blaze::ClientPlatformType::xbsx, request.getPlatformInfoList(), action, parameters);
    getPlatformInfoListForPlatform(Blaze::ClientPlatformType::ps4, request.getPlatformInfoList(), action, parameters);
    getPlatformInfoListForPlatform(Blaze::ClientPlatformType::nx, request.getPlatformInfoList(), action, parameters);

    Blaze::UserManager::UserManager::LookupUsersCb cb(this, &UserExtendedData::lookupUsersCb);
    gBlazeHub->getUserManager()->lookupUsersCrossPlatform(request, cb);
}

#ifdef BLAZE_USER_PROFILE_SUPPORT
void UserExtendedData::initActionGetUserProfiles(Pyro::UiNodeAction &action)
{
    action.setText("Get UserProfiles");
    action.setDescription("Get User Profiles for the given external IDs.");

    for (int32_t i = 0; i < 4; ++i)
    {
        char8_t paramName[16];
        blaze_snzprintf(paramName, sizeof(paramName), "extId%" PRIi32, i);
        action.getParameters().addString(paramName, "", "First Party Lookup ID", "", "The external ID of the user to get profile for.");
    }

    action.getParameters().addBool("forceUpdate", false, "Force Update", "If true, the request will go to first party; otherwise, our cached values will be used for any users if available.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("avatarSize", "", "Avatar Size", "", "The avatar size of the user to get profile for.");
}

void UserExtendedData::actionGetUserProfiles(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::UserManager::FirstPartyLookupIdVector idList(Blaze::MEM_GROUP_DEFAULT_TEMP, "UserExtendedData::actionGetUserProfiles");

    for (int32_t i = 0; i < 4; ++i)
    {
        char8_t paramName[16];
        blaze_snzprintf(paramName, sizeof(paramName), "extId%" PRIi32, i);
        if (parameters[paramName].getAsString()[0] != '\0')
        {
            idList.push_back(parameters[paramName]);
        }
    }

    Blaze::UserManager::UserManager::LookupUserProfilesCb cb(this, &UserExtendedData::getUserProfilesCb);
    gBlazeHub->getUserManager()->lookupUserProfilesByFirstPartyLookupId(idList, cb, Blaze::UserManager::UserManager::USERPROFILE_TIMEOUT_MS, parameters["forceUpdate"], parameters["avatarSize"]);
}

void UserExtendedData::getUserProfileCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::UserManager::UserProfilePtr userProfile)
{
    if (error == Blaze::ERR_OK)
    {
        // draw the user profile data
        Pyro::UiNodeWindow &window = getWindow("UserProfiles");
        window.setDockingState(Pyro::DockingState::DOCK_TOP);

        Pyro::UiNodeTable &table = window.getTable("UserProfiles Lookup Results");
        // if the table already exists, empty out the previous lookups.
        table.getRows().clear();

        table.getColumn("firstPartyId").setText("First Party Lookup Id");
        table.getColumn("personaName").setText("Persona Name");
        table.getColumn("displayName").setText("Display Name");
        table.getColumn("avatarUrl").setText("Avatar Url");

        Pyro::UiNodeTableRowFieldContainer &fields = table.getRow("profile").getFields();

        fields["firstPartyId"] = userProfile->getFirstPartyLookupId();
        fields["personaName"] = userProfile->getGamerTag();
        fields["displayName"] = userProfile->getDisplayName();
        fields["avatarUrl"] = userProfile->getAvatarUrl();
    }

    REPORT_BLAZE_ERROR(error);
}

void UserExtendedData::getUserProfilesCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::UserManager::UserManager::UserProfileVector& userProfiles)
{
    // draw the user profile data
    Pyro::UiNodeWindow &window = getWindow("UserProfiles");
    window.setDockingState(Pyro::DockingState::DOCK_TOP);

    Pyro::UiNodeTable &table = window.getTable("UserProfiles Lookup Results");
    // if the table already exists, empty out the previous lookups.
    table.getRows().clear();

    table.getColumn("firstPartyId").setText("First Party Lookup Id");
    table.getColumn("personaName").setText("Persona Name");
    table.getColumn("displayName").setText("Display Name");
    table.getColumn("avatarUrl").setText("Avatar Url");

    int curRow = 0;
    Blaze::UserManager::UserManager::UserProfileVector::const_iterator itr = userProfiles.begin();
    Blaze::UserManager::UserManager::UserProfileVector::const_iterator end = userProfiles.end();
    for (; itr != end; ++itr, ++curRow)
    {
        Pyro::UiNodeTableRowFieldContainer &fields = table.getRow_va("%" PRIi32, curRow).getFields();

        Blaze::UserManager::UserProfile* userProfile = (*itr).get();

        fields["firstPartyId"] = userProfile->getFirstPartyLookupId();
        fields["personaName"] = userProfile->getGamerTag();
        fields["displayName"] = userProfile->getDisplayName();
        fields["avatarUrl"] = userProfile->getAvatarUrl();
    }

    REPORT_BLAZE_ERROR(error);
}
#endif

}
