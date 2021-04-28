
#include "Ignition/Achievements.h"


namespace Ignition
{

const char8_t* Achievements::DEFAULT_ACHIEVEMENTS_PRODUCT_ID = "Ignition_BlazeSDK_Sample";
const char8_t* Achievements::DEFAULT_ACHIEVEMENTS_LOGIN_PASSWORD = "Ignition:s@mpl3";
const char8_t* Achievements::DEFAULT_ACHIEVEMENTS_SECRET = "IgnitionSecret";

Achievements::Achievements(uint32_t userIndex)
    : LocalUserUiBuilder("Achievements", userIndex)
{
    //Initialize the Achievements component
    gBlazeHub->getComponentManager()->getAchievementsComponent()->createComponent(gBlazeHub->getComponentManager());

    getActions().add(&getActionGetAchievements());
    getActions().add(&getActionGrantAchievement());

    getActions().add(&getActionGetAchievementsForUser());
}

Achievements::~Achievements()
{
}


void Achievements::onAuthenticated()
{
    setIsVisible(true);
}

void Achievements::onDeAuthenticated()
{
    setIsVisible(false);

    getWindows().clear();
}

void Achievements::initActionGetAchievements(Pyro::UiNodeAction &action)
{
    action.setText("Get Achievements");
    action.setDescription("Gets achievements for a specified product as the current user.");

    action.getParameters().addEnum("userType", Blaze::Achievements::PERSONA, "User Type", "The user type requesting the achievements.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("language", "en_US", "Language", "", "The response language.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("productId", DEFAULT_ACHIEVEMENTS_PRODUCT_ID, "ProductId", "", "The Product Id to request achievements from.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addBool("metadata", false, "Metadata", "Whether or not to include metadata in the response.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addEnum("limit", Blaze::Achievements::ALL, "Limit", "The filter to use for retrieving achievements.", Pyro::ItemVisibility::ADVANCED);
}

void Achievements::initActionGetAchievementsForUser(Pyro::UiNodeAction &action)
{
    action.setText("Get Achievements For User");
    action.setDescription("Gets achievements for a specified product and user.");

    action.getParameters().addInt64("blazeId", 0, "Blaze ID", "", "The BlazeId of the user to get the achievement list as.");
    action.getParameters().addEnum("userType", Blaze::Achievements::PERSONA, "User Type", "The user type requesting the achievements.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("language", "en_US", "Language", "", "The response language.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("productId", DEFAULT_ACHIEVEMENTS_PRODUCT_ID, "ProductId", "", "The Product Id to request achievements from.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addBool("metadata", false, "Metadata", "Whether or not to include metadata in the response.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addEnum("limit", Blaze::Achievements::ALL, "Limit", "The filter to use for retrieving achievements.", Pyro::ItemVisibility::ADVANCED);
}

void Achievements::actionGetAchievements(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if( localUser == nullptr)
    {
        if((getUiDriver() != nullptr))
            getUiDriver()->showMessage_va( Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr"); 
        return;
    }

    parameters->addInt64("blazeId", localUser->getId());
    parameters->addInt64("authBlazeId", localUser->getId());
    actionGetAchievementsForUser(action, parameters);
}

void Achievements::actionGetAchievementsForUser(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Achievements::GetAchievementsRequest request;
    request.getUser().setId(parameters["blazeId"]);
    request.getUser().setType(parameters["userType"]);
    request.setLanguage(parameters["language"]);
    request.setProductId(parameters["productId"]);
    request.setIncludeMetadata(parameters["metadata"]);
    request.setLimit(parameters["limit"]);
    request.getAuxAuth().getUser().setId(parameters["authBlazeId"]);
    request.getAuxAuth().getUser().setType(Blaze::Achievements::NUCLEUS_PERSONA);

    Blaze::Achievements::AchievementsComponent * component = gBlazeHub->getComponentManager()->getAchievementsComponent();
    component->getAchievements(request, Blaze::Achievements::AchievementsComponent::GetAchievementsCb(this, &Achievements::GetAchievementsCb));
}

void Achievements::GetAchievementsCb(const Blaze::Achievements::GetAchievementsResponse *response, Blaze::BlazeError error, Blaze::JobId jobId)
{
    if (error == Blaze::ERR_OK)
    {
        // draw the achievement data
        Pyro::UiNodeWindow &window = getWindow("Achievements");
        window.setDockingState(Pyro::DockingState::DOCK_TOP);

        Pyro::UiNodeTable &table = window.getTable("Achievement Lookup Results");
        // if the table already exists, empty out the previous lookups.
        table.getRows().clear();

        table.getColumn("achievementId").setText("Id");
        table.getColumn("name").setText("Name");
        table.getColumn("prog").setText("Progress");
        table.getColumn("rp").setText("Reward Points");
        table.getColumn("xp").setText("Experience Points");
        table.getColumn("desc").setText("Description");
        table.getColumn("how").setText("How-to");
        table.getColumn("tier").setText("Current Tier");
        table.getColumn("count").setText("Award Count");
        table.getColumn("device").setText("Device");
        table.getColumn("updated").setText("Updated");
        table.getColumn("expiry").setText("Expiry");
        table.getColumn("xpackId").setText("Xpack Id");
        table.getColumn("xpackName").setText("Xpack Name");

        if (table.getActions().findById("GrantAchievementFromList") == nullptr)
            table.getActions().insert(&getActionGrantAchievementFromList());

        Blaze::Achievements::AchievementList::const_iterator it = response->getAchievements().begin();
        Blaze::Achievements::AchievementList::const_iterator end = response->getAchievements().end();
        for ( ; it != end; ++it)
        {
            Pyro::UiNodeTableRow &row = table.getRow(it->first.c_str());
            Pyro::UiNodeTableRowFieldContainer &fields = row.getFields();

            fields["achievementId"] = it->first.c_str();
            fields["name"] = it->second->getName();
            fields["prog"].setAsString_va("%" PRId64 " / %" PRId64, it->second->getP(), it->second->getT());
            fields["rp"] = it->second->getRp();
            fields["xp"] = it->second->getXp();
            fields["desc"] = it->second->getDesc();
            fields["how"] = it->second->getHowto();
            fields["tier"].setAsString_va("%d / %d", it->second->getTc(), it->second->getTt());
            fields["count"] = it->second->getCnt();
            fields["device"] = it->second->getD();
            fields["updated"] = it->second->getU().getMicroSeconds();
            fields["expiry"] = it->second->getE().getMicroSeconds();
            fields["xpackId"] = it->second->getXpack().getId();
            fields["xpackName"] = it->second->getXpack().getName();

            Blaze::Achievements::AchievementData::TieredAchievementDataList::const_iterator tItr = it->second->getTn().begin();
            Blaze::Achievements::AchievementData::TieredAchievementDataList::const_iterator tEnd = it->second->getTn().end();
            for (int i = 0; tItr != tEnd; ++tItr, ++i)
            {
                const Blaze::Achievements::TieredAchievementData *tier = *tItr;
                char idStr[8];
                sprintf(idStr, "%d", i);
                Pyro::UiNodeTableRow &childRow = row.getChildRow(idStr);
                Pyro::UiNodeTableRowFieldContainer &childFields = childRow.getFields();

                childFields["name"] = tier->getName();
                childFields["prog"].setAsString_va("/ %" PRId64, tier->getT());
                childFields["rp"] = tier->getRp();
                childFields["xp"] = tier->getXp();
                childFields["desc"] = tier->getDesc();
                childFields["how"] = tier->getHowto();
            }
        }
    }

    REPORT_BLAZE_ERROR(error);
}

void Achievements::initActionGrantAchievement(Pyro::UiNodeAction &action)
{
    action.setText("Grant Achievement");
    action.setDescription("Grants the specified achievement for the current user.");

    action.getParameters().addEnum("userType", Blaze::Achievements::PERSONA, "User Type", "The user type requesting the achievements.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("userPassword", DEFAULT_ACHIEVEMENTS_LOGIN_PASSWORD, "User Password", "", "The HTTP auth information in [user name]:[password] format.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("secretKey", DEFAULT_ACHIEVEMENTS_SECRET, "Secret Key", "", "The shared key to generate a cryptographic signature for verification.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("language", "en_US", "Language", "", "The response language.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("productId", DEFAULT_ACHIEVEMENTS_PRODUCT_ID, "ProductId", "", "The Product Id to request achievements from.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addBool("metadata", false, "Metadata", "Whether or not to include metadata in the record.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("achievementId", "", "Achievement Id", "", "Id of the achievement to grant.", Pyro::ItemVisibility::REQUIRED);
    action.getParameters().addInt64("progress", 0, "Achievement Progress", "", "Id of the achievement to grant.", Pyro::ItemVisibility::REQUIRED);
}

void Achievements::actionGrantAchievement(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::UserManager::LocalUser *localUser = gBlazeHub->getUserManager()->getLocalUser(getUserIndex());
    if( localUser == nullptr)
    {
        if(getUiDriver() != nullptr)
            getUiDriver()->showMessage_va( Pyro::ErrorLevel::ERR, "Pyro Error", "Error: Local user was nullptr"); 
        return;
    }

    Blaze::Achievements::GrantAchievementRequest request;
    request.getUser().setId(localUser->getId());
    request.getUser().setType(parameters["userType"]);
    request.getAuxAuth().setUserPassword(parameters["userPassword"]);
    request.getAuxAuth().setSecretKey(parameters["secretKey"]);
    request.setLanguage(parameters["language"]);
    request.setProductId(parameters["productId"]);
    request.setAchieveId(parameters["achievementId"]);
    request.getProgress().setPoints(parameters["progress"]);
    request.getAuxAuth().getUser().setId(localUser->getId());
    request.getAuxAuth().getUser().setType(Blaze::Achievements::NUCLEUS_PERSONA);

    Blaze::Achievements::AchievementsComponent * component = gBlazeHub->getComponentManager()->getAchievementsComponent();
    component->grantAchievement(request, Blaze::Achievements::AchievementsComponent::GrantAchievementCb(this, &Achievements::GrantAchievementCb));
}

void Achievements::GrantAchievementCb(const Blaze::Achievements::AchievementData *response, Blaze::BlazeError error, Blaze::JobId jobId)
{
    if (error == Blaze::ERR_OK)
    {
        // draw the achievement data
        Pyro::UiNodeWindow &window = getWindow("GrantedAchievement");
        window.setDockingState(Pyro::DockingState::DOCK_TOP);

        Pyro::UiNodeTable &table = window.getTable("Granted Achievement Results");

        table.getColumn("achievementId").setText("Id");
        table.getColumn("name").setText("Name");
        table.getColumn("desc").setText("Description");
        table.getColumn("how").setText("How-to");
        table.getColumn("prog").setText("Progress");
        table.getColumn("count").setText("Award Count");

        Pyro::UiNodeTableRowFieldContainer &fields = table.getRow(response->getName()).getFields();

        fields["name"] = response->getName();
        fields["desc"] = response->getDesc();
        fields["how"] = response->getHowto();
        fields["prog"].setAsString_va("%" PRId64 " / %" PRId64, response->getP(), response->getT());
        fields["count"] = response->getCnt();
    }

    REPORT_BLAZE_ERROR(error);
}

void Achievements::initActionGrantAchievementFromList(Pyro::UiNodeAction &action)
{
    action.setText("Grant Achievement");
    action.setDescription("Grants a specific achievement from a retrieved list.");

    action.getParameters().addEnum("userType", Blaze::Achievements::PERSONA, "User Type", "The user type requesting the achievements.", Pyro::ItemVisibility::HIDDEN);
    action.getParameters().addString("userPassword", DEFAULT_ACHIEVEMENTS_LOGIN_PASSWORD, "User Password", "", "The HTTP auth information in [user name]:[password] format.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("secretKey", DEFAULT_ACHIEVEMENTS_SECRET, "Secret Key", "", "The shared key to generate a cryptographic signature for verification.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("language", "en_US", "Language", "", "The response language.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addString("productId", DEFAULT_ACHIEVEMENTS_PRODUCT_ID, "ProductId", "", "The Product Id to request achievements from.", Pyro::ItemVisibility::ADVANCED);
    action.getParameters().addInt64("progress", 0, "Achievement Progress", "", "Id of the achievement to grant.", Pyro::ItemVisibility::REQUIRED);
}

void Achievements::actionGrantAchievementFromList(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    // this will pick up the local user's blaze ID
    actionGrantAchievement(action, parameters);
}

}
