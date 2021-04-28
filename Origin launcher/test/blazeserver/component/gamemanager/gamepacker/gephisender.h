/*! ************************************************************************************************/
/*!
\file gephisender.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GEPHI_SENDER_H
#define BLAZE_GEPHI_SENDER_H

#include <gamepacker/packer.h>

namespace Blaze
{

namespace GameManager
{

    class GephiSender
    {
    public:
        GephiSender() {}

        class SimpleColor
        {
        public:
            float red;
            float green;
            float blue;

            SimpleColor()
            {
                red = 0.0f;
                green = 0.0f;
                blue = 0.0f;
            }
            SimpleColor(float r, float g, float b)
            {
                red = r;
                green = g;
                blue = b;
            }
            SimpleColor(const SimpleColor& color)
            {
                red = color.red;
                green = color.green;
                blue = color.blue;
            }
        };

        // predefined colors
        static const SimpleColor GEPHI_GAME_NODE_DEFAULT_COLOR;
        static const SimpleColor GEPHI_GAME_NODE_REAP_COLOR;

        static const SimpleColor GEPHI_PLAYER_NODE_DEFAULT_COLOR;
        static const SimpleColor GEPHI_PLAYER_NODE_EXPIRED_COLOR;

        static const SimpleColor GEPHI_PLAYER_NODE_TEAM1_COLOR;
        static const SimpleColor GEPHI_PLAYER_NODE_TEAM2_COLOR;

        static const SimpleColor GEPHI_SCORE_NODE_COLOR;

        static const uint64_t GEPHI_DEFAULT_NODE_SIZE;
        static const uint64_t GEPHI_DEFAULT_NODE_INCREASE_SIZE;

        static const uint64_t GEPHI_SLEEP;
        static const uint64_t GEPHI_SLEEP_LONG;

        static const char*  GEPHI_NODE_SKILL;
        static const char*  GEPHI_NODE_SIZE;
        static const char*  GEPHI_NODE_LEGEND;

        void addNode(const eastl::string nodeKey, const SimpleColor color, uint64_t size = GEPHI_DEFAULT_NODE_SIZE);
        void changeNode(const eastl::string nodeKey, const eastl::string label, const SimpleColor color, uint64_t size = GEPHI_DEFAULT_NODE_SIZE);
        void deleteNode(const eastl::string nodeKey, uint64_t delay);

        void moveNode(const eastl::string nodeKey, const eastl::string label, const SimpleColor color, uint64_t size = GEPHI_DEFAULT_NODE_SIZE);

        void addEdge(const eastl::string sourceKey, const eastl::string targetKey);
        void deleteEdge(const eastl::string sourceKey, const eastl::string targetKey);


        static void calculateScore(Packer::PackerSilo* packer, uint64_t factorIndex, const Packer::Game& game, float& scoreRatio, float& scorePercent);
        static void getGameKey(eastl::string& gameKey, uint64_t gameId) { gameKey.append_sprintf("G%d", gameId); }
        static void getGameLabel(eastl::string& gameLabel, uint64_t gameId, float sizePercent = 100.0f, float skillPercent = 100.0f) { gameLabel.append_sprintf("G%d Size: %.1f%% Skill: %.1f%%", gameId, (100.0f - sizePercent), (100.0f - skillPercent)); }

        // Demo hack: keep party id's within 1000 so it looks good visually.
        static void getPartyKey(eastl::string& partyKey, uint64_t partyId) { partyKey.append_sprintf("P%d", partyId % 1000); }

    private:
        static const char* GEPHI_WORKSPACE;

    };
}
}

#endif
