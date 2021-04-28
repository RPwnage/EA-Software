/*! ************************************************************************************************/
/*!
    \file arsonexternaltournamentutil.h
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef ARSONCOMPONENT_EXTERNAL_TOURNAMENT_UTIL_H
#define ARSONCOMPONENT_EXTERNAL_TOURNAMENT_UTIL_H

#include "gamemanager/tdf/externalsessionconfig_server.h" // for ExternalSessionServerConfig in getExternalSessionConfig
#include "arson/tournamentorganizer/tdf/arsontournamentorganizer_server.h"

namespace Blaze
{
namespace Arson
{
    /*! ************************************************************************************************/
    /*! \brief Generic interface for dealing with external/first party tournament services
    ***************************************************************************************************/
    class ArsonExternalTournamentUtil
    {
    NON_COPYABLE(ArsonExternalTournamentUtil);
    public:
        ArsonExternalTournamentUtil(const ArsonTournamentOrganizerConfig& config)
        {
            config.copyInto(mConfig);
        }
        virtual ~ArsonExternalTournamentUtil(){}

        /*! ************************************************************************************************/
        /*! \brief create tournament
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError createTournament(const CreateTournamentParameters& params, ArsonExternalTournamentInfo* result)
        {
            return ERR_OK;
        }

        /*! ************************************************************************************************/
        /*! \brief update tournament
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError updateTournament(const UpdateTournamentParameters& params, ArsonExternalTournamentInfo* result)
        {
            return ERR_OK;
        }

        /*! ************************************************************************************************/
        /*! \brief get tournament
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError getTournament(const GetTournamentParameters& params, ArsonExternalTournamentInfo& result)
        {
            return ERR_OK;
        }

        /*! ************************************************************************************************/
        /*! \brief get tournament
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError removeTournament(const RemoveTournamentParameters& params)
        {
            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief creates and joins users to the tournament team.
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError createTeam(const CreateTournamentTeamParameters& params, Blaze::Arson::TournamentTeamInfo* result)
        {
            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief joins user to the tournament team.
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError joinTeam(const JoinTournamentTeamParameters& params, Blaze::Arson::TournamentTeamInfo* result)
        {
            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief leaves user from the tournament team.
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError leaveTeam(const LeaveTournamentTeamParameters& params)
        {
            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief updates the tournament team.
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError updateTeam(const UpdateTournamentTeamParameters& params, Blaze::Arson::TournamentTeamInfo* result)
        {
            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief get the user's team in the tournament. Returns ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM if none found
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError getTeamForUser(const GetTournamentTeamParameters& params, Blaze::Arson::TournamentTeamInfo& result)
        {
            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief get the user's teams in tournaments. Returns ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM if none found
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError getTeamsForUser(const GetTournamentTeamsParameters& params, Blaze::Arson::TournamentTeamInfoList& result)
        {
            return ERR_OK;
        }
        
        /*!************************************************************************************************/
        /*! \brief get the team in the tournament by its unique name. Returns ARSON_TO_ERR_INVALID_TOURNAMENT_TEAM if none found
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError getTeamByName(const GetTournamentTeamParameters& params, Blaze::Arson::TournamentTeamInfo& result)
        {
            return ERR_OK;
        }

        /*! ************************************************************************************************/
        /*! \brief set tournament team's next match
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError setTeamMatch(const SetTournamentTeamMatchParameters& params, Blaze::Arson::TournamentTeamInfo* result)
        {
            return ERR_OK;
        }

        /*!************************************************************************************************/
        /*! \brief submits the tournament match result
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError submitResult(const SubmitTournamentMatchResultParameters& params)
        {
            return ERR_OK;
        }


        /*! ************************************************************************************************/
        /*! \brief get all tournaments for the title/organizer
        ***************************************************************************************************/
        virtual Blaze::BlazeRpcError getTournaments(const GetTournamentsParameters& params, GetTournamentsResult& result)
        {
            return ERR_OK;
        }

        /*! ************************************************************************************************/
        /*! \brief creates the external tournament util. Returns nullptr on error.
        ***************************************************************************************************/
        static ArsonExternalTournamentUtil* createExternalTournamentService(const Blaze::Arson::ArsonTournamentOrganizerConfig& config);

        bool isMock() const;

    protected:
        static BlazeRpcError waitSeconds(uint64_t seconds, const char8_t* context, size_t logRetryNumber);

        const Blaze::GameManager::ExternalSessionServerConfig& getExternalSessionConfig() const { return mConfig.getExternalSessions(); }

    protected:
        Blaze::Arson::ArsonTournamentOrganizerConfig mConfig;
    };

}//ns Arson
}//ns Blaze
#endif
