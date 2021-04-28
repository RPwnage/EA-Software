
#ifndef EXAMPLE_H
#define EXAMPLE_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/component/example/tdf/exampletypes.h"

namespace Ignition
{

class Example
    : public BlazeHubUiBuilder
    , public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener
{
    public:
        Example();
        virtual ~Example();

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    protected:

    private:
        PYRO_ACTION(Example, CreateExampleItem);
        PYRO_ACTION(Example, ReadExampleItem);
        PYRO_ACTION(Example, UpdateExampleItem);
        PYRO_ACTION(Example, PinExampleItem);
        PYRO_ACTION(Example, DeleteExampleItem);

        void CreateExampleItemCb(const Blaze::Example::CreateExampleItemResponse* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void ReadExampleItemCb(const Blaze::Example::ExampleItemState* response, Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void UpdateExampleItemCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void PinExampleItemCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
        void DeleteExampleItemCb(Blaze::BlazeError blazeError, Blaze::JobId jobId);
};

}

#endif
