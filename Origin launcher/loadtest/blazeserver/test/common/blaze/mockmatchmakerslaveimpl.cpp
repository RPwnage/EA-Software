/*************************************************************************************************/
/*! 
    \file mockmatchmakerslaveimpl.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "mockmatchmakerslaveimpl.h"

namespace BlazeServerTest
{
namespace Matchmaker
{

    MockMatchmakerSlaveImpl::MockMatchmakerSlaveImpl(const Blaze::Matchmaker::MatchmakerConfig& testConfig, bool activate)
    {
        testConfig.copyInto(mTestConfig);
        if (activate)
            activateComponent();
    }

    MockMatchmakerSlaveImpl::~MockMatchmakerSlaveImpl()
    {
    }

    void MockMatchmakerSlaveImpl::activateComponent() 
    {
        // by default use orig
        Blaze::Matchmaker::MatchmakerSlaveImpl::activateComponent(); 
    }
    void MockMatchmakerSlaveImpl::prepareForReconfigure()
    {
        Blaze::Matchmaker::MatchmakerSlaveImpl::prepareForReconfigure();
    }
    void MockMatchmakerSlaveImpl::reconfigureComponent()
    {
        Blaze::Matchmaker::MatchmakerSlaveImpl::reconfigureComponent();
    }
    void MockMatchmakerSlaveImpl::shutdownComponent()
    {
        Blaze::Matchmaker::MatchmakerSlaveImpl::shutdownComponent();
    }
    
    bool MockMatchmakerSlaveImpl::onValidateConfigTdf(EA::TDF::Tdf& config, const EA::TDF::Tdf* referenceConfigPtr, Blaze::ConfigureValidationErrors& validationErrors)
    {
        // by default use orig
        return Blaze::Matchmaker::MatchmakerSlaveImpl::onValidateConfigTdf(config, referenceConfigPtr, validationErrors);// calls onValidateConfig
    }

    bool MockMatchmakerSlaveImpl::loadConfig(EA::TDF::TdfPtr& newConfig, bool loadFromStaging)
    {
        // allow unit tests to use custom test testConfig
        newConfig = &mTestConfig;
        return true;
    }

    bool MockMatchmakerSlaveImpl::loadPreconfig(EA::TDF::TdfPtr& newPreconfig)
    {
        newPreconfig = &mTestConfig;
        return true;
    }

}//ns Matchmaker
}//ns BlazeServerTest
