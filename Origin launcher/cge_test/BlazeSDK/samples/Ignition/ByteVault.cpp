#include "Ignition/ByteVault.h"

#include "BlazeSDK/component/bytevault/tdf/bytevault.h"
#include "BlazeSDK/component/bytevault/tdf/bytevault_base.h"
#include "BlazeSDK/allocdefines.h"

namespace Ignition
{

ByteVault::ByteVault(uint32_t userIndex)
    : LocalUserUiBuilder("ByteVault", userIndex),
      mGetAdminParams(nullptr),
      mGetCategoryParams(nullptr),
      mGetRecordsParams(nullptr),
      mSharedAuthCredentials(new Pyro::UiNodeParameterStruct()),
      mUserIndex(userIndex)
{
    getActions().add(&getActionInitialize());
    getActions().add(&getActionSetAuthCredentials());
    getActions().add(&getActionGetAdmin());
    getActions().add(&getActionGetContexts());
    getActions().add(&getActionGetCategories());
    getActions().add(&getActionGetRecord());
    getActions().add(&getActionGetRecords());
    getActions().add(&getActionUpsertRecord());
    getActions().add(&getActionDeleteRecord());

    if (!gBlazeHub->getByteVaultManager()->isInitialized())
    {
        setIsVisibleForAll(false);
        getActionInitialize().setIsVisible(true);
        
    }
    else
    {
        setIsVisibleForAll(true);
        getActionInitialize().setIsVisible(false);
    }

    setCollapsed(true);
    mSharedAuthCredentials->addString("authToken", "");
    mSharedAuthCredentials->addEnum("tokenType", Blaze::ByteVault::NUCLEUS_AUTH_TOKEN);
    mSharedAuthCredentials->addInt64("userId", 0);
    mSharedAuthCredentials->addEnum("userType", Blaze::ByteVault::USER_TYPE_INVALID);
}

ByteVault::~ByteVault()
{

}

ByteVaultWindow& ByteVault::getByteVaultWindow(const char8_t* windowId)
{
    ByteVaultWindow *window = static_cast<ByteVaultWindow *>(getWindows().findById(windowId));
    if (window == nullptr)
    {
        window = new ByteVaultWindow(windowId);
        window->UserClosing += Pyro::UiNodeWindow::UserClosingEventHandler(this, &ByteVault::ByteVaultWindowClosing);

        getWindows().add(window);
    }

    return *window;
}


ByteVaultWindow& ByteVault::getAdminWindow()
{
    return getByteVaultWindow("Get Admin");
}

ByteVaultWindow& ByteVault::getContextWindow()
{
    return getByteVaultWindow("Get Contexts");
}

ByteVaultWindow& ByteVault::getCategoryWindow()
{
    return getByteVaultWindow("Get Categories");
}
ByteVaultWindow& ByteVault::getRecordWindow()
{
    return getByteVaultWindow("Get Record");
}

ByteVaultWindow& ByteVault::getRecordsInfoWindow()
{
    return getByteVaultWindow("Get Records Info");
}


void ByteVault::onConnected()
{
}

void ByteVault::onDisconnected()
{
}

void ByteVault::ByteVaultWindowClosing(Pyro::UiNodeWindow *sender)
{
    ByteVaultWindow *byteVaultWindow = (ByteVaultWindow *)sender;
    getWindows().remove(byteVaultWindow);
}

void ByteVault::initActionInitialize(Pyro::UiNodeAction &action)
{
    action.setText("Initialize");
    action.setDescription("Initializes ByteVault");

    action.getParameters().addString("host", "", "Hostname", "", "The Bytevault server hostname");
    action.getParameters().addUInt16("port", 0, "Port", "", "The Bytevault server port");
    action.getParameters().addBool("secure", false, "Secure connection", "Whether to connect securely to Bytevault");
}

void ByteVault::actionInitialize(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    // Leave the hostname/port empty as we can get it from the ByteVault configuration section
    
    Blaze::BlazeError err = gBlazeHub->getByteVaultManager()->initialize(parameters["host"], parameters["port"], parameters["secure"]);
    if (err != Blaze::ERR_OK)
    {
        if (err != Blaze::BYTEVAULT_ALREADY_INITIALIZED)
        {
            REPORT_BLAZE_ERROR(err);
            return;
        }
    }

    setIsVisibleForAll(true);
    getActionInitialize().setIsVisible(false);
}

void ByteVault::initActionGetAdmin(Pyro::UiNodeAction &action)
{
    action.setText("Get Admin");
    action.setDescription("Gets a ByteVault admin");

    action.getParameters().addString("adminEmail", "", "Admin Email", "", "The email address of the admin to get");
}

void ByteVault::actionGetAdmin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    mGetAdminParams = parameters;

    Blaze::ByteVault::GetAdminRequest req;
    req.setAdminEmail(parameters["adminEmail"]);

    setAuthCredentials(req.getAuthCredentials());

    gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->getAdmin(req, Blaze::MakeFunctor(this, &ByteVault::GetAdminCb));
}

void ByteVault::GetAdminCb(const Blaze::ByteVault::GetAdminResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);
    if (blazeError == Blaze::ERR_OK)
    {
        ByteVaultWindow& window = getAdminWindow();
        Pyro::UiNodeTable& contextTable = window.getTable("ContextList");
        Pyro::UiNodeTable& adminTypeTable = window.getTable("AdminTypeList");

        Pyro::UiNodeTableRow::Enumerator enumerator(contextTable.getRows());
        while (enumerator.moveNext())
        {
            Blaze::ByteVault::AdminTypeList* adminTypeList = reinterpret_cast<Blaze::ByteVault::AdminTypeList*>(enumerator.getCurrent()->getContextObject());
            adminTypeList->Release();
        }

        contextTable.getRows().clear();
        adminTypeTable.getRows().clear();

        contextTable.RowSelected += Pyro::UiNodeTable::RowSelectedEventHandler(this, &ByteVault::ContextAdminListRowSelected);
        contextTable.RowUnselected += Pyro::UiNodeTable::RowUnselectedEventHandler(this, &ByteVault::ContextAdminListRowSelected);

        contextTable.setPosition(Pyro::ControlPosition::TOP_LEFT);
        adminTypeTable.setPosition(Pyro::ControlPosition::TOP_MIDDLE);

        contextTable.getColumn("ContextList").setText("Context");
        adminTypeTable.getColumn("AdminTypeList").setText("Admin Type List");

        Blaze::ByteVault::AdminTypeListByContextMap::const_iterator iter = response->getAdminTypeListByContextMap().begin();
        Blaze::ByteVault::AdminTypeListByContextMap::const_iterator end = response->getAdminTypeListByContextMap().end();
        for (; iter != end; ++iter)
        {
            Pyro::UiNodeTableRow& row = contextTable.getRow(iter->first.c_str());
            Blaze::ByteVault::AdminTypeList * adminTypeList = new Blaze::ByteVault::AdminTypeList;
            iter->second->copyInto(*adminTypeList);
            row.setContextObject(adminTypeList);
        }
    }
}

void ByteVault::ContextAdminListRowSelected(Pyro::UiNodeTable* table, Pyro::UiNodeTableRow* row)
{
    ByteVaultWindow& window = getAdminWindow();
    Pyro::UiNodeTable& adminTypeTable = window.getTable("AdminTypeList");

    if (row == nullptr || !row->getIsSelected())
    {
        adminTypeTable.getRows().clear();
    }
    else
    {
        Blaze::ByteVault::AdminTypeList* adminTypeList = reinterpret_cast<Blaze::ByteVault::AdminTypeList*>(row->getContextObject());

        Blaze::ByteVault::AdminTypeList::const_iterator adminItr = adminTypeList->begin();
        Blaze::ByteVault::AdminTypeList::const_iterator adminEnd = adminTypeList->end();

        for (; adminItr != adminEnd; ++adminItr)
        {
            adminTypeTable.getRow(Blaze::ByteVault::AdminTypeToString(*adminItr));
        }
    }
}

void ByteVault::initActionGetContexts(Pyro::UiNodeAction &action)
{
    action.setText("Get Contexts");
    action.setDescription("Gets a list of ByteVault contexts");

    action.getParameters().addString("context", "", "Context", "", "A specific context to fetch");
    action.getParameters().addUInt32("maxResults", 50, "Max Result Count", "", "The maximum number of results to return");
    action.getParameters().addUInt32("offset", 0, "Offset", "", "The number of records to skip before retrieving results");
}

void ByteVault::actionGetContexts(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::ByteVault::GetContextsRequest req;
    req.setMaxResultCount(parameters["maxResults"]);
    req.setOffset(parameters["offset"]);
    req.setContext(parameters["context"]);

    setAuthCredentials(req.getAuthCredentials());

    if (req.getContext()[0] == '\0')
        gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->getContexts(req, Blaze::MakeFunctor(this, &ByteVault::GetContextsCb));
    else
        gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->getContext(req, Blaze::MakeFunctor(this, &ByteVault::GetContextCb));
}

void ByteVault::GetContextCb(const Blaze::ByteVault::Context* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError == Blaze::ERR_OK)
    {
        ByteVaultWindow& window = getContextWindow();
        Pyro::UiNodeTable& tableNode = window.getTable("Contexts");
        Pyro::UiNodeTable& recordCountNode = window.getTable("Record Counts by Category");

        Pyro::UiNodeTableRow::Enumerator enumerator(tableNode.getRows());
        while (enumerator.moveNext())
        {
            Blaze::ByteVault::Context* context = reinterpret_cast<Blaze::ByteVault::Context*>(enumerator.getCurrent()->getContextObject());
            context->Release();
        }
        tableNode.getRows().clear();
        recordCountNode.getRows().clear();

        tableNode.setPosition(Pyro::ControlPosition::TOP_MIDDLE);
        tableNode.RowSelected += Pyro::UiNodeTable::RowSelectedEventHandler(this, &ByteVault::ContextRowSelected);
        tableNode.RowUnselected += Pyro::UiNodeTable::RowUnselectedEventHandler(this, &ByteVault::ContextRowSelected);

        recordCountNode.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);

        tableNode.getColumn("Context").setText("Context");
        tableNode.getColumn("Label").setText("Label");
        tableNode.getColumn("Description").setText("Description");
        tableNode.getColumn("Version").setText("Version");
        tableNode.getColumn("ActiveRecordNum").setText("Number of Active Records");
        tableNode.getColumn("DeletedRecordNum").setText("Number of Deleted Records");

        recordCountNode.getColumn("Category").setText("Category Name");
        recordCountNode.getColumn("ActiveCount").setText("Active Record Count");
        recordCountNode.getColumn("DeletedCount").setText("Deleted Record Count");

        const Blaze::ByteVault::Context& context = *response;
        Pyro::UiNodeTableRow& row = tableNode.getRow(context.getName());
        Pyro::UiNodeTableRowFieldContainer& fields = row.getFields();
        row.setContextObject(context.clone());

        fields["Label"] = context.getLabel();
        fields["Description"] = context.getDescription();
        fields["Version"] = context.getVersion();
        fields["ActiveRecordNum"] = context.getNumOfRecords();
        fields["DeletedRecordNum"] = context.getNumOfDeletedRecords();
    }
}

void ByteVault::GetContextsCb(const Blaze::ByteVault::GetContextsResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError == Blaze::ERR_OK)
    {
        ByteVaultWindow& window = getContextWindow();
        Pyro::UiNodeTable& tableNode = window.getTable("Contexts");
        Pyro::UiNodeTable& recordCountNode = window.getTable("Record Counts by Category");

        Pyro::UiNodeTableRow::Enumerator enumerator(tableNode.getRows());
        while (enumerator.moveNext())
        {
            Blaze::ByteVault::Context* context = reinterpret_cast<Blaze::ByteVault::Context*>(enumerator.getCurrent()->getContextObject());
            context->Release();
        }
        tableNode.getRows().clear();
        recordCountNode.getRows().clear();

        tableNode.setPosition(Pyro::ControlPosition::TOP_MIDDLE);
        tableNode.RowSelected += Pyro::UiNodeTable::RowSelectedEventHandler(this, &ByteVault::ContextRowSelected);
        tableNode.RowUnselected += Pyro::UiNodeTable::RowUnselectedEventHandler(this, &ByteVault::ContextRowSelected);

        recordCountNode.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);

        tableNode.getColumn("Context").setText("Context");
        tableNode.getColumn("Label").setText("Label");
        tableNode.getColumn("Description").setText("Description");
        tableNode.getColumn("Version").setText("Version");
        tableNode.getColumn("ActiveRecordNum").setText("Number of Active Records");
        tableNode.getColumn("DeletedRecordNum").setText("Number of Deleted Records");

        recordCountNode.getColumn("Category").setText("Category Name");
        recordCountNode.getColumn("ActiveCount").setText("Active Record Count");
        recordCountNode.getColumn("DeletedCount").setText("Deleted Record Count");

        Blaze::ByteVault::GetContextsResponse::ContextList::const_iterator iter = response->getContexts().begin();
        Blaze::ByteVault::GetContextsResponse::ContextList::const_iterator end = response->getContexts().end();
        for (; iter != end; ++iter)
        {
            const Blaze::ByteVault::Context& context = **iter;
            Pyro::UiNodeTableRow& row = tableNode.getRow(context.getName());
            Pyro::UiNodeTableRowFieldContainer& fields = row.getFields();
            row.setContextObject(context.clone());

            fields["Label"] = context.getLabel();
            fields["Description"] = context.getDescription();
            fields["Version"] = context.getVersion();
            fields["ActiveRecordNum"] = context.getNumOfRecords();
            fields["DeletedRecordNum"] = context.getNumOfDeletedRecords();
        }
    }
}

void ByteVault::ContextRowSelected(Pyro::UiNodeTable* table, Pyro::UiNodeTableRow* row)
{
    ByteVaultWindow& window = getContextWindow();
    Pyro::UiNodeTable& recordCountNode = window.getTable("Record Counts by Category");

    if (row == nullptr || !row->getIsSelected())
    {
        recordCountNode.getRows().clear();
    }
    else
    {
        Blaze::ByteVault::Context* context = reinterpret_cast<Blaze::ByteVault::Context*>(row->getContextObject());

        Blaze::ByteVault::Context::RecordCountByCategoryMap::const_iterator recordIter = context->getRecordCountByCategory().begin();
        Blaze::ByteVault::Context::RecordCountByCategoryMap::const_iterator recordEnd = context->getRecordCountByCategory().end();
        for (; recordIter != recordEnd; ++recordIter)
        {
            Pyro::UiNodeTableRow& recordRow = recordCountNode.getRow(recordIter->first.c_str());
            recordRow.getFields()["ActiveCount"] = recordIter->second;
        }
        Blaze::ByteVault::Context::DeletedRecordCountByCategoryMap::const_iterator delRecordIter = context->getDeletedRecordCountByCategory().begin();
        Blaze::ByteVault::Context::DeletedRecordCountByCategoryMap::const_iterator delRecordEnd = context->getDeletedRecordCountByCategory().end();
        for (; delRecordIter != delRecordEnd; ++delRecordIter)
        {
            Pyro::UiNodeTableRow& recordRow = recordCountNode.getRow(delRecordIter->first.c_str());
            recordRow.getFields()["DeletedCount"] = delRecordIter->second;
        }
    }
}

void ByteVault::initActionGetCategories(Pyro::UiNodeAction &action)
{
    action.setText("Get Categories");
    action.setDescription("Gets a list of ByteVault categories for a specific context");

    action.getParameters().addString("context", "", "Context", "", "The name of the context to fetch categories from");
    action.getParameters().addString("category", "", "Category", "", "The specific category to fetch");
    action.getParameters().addUInt32("maxResults", 50, "Max Result Count", "", "The maximum number of results to return");
    action.getParameters().addUInt32("offset", 0, "Offset", "", "The number of records to skip before retrieving results");
}

void ByteVault::actionGetCategories(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    mGetCategoryParams = parameters;

    Blaze::ByteVault::GetCategoriesRequest req;
    req.setContext(parameters["context"]);
    req.setMaxResultCount(parameters["maxResults"]);
    req.setOffset(parameters["offset"]);
    req.setCategoryName(parameters["category"]);

    setAuthCredentials(req.getAuthCredentials());

    if (req.getCategoryName()[0] == '\0')
        gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->getCategories(req, Blaze::MakeFunctor(this, &ByteVault::GetCategoriesCb));
    else
        gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->getCategory(req, Blaze::MakeFunctor(this, &ByteVault::GetCategoryCb));
}

void ByteVault::GetCategoryCb(const Blaze::ByteVault::Category* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError == Blaze::ERR_OK)
    {
        ByteVaultWindow& window = getCategoryWindow();
        Pyro::UiNodeTable& tableNode = window.getTable_va("Categories for context %s", mGetCategoryParams["context"].getAsString());
        Pyro::UiNodeUiGroup &detailsGroup = window.getUiGroup("Category Details");
        Pyro::UiNodeTable& trustedNode = detailsGroup.getTable("Trusted Sources");
        Pyro::UiNodeTable& recordNode = detailsGroup.getTable("Editable Record Names");
        Pyro::UiNodeUiGroup& countNodeGroup = detailsGroup.getUiGroup("Record counts by name");
        Pyro::UiNodeTable& activeCountNode = countNodeGroup.getTable("Active Record counts by name");
        Pyro::UiNodeTable& deletedCountNode = countNodeGroup.getTable("Deleted Record counts by name");

        Pyro::UiNodeTableRow::Enumerator enumerator(tableNode.getRows());
        while (enumerator.moveNext())
        {
            Blaze::ByteVault::Category* category = reinterpret_cast<Blaze::ByteVault::Category*>(enumerator.getCurrent()->getContextObject());
            category->Release();
        }

        tableNode.getRows().clear();
        trustedNode.getRows().clear();
        recordNode.getRows().clear();
        activeCountNode.getRows().clear();
        deletedCountNode.getRows().clear();

        tableNode.setPosition(Pyro::ControlPosition::TOP_MIDDLE);
        tableNode.RowSelected += Pyro::UiNodeTable::RowSelectedEventHandler(this, &ByteVault::CategoryRowSelected);
        tableNode.RowUnselected += Pyro::UiNodeTable::RowUnselectedEventHandler(this, &ByteVault::CategoryRowSelected);

        detailsGroup.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);
        trustedNode.setPosition(Pyro::ControlPosition::BOTTOM_LEFT);
        recordNode.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);

        countNodeGroup.setPosition(Pyro::ControlPosition::BOTTOM_RIGHT);
        activeCountNode.setPosition(Pyro::ControlPosition::TOP_MIDDLE);
        deletedCountNode.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);

        tableNode.getColumn("Category").setText("Category");
        tableNode.getColumn("Description").setText("Description");
        tableNode.getColumn("Created").setText("Creation Time");
        tableNode.getColumn("Updated").setText("Last Updated");
        tableNode.getColumn("Version").setText("Version");
        tableNode.getColumn("RecordNum").setText("Number of Records");
        tableNode.getColumn("MaxRecords").setText("Maximum records per user");
        tableNode.getColumn("MaxPayload").setText("Maximum record payload size");

        trustedNode.getColumn("source").setText("Source");

        recordNode.getColumn("RecordName").setText("Record name");

        activeCountNode.getColumn("RecordName").setText("Record name");
        activeCountNode.getColumn("Count").setText("Count");

        deletedCountNode.getColumn("RecordName").setText("Record name");
        deletedCountNode.getColumn("Count").setText("Count");

        const Blaze::ByteVault::Category& category = *response;
        Pyro::UiNodeTableRow& row = tableNode.getRow(category.getCategoryName());
        Pyro::UiNodeTableRowFieldContainer& fields = row.getFields();
        row.setContextObject(category.clone());

        fields["Description"] = category.getDescription();
        fields["Created"] = category.getCreationTime().getMicroSeconds();
        fields["Updated"] = category.getLastUpdateTime().getMicroSeconds();
        fields["Version"] = category.getConfiguration().getVersion();
        fields["ActiveRecordNum"] = category.getNumOfRecords();
        fields["DeletedRecordNum"] = category.getNumOfDeletedRecords();
        fields["MaxRecords"] = category.getConfiguration().getMaxRecordsPerUser();
        fields["MaxPayload"] = category.getConfiguration().getMaxRecordPayloadSize();
    }
}

void ByteVault::GetCategoriesCb(const Blaze::ByteVault::GetCategoriesResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError == Blaze::ERR_OK)
    {
        ByteVaultWindow& window = getCategoryWindow();
        Pyro::UiNodeTable& tableNode = window.getTable_va("Categories for context %s", mGetCategoryParams["context"].getAsString());
        Pyro::UiNodeUiGroup &detailsGroup = window.getUiGroup("Category Details");
        Pyro::UiNodeTable& trustedNode = detailsGroup.getTable("Trusted Sources");
        Pyro::UiNodeTable& recordNode = detailsGroup.getTable("Editable Record Names");
        Pyro::UiNodeUiGroup& countNodeGroup = detailsGroup.getUiGroup("Record counts by name");
        Pyro::UiNodeTable& activeCountNode = countNodeGroup.getTable("Active Record counts by name");
        Pyro::UiNodeTable& deletedCountNode = countNodeGroup.getTable("Deleted Record counts by name");

        Pyro::UiNodeTableRow::Enumerator enumerator(tableNode.getRows());
        while (enumerator.moveNext())
        {
            Blaze::ByteVault::Category* category = reinterpret_cast<Blaze::ByteVault::Category*>(enumerator.getCurrent()->getContextObject());
            category->Release();
        }

        tableNode.getRows().clear();
        trustedNode.getRows().clear();
        recordNode.getRows().clear();
        activeCountNode.getRows().clear();
        deletedCountNode.getRows().clear();

        tableNode.setPosition(Pyro::ControlPosition::TOP_MIDDLE);
        tableNode.RowSelected += Pyro::UiNodeTable::RowSelectedEventHandler(this, &ByteVault::CategoryRowSelected);
        tableNode.RowUnselected += Pyro::UiNodeTable::RowUnselectedEventHandler(this, &ByteVault::CategoryRowSelected);

        detailsGroup.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);
        trustedNode.setPosition(Pyro::ControlPosition::BOTTOM_LEFT);
        recordNode.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);

        countNodeGroup.setPosition(Pyro::ControlPosition::BOTTOM_RIGHT);
        activeCountNode.setPosition(Pyro::ControlPosition::TOP_MIDDLE);
        deletedCountNode.setPosition(Pyro::ControlPosition::BOTTOM_MIDDLE);

        tableNode.getColumn("Category").setText("Category");
        tableNode.getColumn("Description").setText("Description");
        tableNode.getColumn("Created").setText("Creation Time");
        tableNode.getColumn("Updated").setText("Last Updated");
        tableNode.getColumn("Version").setText("Version");
        tableNode.getColumn("RecordNum").setText("Number of Records");
        tableNode.getColumn("MaxRecords").setText("Maximum records per user");
        tableNode.getColumn("MaxPayload").setText("Maximum record payload size");

        trustedNode.getColumn("source").setText("Source");

        recordNode.getColumn("RecordName").setText("Record name");

        activeCountNode.getColumn("RecordName").setText("Record name");
        activeCountNode.getColumn("Count").setText("Count");

        deletedCountNode.getColumn("RecordName").setText("Record name");
        deletedCountNode.getColumn("Count").setText("Count");

        Blaze::ByteVault::GetCategoriesResponse::CategoryList::const_iterator iter = response->getCategories().begin();
        Blaze::ByteVault::GetCategoriesResponse::CategoryList::const_iterator end = response->getCategories().end();
        for (; iter != end; ++iter)
        {
            const Blaze::ByteVault::Category& category = **iter;
            Pyro::UiNodeTableRow& row = tableNode.getRow(category.getCategoryName());
            Pyro::UiNodeTableRowFieldContainer& fields = row.getFields();
            row.setContextObject(category.clone());

            fields["Description"] = category.getDescription();
            fields["Created"] = category.getCreationTime().getMicroSeconds();
            fields["Updated"] = category.getLastUpdateTime().getMicroSeconds();
            fields["Version"] = category.getConfiguration().getVersion();
            fields["ActiveRecordNum"] = category.getNumOfRecords();
            fields["DeletedRecordNum"] = category.getNumOfDeletedRecords();
            fields["MaxRecords"] = category.getConfiguration().getMaxRecordsPerUser();
            fields["MaxPayload"] = category.getConfiguration().getMaxRecordPayloadSize();
        }
    }
}

void ByteVault::CategoryRowSelected(Pyro::UiNodeTable* table, Pyro::UiNodeTableRow* row)
{
    ByteVaultWindow& window = getCategoryWindow();
    Pyro::UiNodeUiGroup &detailsGroup = window.getUiGroup("Category Details");
    Pyro::UiNodeTable& trustedNode = detailsGroup.getTable("Trusted Sources");
    Pyro::UiNodeTable& recordNode = detailsGroup.getTable("Editable Record Names");
    Pyro::UiNodeUiGroup& countNodeGroup = detailsGroup.getUiGroup("Record counts by name");
    Pyro::UiNodeTable& activeCountNode = countNodeGroup.getTable("Active Record counts by name");
    Pyro::UiNodeTable& deletedCountNode = countNodeGroup.getTable("Deleted Record counts by name");

    if (row == nullptr || !row->getIsSelected())
    {
        trustedNode.getRows().clear();
        recordNode.getRows().clear();
        activeCountNode.getRows().clear();
        deletedCountNode.getRows().clear();
    }
    else
    {
        Blaze::ByteVault::Category* category = reinterpret_cast<Blaze::ByteVault::Category*>(row->getContextObject());

        Blaze::ByteVault::CategorySettings::StringRecordNameList::const_iterator recordNameIter = category->getConfiguration().getEditableRecordNames().begin();
        Blaze::ByteVault::CategorySettings::StringRecordNameList::const_iterator recordNameEnd = category->getConfiguration().getEditableRecordNames().end();
        for (; recordNameIter != recordNameEnd; ++recordNameIter)
        {
            recordNode.getRow(recordNameIter->c_str());
        }

        Blaze::NetworkFilterConfig::const_iterator trustedSourcesIter = category->getTrustedSources().begin();
        Blaze::NetworkFilterConfig::const_iterator trustedSourcesEnd = category->getTrustedSources().end();
        for (; trustedSourcesIter != trustedSourcesEnd; ++trustedSourcesIter)
        {
            trustedNode.getRow(trustedSourcesIter->c_str());
        }

        Blaze::ByteVault::Category::RecordCountByNameMap::const_iterator countIter = category->getRecordCountByName().begin();
        Blaze::ByteVault::Category::RecordCountByNameMap::const_iterator countEnd = category->getRecordCountByName().end();
        for (; countIter != countEnd; ++countIter)
        {
            Pyro::UiNodeTableRow& countRow = activeCountNode.getRow(countIter->first.c_str());
            countRow.getFields()["RecordName"] = countIter->first.c_str();
            countRow.getFields()["Count"] = countIter->second;
        }

        Blaze::ByteVault::Category::DeletedRecordCountByNameMap::const_iterator delCountIter = category->getDeletedRecordCountByName().begin();
        Blaze::ByteVault::Category::DeletedRecordCountByNameMap::const_iterator delCountEnd = category->getDeletedRecordCountByName().end();
        for (; delCountIter != delCountEnd; ++delCountIter)
        {
            Pyro::UiNodeTableRow& countRow = deletedCountNode.getRow(delCountIter->first.c_str());
            countRow.getFields()["RecordName"] = delCountIter->first.c_str();
            countRow.getFields()["Count"] = delCountIter->second;
        }
    }
}

void ByteVault::initActionGetRecord(Pyro::UiNodeAction &action)
{
    action.setText("Get Record");
    action.setDescription("Gets a record based on the specified record address(User Id, User Type and Record Name)");

    action.getParameters().addString("category", "", "Category", "", "The category the record belongs to");
    action.getParameters().addString("context", "", "Context", "", "The context the record and category belong to");
    action.getParameters().addString("record", "", "Record name", "", "The name of the record to look up");
    action.getParameters().addInt64("id", 0, "Owner ID", "", "The ID of the record's owner");
    action.getParameters().addEnum("type", Blaze::ByteVault::USER_TYPE_INVALID, "Owner Type", "The type of user ID");
    action.getParameters().addString("subrecord", "", "Sub-record name", "", "The name of the sub-record to look up");
}

void ByteVault::actionGetRecord(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    mGetRecordsParams = parameters;

    Blaze::ByteVault::GetRecordRequest req;
    req.getRecordAddress().setCategoryName(parameters["category"]);
    req.getRecordAddress().setContext(parameters["context"]);
    req.getRecordAddress().setRecordName(parameters["record"]);
    req.getRecordAddress().getOwner().setId(parameters["id"]);
    req.getRecordAddress().getOwner().setType(parameters["type"]);
    req.setSubrecord(parameters["subrecord"]);

    setAuthCredentials(req.getAuthCredentials());

    gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->getRecord(req, Blaze::MakeFunctor(this, &ByteVault::GetRecordCb));
}

void ByteVault::GetRecordCb(const Blaze::ByteVault::GetRecordResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError == Blaze::ERR_OK)
    {
        ByteVaultWindow& window = getRecordWindow();
        Pyro::UiNodeTable& tableNode = window.getTable_va("Record for category %s/%s", mGetRecordsParams["context"].getAsString(), mGetRecordsParams["category"].getAsString());
        tableNode.getRows().clear();

        tableNode.getColumn("Record").setText("Record name");
        tableNode.getColumn("Type").setText("Payload Content Type");
        tableNode.getColumn("Created").setText("Creation Time");
        tableNode.getColumn("Updated").setText("Last Updated");

        Pyro::UiNodeTableRow& row = tableNode.getRow(mGetRecordsParams["record"].getAsString());
        Pyro::UiNodeTableRowFieldContainer& fields = row.getFields();
        fields["Type"] = response->getPayload().getContentType();
        fields["Created"] = response->getCreation().getMicroSeconds();
        fields["Updated"] = response->getLastUpdate().getMicroSeconds();
    }
}

void ByteVault::initActionGetRecords(Pyro::UiNodeAction &action)
{
    action.setText("Get Record Info");
    action.setDescription("Gets a record by its owner Id and name");

    action.getParameters().addString("category", "", "Category", "", "The category the record belongs to");
    action.getParameters().addString("context", "", "Context", "", "The context the record and category belong to");
    action.getParameters().addString("record", "", "Record name", "", "The name of the record to look up");
    action.getParameters().addInt64("id", 0, "Owner ID", "", "The ID of the record's owner");
    action.getParameters().addEnum("type", Blaze::ByteVault::USER_TYPE_INVALID, "Owner Type", "The type of user ID");
    action.getParameters().addUInt32("maxresults", 0, "Maximum results", "", "The maximum results to return");
    action.getParameters().addUInt32("offset", 0, "Offset", "", "The maximum results to return");
}

void ByteVault::actionGetRecords(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    mGetRecordsParams = parameters;

    Blaze::ByteVault::GetRecordInfoRequest req;
    req.getRecordAddress().setCategoryName(parameters["category"]);
    req.getRecordAddress().setContext(parameters["context"]);
    req.getRecordAddress().setRecordName(parameters["record"]);
    req.getRecordAddress().getOwner().setId(parameters["id"]);
    req.getRecordAddress().getOwner().setType(parameters["type"]);
    req.setMaxResultCount(parameters["maxresults"]);
    req.setOffset(parameters["offset"]);

    setAuthCredentials(req.getAuthCredentials());

    gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->getRecordsInfo(req, Blaze::MakeFunctor(this, &ByteVault::GetRecordsInfoCb));
}

void ByteVault::GetRecordsInfoCb(const Blaze::ByteVault::GetRecordInfoResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);

    if (blazeError == Blaze::ERR_OK)
    {
        ByteVaultWindow& window = getRecordsInfoWindow();
        Pyro::UiNodeTable& tableNode = window.getTable_va("Records for category %s/%s", mGetRecordsParams["context"].getAsString(), mGetRecordsParams["category"].getAsString());
        tableNode.getRows().clear();

        tableNode.getColumn("Result No").setText("No.");
        tableNode.getColumn("Record").setText("Record name");
        tableNode.getColumn("Id").setText("Owner ID");
        tableNode.getColumn("Type").setText("Owner Type");
        tableNode.getColumn("Created").setText("Creation Time");
        tableNode.getColumn("Updated").setText("Last Updated");

        Blaze::ByteVault::GetRecordInfoResponse::RecordInfoList::const_iterator iter = response->getRecords().begin();
        Blaze::ByteVault::GetRecordInfoResponse::RecordInfoList::const_iterator end = response->getRecords().end();
        uint32_t resultIndex = 0;
        for (; iter != end; ++iter)
        {
            const Blaze::ByteVault::RecordInfo& info = **iter;
            Pyro::UiNodeTableRow& row = tableNode.getRow_va("%u",++resultIndex);
            Pyro::UiNodeTableRowFieldContainer& fields = row.getFields();
            fields["Record"] = info.getRecordName();
            fields["Id"] = info.getOwner().getId();
            fields["Type"] = info.getOwner().getType();
            fields["Created"] = info.getCreationTime().getMicroSeconds();
            fields["Updated"] = info.getLastUpdateTime().getMicroSeconds();
        }
    }
}

void ByteVault::initActionUpsertRecord(Pyro::UiNodeAction &action)
{
    action.setText("Create/Update Record");
    action.setDescription("Gets a record by either owner Id or name");

    action.getParameters().addString("category", "", "Category", "", "The category the record belongs to");
    action.getParameters().addString("context", "", "Context", "", "The context the record and category belong to");
    action.getParameters().addString("record", "", "Record name", "", "The name of the record to look up");
    action.getParameters().addInt64("id", 0, "Owner ID", "", "The ID of the record's owner");
    action.getParameters().addEnum("type", Blaze::ByteVault::USER_TYPE_INVALID, "Owner Type", "The type of user ID");
    action.getParameters().addString("content", "", "Content Type", "The content type of the payload data");
    action.getParameters().addString("data", "", "Payload data", "The record payload to include");
    action.getParameters().addBool("subrecordupdate", false, "Sub-record update", "Flag to indicate if the record is to be updated (or replaced)");
}

void ByteVault::actionUpsertRecord(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::ByteVault::UpsertRecordRequest req;
    req.getRecordAddress().setCategoryName(parameters["category"]);
    req.getRecordAddress().setContext(parameters["context"]);
    req.getRecordAddress().setRecordName(parameters["record"]);
    req.getRecordAddress().getOwner().setId(parameters["id"]);
    req.getRecordAddress().getOwner().setType(parameters["type"]);

    const char8_t* data = parameters["data"];
    req.getPayload().setContentType(parameters["content"]);
    req.getPayload().getBlob().setData((const uint8_t *) data, (EA::TDF::TdfSizeVal)strlen(data));
    req.setSubrecordUpdate(parameters["subrecordupdate"]);

    setAuthCredentials(req.getAuthCredentials());

    gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->upsertRecord(req, Blaze::MakeFunctor(this, &ByteVault::UpsertRecordCb));
}

void ByteVault::UpsertRecordCb(const Blaze::ByteVault::UpsertRecordResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);
}

void ByteVault::initActionDeleteRecord(Pyro::UiNodeAction &action)
{
    action.setText("Delete Record");
    action.setDescription("Delete a record by name and owner Id");

    action.getParameters().addString("category", "", "Category", "", "The category the record belongs to");
    action.getParameters().addString("context", "", "Context", "", "The context the record and category belong to");
    action.getParameters().addString("record", "", "Record name", "", "The name of the record to look up");
    action.getParameters().addInt64("id", 0, "Owner ID", "", "The ID of the record's owner");
    action.getParameters().addEnum("type", Blaze::ByteVault::USER_TYPE_INVALID, "Owner Type", "The type of user ID");
}

void ByteVault::actionDeleteRecord(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::ByteVault::DeleteRecordRequest req;
    req.getRecordAddress().setCategoryName(parameters["category"]);
    req.getRecordAddress().setContext(parameters["context"]);
    req.getRecordAddress().setRecordName(parameters["record"]);
    req.getRecordAddress().getOwner().setId(parameters["id"]);
    req.getRecordAddress().getOwner().setType(parameters["type"]);

    setAuthCredentials(req.getAuthCredentials());

    gBlazeHub->getByteVaultManager()->getComponent(mUserIndex)->deleteRecord(req);
}

void ByteVault::initActionSetAuthCredentials(Pyro::UiNodeAction &action)
{
    getSharedAuthCredentials(action.getParameters());
}

void ByteVault::actionSetAuthCredentials(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    setSharedAuthCredentials(parameters);
}

void ByteVault::setSharedAuthCredentials(Pyro::UiNodeParameterStructPtr parameters)
{
    mSharedAuthCredentials->addString("authToken", parameters["authToken"]);
    mSharedAuthCredentials->addEnum("tokenType", Blaze::ByteVault::NUCLEUS_AUTH_TOKEN);
    mSharedAuthCredentials->addInt64("userId", parameters["userId"]);
    mSharedAuthCredentials->addEnum<Blaze::ByteVault::UserType>("userType", parameters["userType"]);
}

void ByteVault::getSharedAuthCredentials(Pyro::UiNodeParameterStruct& parameters)
{
    parameters.addString("authToken", mSharedAuthCredentials["authToken"], "Authentication Token", "", "The auth token of the account authenticating.");
    parameters.addInt64("userId", mSharedAuthCredentials["userId"], "User ID", "", "The User ID of the user authenticating.");
    parameters.addEnum<Blaze::ByteVault::UserType>("userType", mSharedAuthCredentials["userType"], "User Type", "The type of the user authenticating.");
}

void ByteVault::setAuthCredentials(Blaze::ByteVault::AuthenticationCredentials& credentials)
{
    credentials.setToken(mSharedAuthCredentials["authToken"]);
    credentials.setTokenType(mSharedAuthCredentials["tokenType"]);
    credentials.getUser().setId(mSharedAuthCredentials["userId"]);
    credentials.getUser().setType(mSharedAuthCredentials["userType"]);
}

ByteVaultWindow::ByteVaultWindow(const char8_t* windowId)
    : Pyro::UiNodeWindow(windowId)
{

}

ByteVaultWindow::~ByteVaultWindow()
{

}

} // namespace Ignition
