/*************************************************************************************************/
/*! 
    \file mockmatchmakerslaveimpl.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SERVER_TEST_MOCK_MATCHMAKERSLAVE_IMPL_H
#define BLAZE_SERVER_TEST_MOCK_MATCHMAKERSLAVE_IMPL_H

#include "test/mock/mocklogger.h"
#include "framework/blaze.h"
#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/tdf/matchmaker_component_config_server.h"

namespace Blaze
{
    class ConfigureValidationErrors;
    class ComponentStatus;
}

namespace BlazeServerTest
{
namespace Matchmaker
{
    class MockMatchmakerSlaveImpl : public Blaze::Matchmaker::MatchmakerSlaveImpl
    {
    public:
        MockMatchmakerSlaveImpl(const Blaze::Matchmaker::MatchmakerConfig& config, bool activate = true);
        ~MockMatchmakerSlaveImpl();

        void setPreconfig(const Blaze::Matchmaker::MatchmakerSlavePreconfig& config) { config.copyInto(mTestPreConfig); }

        ////////////////
        // ComponentStub interface:

        void activateComponent() override;
        void prepareForReconfigure() override;
        void reconfigureComponent() override;
        void shutdownComponent() override;

        bool onValidateConfigTdf(EA::TDF::Tdf& config, const EA::TDF::Tdf* referenceConfigPtr, Blaze::ConfigureValidationErrors& validationErrors) override;

        bool loadConfig(EA::TDF::TdfPtr& newConfig, bool loadFromStaging) override;
        bool loadPreconfig(EA::TDF::TdfPtr& newConfig) override;

        // Default NOOP/disabled implementations
        bool onPreconfigure() override { return true; }
        bool onValidatePreconfigTdf(EA::TDF::Tdf& preconfig, const EA::TDF::Tdf* referenceConfigPtr, Blaze::ConfigureValidationErrors& validationErrors) override { return true; }

        bool onConfigure() override { return true; }
        bool onResolve() override { return true; }
        bool onConfigureReplication() override { return true; }
        bool onActivate() override { return true; }
        bool onCompleteActivation() override { return true; }
        bool onPrepareForReconfigureComponent(const EA::TDF::Tdf& newConfig) override { return true; }
        bool onReconfigure() override { return true; }
        void getStatusInfo(Blaze::ComponentStatus& status) const override { }
        uint16_t getDbSchemaVersion() const override { return 0; }
        uint32_t getContentDbSchemaVersion() const override { return 0; }

        Blaze::Matchmaker::MatchmakerConfig mTestConfig;
        Blaze::Matchmaker::MatchmakerSlavePreconfig mTestPreConfig;
    };
}//ns Matchmaker
}//ns BlazeServerTest

#endif
