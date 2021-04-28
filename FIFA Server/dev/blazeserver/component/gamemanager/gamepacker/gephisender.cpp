/*! ************************************************************************************************/
/*!
    \file gephisender.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "gephi/tdf/gephi.h"
#include "gephi/rpc/gephislave.h"
#include "gamemanager/gamepacker/gephisender.h"
#include "gamemanager/rpc/gamepackerslave.h"

#pragma push_macro("LOGGER_CATEGORY")
#undef LOGGER_CATEGORY
#define LOGGER_CATEGORY Blaze::BlazeRpcLog::gamepacker

namespace Blaze
{


namespace GameManager
{
const char* GephiSender::GEPHI_WORKSPACE = "workspace0";
const uint64_t GephiSender::GEPHI_DEFAULT_NODE_SIZE = 10;
const uint64_t GephiSender::GEPHI_DEFAULT_NODE_INCREASE_SIZE = 30;
const uint64_t GephiSender::GEPHI_SLEEP = 160 * 1000;
const uint64_t GephiSender::GEPHI_SLEEP_LONG = 2 * 1000 * 1000;

const char*  GephiSender::GEPHI_NODE_SKILL = "4";
const char*  GephiSender::GEPHI_NODE_SIZE = "6";
const char*  GephiSender::GEPHI_NODE_LEGEND = "0";

const GephiSender::SimpleColor GephiSender::GEPHI_GAME_NODE_DEFAULT_COLOR(0.9f, 1.0f, 0.9f);
const GephiSender::SimpleColor GephiSender::GEPHI_GAME_NODE_REAP_COLOR(0.0f, 1.0f, 0.0f);

const GephiSender::SimpleColor GephiSender::GEPHI_PLAYER_NODE_DEFAULT_COLOR(0.9f, 0.9f, 0.9f);
const GephiSender::SimpleColor GephiSender::GEPHI_PLAYER_NODE_EXPIRED_COLOR(0.3f, 0.3f, 0.3f);

const GephiSender::SimpleColor GephiSender::GEPHI_PLAYER_NODE_TEAM1_COLOR(0.5f, 0.5f, 1.0f);
const GephiSender::SimpleColor GephiSender::GEPHI_PLAYER_NODE_TEAM2_COLOR(0.8f, 0.5f, 1.0f);

const GephiSender::SimpleColor GephiSender::GEPHI_SCORE_NODE_COLOR(1.0f, 1.0f, 1.0f);


void GephiSender::addNode(const eastl::string nodeKey, const SimpleColor color, uint64_t size)
{
    Gephi::GephiSlave * gephiSlave = (Gephi::GephiSlave *)Blaze::gOutboundHttpService->getService(Gephi::GephiSlave::COMPONENT_INFO.name);
    if (gephiSlave == nullptr)
    {
        WARN_LOG("[GephiSender] Unable to obtain proxy component " << Gephi::GephiSlave::COMPONENT_INFO.name);
    }
    else
    {
        Gephi::AddNodeRequest playerNodeRequest;
        playerNodeRequest.setWorkspace(GEPHI_WORKSPACE);
        auto* nodeAttributes = playerNodeRequest.getBody().getAddNode().allocate_element();
        nodeAttributes->setLabel(nodeKey.c_str());
        nodeAttributes->setSize(size);

        // Hack to check Game node vs Player node
        if (nodeKey.at(0) == 'G')
        {
            nodeAttributes->setXLocation(-700.0f);
            nodeAttributes->setYLocation((float)((.01 + (rand() % 300)) * 100) / (float)100.0);
        }
        else
        {
            nodeAttributes->setXLocation(700.0f);
            nodeAttributes->setYLocation((float)((.01 + (rand() % 300)) * 100) / (float)100.0);
        }

        nodeAttributes->setBlue(color.blue);
        nodeAttributes->setGreen(color.green);
        nodeAttributes->setRed(color.red);

        // nodeKey is also the name of the node, and how to reference the node for adding edges and the like.
        playerNodeRequest.getBody().getAddNode().insert(eastl::make_pair(nodeKey.c_str(), nodeAttributes));

        Fiber::sleep(GEPHI_SLEEP, "Gephi sleep");
        gephiSlave->addNode(playerNodeRequest);
    }
}


void GephiSender::changeNode(const eastl::string nodeKey, const eastl::string label, const SimpleColor color, uint64_t size)
{
    Gephi::GephiSlave * gephiSlave = (Gephi::GephiSlave *)Blaze::gOutboundHttpService->getService(Gephi::GephiSlave::COMPONENT_INFO.name);
    if (gephiSlave == nullptr)
    {
        WARN_LOG("[GephiSender] Unable to obtain proxy component " << Gephi::GephiSlave::COMPONENT_INFO.name);
    }
    else
    {
        Gephi::ChangeNodeRequest playerNodeRequest;
        playerNodeRequest.setWorkspace(GEPHI_WORKSPACE);
        auto* nodeAttributes = playerNodeRequest.getBody().getChangeNode().allocate_element();
        nodeAttributes->setLabel(label.c_str());
        nodeAttributes->setSize(size);

        nodeAttributes->setBlue(color.blue);
        nodeAttributes->setGreen(color.green);
        nodeAttributes->setRed(color.red);

        // nodeKey here is the name of the node, and how to reference the node for adding edges and the like.
        playerNodeRequest.getBody().getChangeNode().insert(eastl::make_pair(nodeKey.c_str(), nodeAttributes));

        Fiber::sleep(GEPHI_SLEEP, "Gephi sleep");
        gephiSlave->changeNode(playerNodeRequest);
    }
}


void GephiSender::moveNode(const eastl::string nodeKey, const eastl::string label, const SimpleColor color, uint64_t size)
{
    Gephi::GephiSlave * gephiSlave = (Gephi::GephiSlave *)Blaze::gOutboundHttpService->getService(Gephi::GephiSlave::COMPONENT_INFO.name);
    if (gephiSlave == nullptr)
    {
        WARN_LOG("[GephiSender] Unable to obtain proxy component " << Gephi::GephiSlave::COMPONENT_INFO.name);
    }
    else
    {
        Gephi::MoveNodeRequest playerNodeRequest;
        playerNodeRequest.setWorkspace(GEPHI_WORKSPACE);
        auto* nodeAttributes = playerNodeRequest.getBody().getChangeNode().allocate_element();
        nodeAttributes->setLabel(label.c_str());
        nodeAttributes->setSize(size);

        nodeAttributes->setBlue(color.blue);
        nodeAttributes->setGreen(color.green);
        nodeAttributes->setRed(color.red);

        nodeAttributes->setXLocation(-250.0f);
        nodeAttributes->setYLocation(100.0f);

        // nodeKey here is the name of the node, and how to reference the node for adding edges and the like.
        playerNodeRequest.getBody().getChangeNode().insert(eastl::make_pair(nodeKey.c_str(), nodeAttributes));

        Fiber::sleep(GEPHI_SLEEP, "Gephi sleep");
        gephiSlave->moveNode(playerNodeRequest);
    }
}


void GephiSender::deleteNode(const eastl::string nodeKey, uint64_t delay)
{
    Gephi::GephiSlave * gephiSlave = (Gephi::GephiSlave *)Blaze::gOutboundHttpService->getService(Gephi::GephiSlave::COMPONENT_INFO.name);
    if (gephiSlave == nullptr)
    {
        WARN_LOG("[GephiSender] Unable to obtain proxy component " << Gephi::GephiSlave::COMPONENT_INFO.name);
    }
    else
    {
        Gephi::DeleteNodeRequest playerNodeRequest;
        playerNodeRequest.setWorkspace(GEPHI_WORKSPACE);
        auto* nodeAttributes = playerNodeRequest.getBody().getDeleteNode().allocate_element();

        playerNodeRequest.getBody().getDeleteNode().insert(eastl::make_pair(nodeKey.c_str(), nodeAttributes));

        if (delay != 0)
        {
            Fiber::sleep(delay, "Gephi sleep");
        }
        gephiSlave->deleteNode(playerNodeRequest);
    }
}


void GephiSender::addEdge(const eastl::string sourceKey, const eastl::string targetKey)
{
    Gephi::GephiSlave * gephiSlave = (Gephi::GephiSlave *)Blaze::gOutboundHttpService->getService(Gephi::GephiSlave::COMPONENT_INFO.name);
    if (gephiSlave == nullptr)
    {
        WARN_LOG("[GephiSender] Unable to obtain proxy component " << Gephi::GephiSlave::COMPONENT_INFO.name);
    }
    else
    {
        eastl::string edgeKey;
        edgeKey.append(sourceKey.c_str());
        edgeKey.append(targetKey.c_str());

        Gephi::AddEdgeRequest playerEdgeRequest;
        playerEdgeRequest.setWorkspace(GEPHI_WORKSPACE);
        auto* edgeAttributes = playerEdgeRequest.getBody().getAddEdge().allocate_element();
        edgeAttributes->setDirected(false);
        edgeAttributes->setWeight(2);
        edgeAttributes->setSource(sourceKey.c_str());
        edgeAttributes->setTarget(targetKey.c_str());

        playerEdgeRequest.getBody().getAddEdge().insert(eastl::make_pair(edgeKey.c_str(), edgeAttributes));
        Fiber::sleep(GEPHI_SLEEP, "Gephi sleep");
        gephiSlave->addEdge(playerEdgeRequest);
    }
}


void GephiSender::deleteEdge(const eastl::string sourceKey, const eastl::string targetKey)
{
    Gephi::GephiSlave * gephiSlave = (Gephi::GephiSlave *)Blaze::gOutboundHttpService->getService(Gephi::GephiSlave::COMPONENT_INFO.name);
    if (gephiSlave == nullptr)
    {
        WARN_LOG("[GephiSender] Unable to obtain proxy component " << Gephi::GephiSlave::COMPONENT_INFO.name);
    }
    else
    {
        eastl::string edgeKey;
        edgeKey.append(sourceKey.c_str());
        edgeKey.append(targetKey.c_str());

        Gephi::DeleteEdgeRequest playerEdgeRequest;
        playerEdgeRequest.setWorkspace(GEPHI_WORKSPACE);
        auto* edgeAttributes = playerEdgeRequest.getBody().getDeleteEdge().allocate_element();

        playerEdgeRequest.getBody().getDeleteEdge().insert(eastl::make_pair(edgeKey.c_str(), edgeAttributes));

        Fiber::sleep(GEPHI_SLEEP, "Gephi sleep");
        gephiSlave->deleteEdge(playerEdgeRequest);
    }
}


void GephiSender::calculateScore(Packer::PackerSilo* packer, uint64_t factorIndex, const Packer::Game& game, float& scoreRatio, float& scorePercent)
{
    scoreRatio = game.mFactorScores[factorIndex];
    scorePercent = scoreRatio * 100.0f;

    TRACE_LOG("[GamePacker] MRW Score for Game " << game.mGameId << " and factor " << factorIndex << " is " << scorePercent);
}

}
}

#pragma pop_macro("LOGGER_CATEGORY")

