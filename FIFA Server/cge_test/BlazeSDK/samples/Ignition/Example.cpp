
#include "Ignition/Example.h"

#include "BlazeSDK/component/examplecomponent.h"

namespace Ignition
{

Example::Example()
    : BlazeHubUiBuilder("Example")
{
    gBlazeHub->getComponentManager()->getExampleComponent()->createComponent(gBlazeHub->getComponentManager());

    getActions().add(&getActionCreateExampleItem());
    getActions().add(&getActionReadExampleItem());
    getActions().add(&getActionUpdateExampleItem());
    getActions().add(&getActionPinExampleItem());
    getActions().add(&getActionDeleteExampleItem());
}

Example::~Example()
{
}


void Example::onAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(true);
}

void Example::onDeAuthenticatedPrimaryUser()
{
    setIsVisibleForAll(false);
}

void Example::initActionCreateExampleItem(Pyro::UiNodeAction &action)
{
    action.setText("Create ExampleItem");
    action.setDescription("Call the Example::createExampleItem() RPC.");

    action.getParameters().addString("state", "", "State");
    action.getParameters().addUInt64("ttl", 180, "Time To Line (seconds)");
}

void Example::actionCreateExampleItem(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Example::CreateExampleItemRequest req;
    req.setState(parameters["state"]);
    req.setTimeToLiveInSeconds(parameters["ttl"]);

    gBlazeHub->getComponentManager()->getExampleComponent()->createExampleItem(req, Blaze::Example::ExampleComponent::CreateExampleItemCb(this, &Example::CreateExampleItemCb));
}

void Example::CreateExampleItemCb(const Blaze::Example::CreateExampleItemResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);
}

void Example::initActionReadExampleItem(Pyro::UiNodeAction &action)
{
    action.setText("Read ExampleItem");
    action.setDescription("Call the Example::readExampleItem() RPC.");

    action.getParameters().addUInt64("exampleItemId", 0, "ExampleItemId");
}

void Example::actionReadExampleItem(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Example::ReadExampleItemRequest req;
    req.setExampleItemId(parameters["exampleItemId"]);

    gBlazeHub->getComponentManager()->getExampleComponent()->readExampleItem(req, Blaze::Example::ExampleComponent::ReadExampleItemCb(this, &Example::ReadExampleItemCb));
}

void Example::ReadExampleItemCb(const Blaze::Example::ExampleItemState* response, Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);
}

void Example::initActionUpdateExampleItem(Pyro::UiNodeAction &action)
{
    action.setText("Update ExampleItem");
    action.setDescription("Call the Example::updateExampleItem() RPC.");

    action.getParameters().addUInt64("exampleItemId", 0, "ExampleItemId");
    action.getParameters().addString("state", "", "State");
}

void Example::actionUpdateExampleItem(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Example::UpdateExampleItemRequest req;
    req.setExampleItemId(parameters["exampleItemId"]);
    req.setState(parameters["state"]);

    gBlazeHub->getComponentManager()->getExampleComponent()->updateExampleItem(req, Blaze::Example::ExampleComponent::UpdateExampleItemCb(this, &Example::UpdateExampleItemCb));
}

void Example::UpdateExampleItemCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);
}

void Example::initActionPinExampleItem(Pyro::UiNodeAction &action)
{
    action.setText("Pin ExampleItem");
    action.setDescription("Call the Example::pinExampleItem() RPC.");

    action.getParameters().addUInt64("exampleItemId", 0, "ExampleItemId");
    action.getParameters().addUInt64("duration", 40, "Duration (seconds)");
}

void Example::actionPinExampleItem(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Example::PinExampleItemRequest req;
    req.setExampleItemId(parameters["exampleItemId"]);
    req.setDurationInSeconds(parameters["duration"]);

    gBlazeHub->getComponentManager()->getExampleComponent()->pinExampleItem(req, Blaze::Example::ExampleComponent::PinExampleItemCb(this, &Example::PinExampleItemCb));
}

void Example::PinExampleItemCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);
}

void Example::initActionDeleteExampleItem(Pyro::UiNodeAction &action)
{
    action.setText("Delete ExampleItem");
    action.setDescription("Call the Example::deleteExampleItem() RPC.");

    action.getParameters().addUInt64("exampleItemId", 0, "ExampleItemId");
}

void Example::actionDeleteExampleItem(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters)
{
    Blaze::Example::DeleteExampleItemRequest req;
    req.setExampleItemId(parameters["exampleItemId"]);

    gBlazeHub->getComponentManager()->getExampleComponent()->deleteExampleItem(req, Blaze::Example::ExampleComponent::DeleteExampleItemCb(this, &Example::DeleteExampleItemCb));
}

void Example::DeleteExampleItemCb(Blaze::BlazeError blazeError, Blaze::JobId jobId)
{
    REPORT_BLAZE_ERROR(blazeError);
}

}
