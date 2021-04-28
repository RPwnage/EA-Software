/*************************************************************************************************/
/*!
	\file   proclubscustomdb.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustom/proclubscustomdb.h"
#include "proclubscustom/proclubscustomqueries.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// proclubscustom includes
#include "proclubscustomslaveimpl.h"
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
    namespace proclubscustom
    {
        // Statics ***** values are set in initialize()
        EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdFetchSettings = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdUpdateSettings = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdFetchAIPlayerCustomization = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdUpdateAIPlayerCustomization = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdFetchCustomTactics = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdInsertCustomTactics = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdResetAiPlayerNames = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdFetchAiPlayerNames = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdFetchAvatarName = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdFetchAvatarData = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdInsertAvatarName = 0;
		EA_THREAD_LOCAL PreparedStatementId ProclubsCustomDb::mCmdFlagProfaneAvatarName = 0;
		

        /*** Public Methods ******************************************************************************/

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the slave component.
        */
        /*************************************************************************************************/
        ProclubsCustomDb::ProclubsCustomDb(ProclubsCustomSlaveImpl* component) : 
            mDbConn(),
            mStatementError(ERR_OK)
        {
            if (NULL != component)
            {
                mComponentMaster = component->getMaster();
            }
            else
            {
				ASSERT_LOG("[ProclubsCustomDb] - unable to determine master instance from slave!");
            }
        }

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the master component.
        */
        /*************************************************************************************************/
        ProclubsCustomDb::ProclubsCustomDb() : 
            mComponentMaster(NULL), 
            mDbConn(),
            mStatementError(ERR_OK)
        {
        }

        /*************************************************************************************************/
        /*!
            \brief destructor

            Destroys a db handler.
        */
        /*************************************************************************************************/
        ProclubsCustomDb::~ProclubsCustomDb()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief initialize

            Registers prepared statements for the specified dbId.
        */
        /*************************************************************************************************/
        void ProclubsCustomDb::initialize(uint32_t dbId)
        {
            INFO_LOG("[ProclubsCustomDb].initialize() - dbId = "<<dbId <<" ");

            mCmdFetchSettings					= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_fetchSetting",				PROCLUBSCUSTOM_FETCH_TEAM_SETTINGS);	
			mCmdUpdateSettings					= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_updateSetting",				PROCLUBSCUSTOM_UPDATE_TEAM_SETTINGS);
			mCmdFetchAIPlayerCustomization		= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_fetchAiPlayerCust",			PROCLUBSCUSTOM_FETCH_AI_PLAYER_SETTINGS);
			mCmdUpdateAIPlayerCustomization		= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_updateAiPlayerCust",		PROCLUBSCUSTOM_UPDATE_AI_PLAYER_SETTINGS);
			mCmdFetchCustomTactics				= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_fetchCustomTactics",		PROCLUBSCUSTOM_FETCH_CUSTOM_TACTICS);
			mCmdInsertCustomTactics				= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_insertCustomTactics",		PROCLUBSCUSTOM_INSERT_CUSTOM_TACTICS);
			mCmdResetAiPlayerNames				= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_resetAiPlayerNames",		PROCLUBSCUSTOM_RESET_AI_PLAYER_NAMES);
			mCmdFetchAiPlayerNames				= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_fetchAiPlayerNames",		PROCLUBSCUSTOM_FETCH_AI_PLAYER_NAMES);
			mCmdFetchAvatarName					= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_fetchAvatarName",			PROCLUBSCUSTOM_FETCH_AVATAR_NAME);
			mCmdFetchAvatarData					= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_fetchAvatarData",			PROCLUBSCUSTOM_FETCH_AVATAR_DATA);
			mCmdInsertAvatarName				= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_insertAvatarName",			PROCLUBSCUSTOM_INSERT_AVATAR_NAME);
			mCmdFlagProfaneAvatarName			= gDbScheduler->registerPreparedStatement(dbId, "proclubscustom_flagProfaneAvatarName",		PROCLUBSCUSTOM_FLAG_PROFANE_AVATAR_NAME);
       }
        
        /*************************************************************************************************/
        /*!
            \brief setDbConnection

            Sets the database connection to a new instance.
        */
        /*************************************************************************************************/
        BlazeRpcError ProclubsCustomDb::setDbConnection(DbConnPtr dbConn)
        { 
			BlazeRpcError error = ERR_OK;
            if (dbConn == NULL) 
            {
				TRACE_LOG("[ProclubsCustomDb].setDbConnection() - NULL dbConn passed in");
				error = PROCLUBSCUSTOM_ERR_DB;
            }
            else
            {
                mDbConn = dbConn;
            }

			return error;
        }

		/*************************************************************************************************/
		/*!
			\brief fetchSetting

			Retrieves vpro load outs for a multiple users
		*/
		/*************************************************************************************************/
		BlazeRpcError ProclubsCustomDb::fetchSetting(uint64_t clubId, Blaze::proclubscustom::customizationSettings &customizationSettings)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			if (mDbConn != NULL)
			{
				PreparedStatement *statement = getStatementFromId(mCmdFetchSettings);
				executeStatementError = mStatementError;
				if (nullptr != statement && ERR_OK == executeStatementError)
				{
					statement->setUInt64(0, clubId);
					
					DbResultPtr dbResult;
					executeStatementError = executeStatement(statement, dbResult);
					if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
					{
						DbResult::const_iterator itr = dbResult->begin();
						DbResult::const_iterator itr_end = dbResult->end();
						int rowCount = 0;
						for (; itr != itr_end; ++itr)
						{
							rowCount++;
							if (rowCount > 1)
							{
								WARN_LOG("[ProclubsCustomDb].fetchSetting() - WARNING: returned more than one row (" << rowCount <<")");
								break;
							}
							DbRow* thisRow = (*itr);

							customizationSettings.setClubId(clubId);
							customizationSettings.setCustomkit1(thisRow->getInt("customkit1"));
							customizationSettings.setCustomkit1PrimaryColour(thisRow->getInt("customkit1PrimaryColour"));
							customizationSettings.setCustomkit1SecondaryColour(thisRow->getInt("customkit1SecondaryColour"));
							customizationSettings.setCustomkit1TertiaryColour(thisRow->getInt("customkit1TertiaryColour"));
							customizationSettings.setCustomkit2(thisRow->getInt("customkit2"));
							customizationSettings.setCustomkit2PrimaryColour(thisRow->getInt("customkit2PrimaryColour"));
							customizationSettings.setCustomkit2SecondaryColour(thisRow->getInt("customkit2SecondaryColour"));
							customizationSettings.setCustomkit2TertiaryColour(thisRow->getInt("customkit2TertiaryColour"));
							customizationSettings.setCrestId(thisRow->getInt("crestId"));
							customizationSettings.setCrowdChantId(thisRow->getInt("crowdChantId"));
							customizationSettings.setCommentaryId(thisRow->getInt("commentaryId"));
							customizationSettings.setGoalMusicId(thisRow->getInt("goalMusicId"));
							customizationSettings.setHomeStadiumId(thisRow->getInt("homeStadiumId"));
							customizationSettings.setThemeId(thisRow->getInt("themeId"));
							customizationSettings.setPitchPatternId(thisRow->getInt("pitchPatternId"));
							customizationSettings.setPitchLineColour(thisRow->getInt("pitchLineColour"));
							customizationSettings.setNetTensionType(thisRow->getInt("netTensionType"));
							customizationSettings.setNetShapeType(thisRow->getInt("netShapeType"));
							customizationSettings.setNetStyleType(thisRow->getInt("netStyleType"));
							customizationSettings.setNetMeshType(thisRow->getInt("netMeshType"));
							customizationSettings.setFlameThrower(thisRow->getInt("flameThrower"));
							customizationSettings.setBallId(thisRow->getInt("ballId"));
							customizationSettings.setStadiumColorId(thisRow->getInt("stadiumColorId"));
							customizationSettings.setSeatColorId(thisRow->getInt("seatColorId"));
							customizationSettings.setTifoId(thisRow->getInt("tifoId"));
							customizationSettings.setExtra5(thisRow->getInt("extra5"));
						}
					}
				}

			}			
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief fetchLoadOuts

			Retrieves vpro load outs for a single user
		*/
		/*************************************************************************************************/
		BlazeRpcError ProclubsCustomDb::updateSetting(uint64_t clubId, const Blaze::proclubscustom::customizationSettings &settings)
        {
			BlazeRpcError executeStatementError = ERR_OK;
			if (mDbConn != nullptr)
			{
				QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
				if (query != nullptr)
				{
					eastl::string insertStr;
					insertStr.clear();
					eastl::string valueStr;
					valueStr.clear();
					eastl::string updateStr;
					updateStr.clear();

					appendToStatementForInsert(insertStr, valueStr, updateStr, "clubId", clubId);

					if (settings.isCustomkit1Set())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit1", settings.getCustomkit1());
					}
					if (settings.isCustomkit1PrimaryColourSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit1PrimaryColour", settings.getCustomkit1PrimaryColour());
					}
					if (settings.isCustomkit1SecondaryColourSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit1SecondaryColour", settings.getCustomkit1SecondaryColour());
					}
					if (settings.isCustomkit1TertiaryColourSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit1TertiaryColour", settings.getCustomkit1TertiaryColour());
					}
					if (settings.isCustomkit2Set())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit2", settings.getCustomkit2());
					}
					if (settings.isCustomkit2PrimaryColourSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit2PrimaryColour", settings.getCustomkit1PrimaryColour());
					}
					if (settings.isCustomkit1SecondaryColourSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit2SecondaryColour", settings.getCustomkit2SecondaryColour());
					}
					if (settings.isCustomkit2TertiaryColourSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "customkit2TertiaryColour", settings.getCustomkit2TertiaryColour());
					}
					if (settings.isCrestIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "crestId", settings.getCrestId());
					}
					if (settings.isCrowdChantIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "crowdChantId", settings.getCrowdChantId());
					}
					if (settings.isCommentaryIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "commentaryId", settings.getCommentaryId());
					}
					if (settings.isGoalMusicIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "goalMusicId", settings.getGoalMusicId());
					}
					if (settings.isHomeStadiumIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "homeStadiumId", settings.getHomeStadiumId());
					}
					if (settings.isThemeIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "themeId", settings.getThemeId());
					}
					if (settings.isPitchPatternIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "pitchPatternId", settings.getPitchPatternId());
					}
					if (settings.isPitchLineColourSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "pitchLineColour", settings.getPitchLineColour());
					}
					if (settings.isNetTensionTypeSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "netTensionType", settings.getNetTensionType());
					}
					if (settings.isNetShapeTypeSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "netShapeType", settings.getNetShapeType());
					}
					if (settings.isNetStyleTypeSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "netStyleType", settings.getNetStyleType());
					}
					if (settings.isNetMeshTypeSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "netMeshType", settings.getNetMeshType());
					}
					if (settings.isFlameThrowerSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "flameThrower", settings.getFlameThrower());
					}
					if (settings.isBallIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "ballId", settings.getBallId());
					}
					if (settings.isSeatColorIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "stadiumColorId", settings.getStadiumColorId());
					}
					if (settings.isSeatColorIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "seatColorId", settings.getSeatColorId());
					}
					if (settings.isTifoIdSet())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "tifoId", settings.getTifoId());
					}
					if (settings.isExtra5Set())
					{
						appendToStatementForInsert(insertStr, valueStr, updateStr, "extra5", settings.getExtra5());
					}

					//pop the last comma
					if (!insertStr.empty())
					{
						insertStr.pop_back();
					}
					if (!valueStr.empty())
					{
						valueStr.pop_back();
					}
					if (!updateStr.empty())
					{
						updateStr.pop_back();
					}

					if (!insertStr.empty())
					{
						query->append(PROCLUBSCUSTOM_UPDATE_TEAM_SETTINGS);
						query->append("($s) VALUES ($s) ON DUPLICATE KEY UPDATE $s", insertStr.c_str(), valueStr.c_str(), updateStr.c_str());

						DbResultPtr dbResult;
						executeStatementError = mDbConn->executeQuery(query, dbResult);
					}
				}
				else
				{
					executeStatementError = PROCLUBSCUSTOM_ERR_DB;
				}
			}
			else 
			{
				executeStatementError = PROCLUBSCUSTOM_ERR_DB;
			}
            return executeStatementError;
        }

		void ProclubsCustomDb::appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const float value)
		{
			eastl::string tempStr;

			tempStr.sprintf(" `%s`,", insertLabel);
			insertStr.append(tempStr.c_str());
			tempStr.sprintf(" %f,", value);
			valueStr.append(tempStr.c_str());
			tempStr.sprintf(" `%s`=%f,", insertLabel, value);
			updateStr.append(tempStr.c_str());
		}

		void ProclubsCustomDb::appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const int32_t value)
		{
			eastl::string tempStr;

			tempStr.sprintf(" `%s`,", insertLabel);
			insertStr.append(tempStr.c_str());
			tempStr.sprintf(" %d,", value);
			valueStr.append(tempStr.c_str());
			tempStr.sprintf(" `%s`=%d,", insertLabel, value);
			updateStr.append(tempStr.c_str());
		}
		
		void ProclubsCustomDb::appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const uint64_t value)
		{
			eastl::string tempStr;

			tempStr.sprintf(" `%s`,", insertLabel);
			insertStr.append(tempStr.c_str());
			tempStr.sprintf(" %" PRIu64 ",", value);			
			valueStr.append(tempStr.c_str());
			tempStr.sprintf(" `%s`=%" PRIu64 ",", insertLabel, value);
			updateStr.append(tempStr.c_str());
		}

		void ProclubsCustomDb::appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const char8_t* value)
		{
			eastl::string tempStr;

			tempStr.sprintf(" `%s`,", insertLabel);
			insertStr.append(tempStr.c_str());
			tempStr.sprintf("\"%s\",", value);
			valueStr.append(tempStr.c_str());
			tempStr.sprintf(" `%s`=\"%s\",", insertLabel, value);
			updateStr.append(tempStr.c_str());
		}

		/*************************************************************************************************/
		/*!
			\brief fetchAIPlayerCustomization

			fetch AI Player customization morphs
		*/
		/*************************************************************************************************/
		BlazeRpcError ProclubsCustomDb::fetchAIPlayerCustomization(uint64_t clubId, EA::TDF::TdfStructVector < Blaze::proclubscustom::AIPlayerCustomization> &aiCustomizationList)
        {
			BlazeRpcError executeStatementError = ERR_OK;

			if (mDbConn != NULL)
			{
				PreparedStatement *statement = getStatementFromId(mCmdFetchAIPlayerCustomization);
				executeStatementError = mStatementError;
				if (nullptr != statement && ERR_OK == executeStatementError)
				{
					statement->setUInt64(0, clubId);

					DbResultPtr dbResult;
					executeStatementError = executeStatement(statement, dbResult);
					if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
					{
						DbResult::const_iterator itr = dbResult->begin();
						DbResult::const_iterator itr_end = dbResult->end();
						int rowCount = 0;
						for (; itr != itr_end; ++itr)
						{
							rowCount++;
							DbRow* thisRow = (*itr);

							AIPlayerCustomization *pAICustomization = aiCustomizationList.allocate_element();
							pAICustomization->setSlotId (thisRow->getInt("avatarId"));
							pAICustomization->setGender(thisRow->getInt("gender"));
							pAICustomization->setPriority(thisRow->getInt("priority"));
							pAICustomization->setEyebrowcode(thisRow->getInt("eyebrowcode"));
							pAICustomization->setEyecolorcode(thisRow->getInt("eyecolorcode"));
							pAICustomization->setFacialhaircolorcode(thisRow->getInt("facialhaircolorcode"));
							pAICustomization->setFacialhairtypecode(thisRow->getInt("facialhairtypecode"));
							pAICustomization->setHaircolorcode(thisRow->getInt("haircolorcode"));
							pAICustomization->setHairtypecode(thisRow->getInt("hairtypecode"));
							
							pAICustomization->setFirstname(thisRow->getString("firstname"));
							pAICustomization->setLastname(thisRow->getString("lastname"));
							pAICustomization->setCommonname(thisRow->getString("commonname"));
							pAICustomization->setKitname(thisRow->getString("kitname"));

							pAICustomization->setCommentaryid(thisRow->getInt("commentaryid"));
							pAICustomization->setNationality(thisRow->getInt("nationality"));

							pAICustomization->setPreferredfoot(thisRow->getInt("preferredfoot"));
														
							pAICustomization->setKitnumber(thisRow->getInt("kitnumber"));
							pAICustomization->setSkintonecode(thisRow->getInt("skintonecode"));
							pAICustomization->setShoetypecode(thisRow->getInt("shoetypecode"));
							pAICustomization->setShoecolorcode1(thisRow->getInt("shoecolorcode1"));
							pAICustomization->setShoecolorcode2(thisRow->getInt("shoecolorcode2"));
							pAICustomization->setShoedesigncode(thisRow->getInt("shoedesigncode"));
							pAICustomization->setGkglovetypecode(thisRow->getInt("gkglovetypecode"));
							pAICustomization->setSocklengthcode(thisRow->getInt("socklengthcode"));
							pAICustomization->setShortstyle(thisRow->getInt("shortstyle"));
							pAICustomization->setAnimfreekickstartposcode(thisRow->getInt("animfreekickstartposcode"));
							pAICustomization->setAnimpenaltiesstartposcode(thisRow->getInt("animpenaltiesstartposcode"));
							pAICustomization->setRunstylecode(thisRow->getInt("runstylecode"));
							pAICustomization->setRunningcode1(thisRow->getInt("runningcode1"));
							pAICustomization->setFinishingcode1(thisRow->getInt("finishingcode1"));
							pAICustomization->setJerseyfit(thisRow->getInt("jerseyfit"));
							pAICustomization->setHasseasonaljersey(thisRow->getInt("hasseasonaljersey"));
							pAICustomization->setJerseystylecode(thisRow->getInt("jerseystylecode"));
							pAICustomization->setJerseysleevelengthcode(thisRow->getInt("jerseysleevelengthcode"));
							pAICustomization->setAccessories_code_0(thisRow->getInt("accessories_code_0"));
							pAICustomization->setAccessories_code_1(thisRow->getInt("accessories_code_1"));
							pAICustomization->setAccessories_code_2(thisRow->getInt("accessories_code_2"));
							pAICustomization->setAccessories_code_3(thisRow->getInt("accessories_code_3"));
							pAICustomization->setAccessories_color_0(thisRow->getInt("accessories_color_0"));
							pAICustomization->setAccessories_color_1(thisRow->getInt("accessories_color_1"));
							pAICustomization->setAccessories_color_2(thisRow->getInt("accessories_color_2"));
							pAICustomization->setAccessories_color_3(thisRow->getInt("accessories_color_3"));
							pAICustomization->setYear(thisRow->getInt("year"));
							pAICustomization->setMonth(thisRow->getInt("month"));
							pAICustomization->setDay(thisRow->getInt("day"));

							for (int i = 0; i < NUM_MORPH_REGIONS; i++)
							{
								MorphRegion* morphRegion = pAICustomization->getMorphs().allocate_element();

								eastl::string columnName;
								columnName.sprintf("tweak_preset_%d", i);
								morphRegion->setPreset(thisRow->getInt(columnName.c_str()));

								columnName.sprintf("tweak_a_%d", i);
								morphRegion->setTweak_a(thisRow->getFloat(columnName.c_str()));

								columnName.sprintf("tweak_b_%d", i);
								morphRegion->setTweak_b(thisRow->getFloat(columnName.c_str()));

								columnName.sprintf("tweak_c_%d", i);
								morphRegion->setTweak_c(thisRow->getFloat(columnName.c_str()));

								columnName.sprintf("tweak_d_%d", i);
								morphRegion->setTweak_d(thisRow->getFloat(columnName.c_str()));

								columnName.sprintf("tweak_e_%d", i);
								morphRegion->setTweak_e(thisRow->getFloat(columnName.c_str()));

								pAICustomization->getMorphs().push_back(morphRegion);
							}


							aiCustomizationList.push_back(pAICustomization);
						}
					}
				}

			}
			return executeStatementError;
		}
		
		BlazeRpcError ProclubsCustomDb::updateAIPlayerCustomization(uint64_t clubId, int32_t slotId, const Blaze::proclubscustom::AIPlayerCustomization &aiPlayerCustomizationBulk, const Blaze::proclubscustom::AvatarCustomizationPropertyMap &modifiedItems)
		{
			BlazeRpcError executeStatementError = ERR_OK;
			if (mDbConn != nullptr)
			{
				QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
				if (query != nullptr)
				{
					eastl::string insertStr;
					insertStr.clear();
					eastl::string valueStr;
					valueStr.clear();
					eastl::string updateStr;
					updateStr.clear();

					appendToStatementForInsert(insertStr, valueStr, updateStr, "clubId", clubId);
					appendToStatementForInsert(insertStr, valueStr, updateStr, "avatarId", slotId);
					appendToStatementForInsert(insertStr, valueStr, updateStr, "gender", aiPlayerCustomizationBulk.getGender());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "priority", aiPlayerCustomizationBulk.getPriority());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "hairtypecode", aiPlayerCustomizationBulk.getHairtypecode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "haircolorcode", aiPlayerCustomizationBulk.getHaircolorcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "facialhairtypecode", aiPlayerCustomizationBulk.getFacialhairtypecode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "facialhaircolorcode", aiPlayerCustomizationBulk.getFacialhaircolorcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "eyebrowcode", aiPlayerCustomizationBulk.getEyebrowcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "eyecolorcode", aiPlayerCustomizationBulk.getEyecolorcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "lastname", aiPlayerCustomizationBulk.getLastname());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "firstname", aiPlayerCustomizationBulk.getFirstname());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "commonname", aiPlayerCustomizationBulk.getCommonname());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "kitname", aiPlayerCustomizationBulk.getKitname());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "kitnumber", aiPlayerCustomizationBulk.getKitnumber());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "skintonecode", aiPlayerCustomizationBulk.getSkintonecode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "shoetypecode", aiPlayerCustomizationBulk.getShoetypecode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "commentaryid", aiPlayerCustomizationBulk.getCommentaryid());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "nationality", aiPlayerCustomizationBulk.getNationality());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "preferredfoot", aiPlayerCustomizationBulk.getPreferredfoot());
					
					appendToStatementForInsert(insertStr, valueStr, updateStr, "shoecolorcode1", aiPlayerCustomizationBulk.getShoecolorcode1());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "shoecolorcode2", aiPlayerCustomizationBulk.getShoecolorcode2());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "shoedesigncode", aiPlayerCustomizationBulk.getShoedesigncode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "gkglovetypecode", aiPlayerCustomizationBulk.getGkglovetypecode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "socklengthcode", aiPlayerCustomizationBulk.getSocklengthcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "shortstyle", aiPlayerCustomizationBulk.getShortstyle());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "animfreekickstartposcode", aiPlayerCustomizationBulk.getAnimfreekickstartposcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "animpenaltiesstartposcode", aiPlayerCustomizationBulk.getAnimpenaltiesstartposcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "runstylecode", aiPlayerCustomizationBulk.getRunstylecode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "runningcode1", aiPlayerCustomizationBulk.getRunningcode1());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "finishingcode1", aiPlayerCustomizationBulk.getFinishingcode1());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "jerseyfit", aiPlayerCustomizationBulk.getJerseyfit());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "hasseasonaljersey", aiPlayerCustomizationBulk.getHasseasonaljersey());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "jerseystylecode", aiPlayerCustomizationBulk.getJerseystylecode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "jerseysleevelengthcode", aiPlayerCustomizationBulk.getJerseysleevelengthcode());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_code_0", aiPlayerCustomizationBulk.getAccessories_code_0());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_code_1", aiPlayerCustomizationBulk.getAccessories_code_1());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_code_2", aiPlayerCustomizationBulk.getAccessories_code_2());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_code_3", aiPlayerCustomizationBulk.getAccessories_code_3());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_color_0", aiPlayerCustomizationBulk.getAccessories_color_0());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_color_1", aiPlayerCustomizationBulk.getAccessories_color_1());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_color_2", aiPlayerCustomizationBulk.getAccessories_color_2());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "accessories_color_3", aiPlayerCustomizationBulk.getAccessories_color_3());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "year", aiPlayerCustomizationBulk.getYear());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "month", aiPlayerCustomizationBulk.getMonth());
					appendToStatementForInsert(insertStr, valueStr, updateStr, "day", aiPlayerCustomizationBulk.getDay());

					for (int i = 0; i < NUM_MORPH_REGIONS; i++)
					{
						eastl::string columnName;
						columnName.sprintf("tweak_preset_%d", i);
						appendToStatementForInsert(insertStr, valueStr, updateStr, columnName.c_str(), aiPlayerCustomizationBulk.getMorphs()[i]->getPreset());

						columnName.sprintf("tweak_a_%d", i);
						appendToStatementForInsert(insertStr, valueStr, updateStr, columnName.c_str(), aiPlayerCustomizationBulk.getMorphs()[i]->getTweak_a());

						columnName.sprintf("tweak_b_%d", i);
						appendToStatementForInsert(insertStr, valueStr, updateStr, columnName.c_str(), aiPlayerCustomizationBulk.getMorphs()[i]->getTweak_b());

						columnName.sprintf("tweak_c_%d", i);
						appendToStatementForInsert(insertStr, valueStr, updateStr, columnName.c_str(), aiPlayerCustomizationBulk.getMorphs()[i]->getTweak_c());

						columnName.sprintf("tweak_d_%d", i);
						appendToStatementForInsert(insertStr, valueStr, updateStr, columnName.c_str(), aiPlayerCustomizationBulk.getMorphs()[i]->getTweak_d());

						columnName.sprintf("tweak_e_%d", i);
						appendToStatementForInsert(insertStr, valueStr, updateStr, columnName.c_str(), aiPlayerCustomizationBulk.getMorphs()[i]->getTweak_e());
					}

					//pop the last comma
					if (!insertStr.empty())
					{
						insertStr.pop_back();
					}
					if (!valueStr.empty())
					{
						valueStr.pop_back();
					}
					if (!updateStr.empty())
					{
						updateStr.pop_back();
					}

					if (!insertStr.empty())
					{
						query->append(PROCLUBSCUSTOM_INSERT_AI_PLAYER_SETTINGS);
						query->append("($s) VALUES ($S) ON DUPLICATE KEY UPDATE $S", insertStr.c_str(), valueStr.c_str(), updateStr.c_str());

						DbResultPtr dbResult;
						executeStatementError = mDbConn->executeQuery(query, dbResult);
					}
				}
				else
				{
					executeStatementError = PROCLUBSCUSTOM_ERR_DB;
				}
			}
			else
			{
				executeStatementError = PROCLUBSCUSTOM_ERR_DB;
			}
			return executeStatementError;
			
		}
		
		/*************************************************************************************************/
		/*!
			\brief fetchCustomTactics

			fetch custom tactics
		*/
		/*************************************************************************************************/
		BlazeRpcError ProclubsCustomDb::fetchCustomTactics(uint64_t clubId, EA::TDF::TdfStructVector <Blaze::proclubscustom::CustomTactics> &customTactics)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdFetchCustomTactics);
			executeStatementError = mStatementError;
			if (NULL != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, clubId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			
				if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
				{
					DbResult::const_iterator itr = dbResult->begin();
					DbResult::const_iterator itr_end = dbResult->end();
					
					for (; itr != itr_end; ++itr)
					{
						DbRow* thisRow = (*itr);

						size_t len;
						const uint32_t slotId = thisRow->getUInt("slotId");
						const uint8_t* data = thisRow->getBinary("tacticsSlotData", &len);

						CustomTactics *pTempTactics = customTactics.allocate_element();
						pTempTactics->setTacticSlotId(slotId);
						pTempTactics->getTacticsDataBlob().setData(data, len);
						
						customTactics.push_back(pTempTactics);
					}
				}
			}

			return executeStatementError;
		}

		BlazeRpcError ProclubsCustomDb::updateCustomTactics(uint64_t clubId, uint32_t slotId, const Blaze::proclubscustom::CustomTactics &customTactics)
		{
			BlazeRpcError executeStatementError = PROCLUBSCUSTOM_ERR_DB;
			PreparedStatement *statement = getStatementFromId(mCmdInsertCustomTactics);

			if (statement != nullptr)
			{
				statement->setInt64(0, clubId);
				statement->setInt32(1, slotId);
				statement->setBinary(2, customTactics.getTacticsDataBlob().getData(), customTactics.getTacticsDataBlob().getSize());

				DbResultPtr dbResultInsert;
				executeStatementError = executeStatement(statement, dbResultInsert);
			}

			return executeStatementError;
		}

		BlazeRpcError ProclubsCustomDb::resetAiPlayerNames(uint64_t clubId, eastl::string& statusMessage)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdResetAiPlayerNames);

			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, clubId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);

				if (dbResult == nullptr)
				{
					executeStatementError = ERR_SYSTEM;
				}
				else if (dbResult->getAffectedRowCount() != 0)  // a successful edit will involve one or more rows
				{
					statusMessage.append("AI Players Reported");
				}
			}

			return executeStatementError;
		}

		BlazeRpcError ProclubsCustomDb::fetchAiPlayerNames(uint64_t clubId, eastl::string &playerNames)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdFetchAiPlayerNames);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, clubId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);

				if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
				{
					DbResult::const_iterator itr = dbResult->begin();
					DbResult::const_iterator itr_end = dbResult->end();

					for (; itr != itr_end; ++itr)
					{
						DbRow* thisRow = (*itr);

						const char8_t* firstName = thisRow->getString("firstname");
						const char8_t* lastName = thisRow->getString("lastname");

						eastl::string playerNameStr;
						playerNameStr.sprintf("|%s %s|", firstName, lastName );
						playerNames.append(playerNameStr.c_str());
					}
				}
			}

			return executeStatementError;
		}

		BlazeRpcError ProclubsCustomDb::fetchAvatarData(int64_t blazeId, Blaze::proclubscustom::AvatarData &avatarData, uint32_t maxAvatarNameResets)
		{
			PreparedStatement *statement = getStatementFromId(mCmdFetchAvatarData);
			BlazeRpcError executeStatementError = mStatementError;

			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, blazeId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);

				if (ERR_OK == executeStatementError && dbResult != nullptr)
				{
					if (!dbResult->empty())
					{
						DbResult::const_iterator itr = dbResult->begin();
						DbResult::const_iterator itr_end = dbResult->end();

						for (; itr != itr_end; ++itr)
						{
							DbRow* thisRow = (*itr);

							uint32_t resetCount = thisRow->getUInt("avatarNameResetCount");

							avatarData.setIsProfane(thisRow->getBool("isNameProfane"));
							avatarData.setIsNameEditable(resetCount <= maxAvatarNameResets);
						}
					}
					else
					{
						//If we don't find any entry return a non blocking result
						avatarData.setIsProfane(false);
						avatarData.setIsNameEditable(true);
					}
				}
			}

			return executeStatementError;
		}

		BlazeRpcError ProclubsCustomDb::fetchAvatarName(int64_t blazeId, eastl::string &avatarFirstName, eastl::string &avatarLastName)
		{
			PreparedStatement *statement = getStatementFromId(mCmdFetchAvatarName);
			BlazeRpcError executeStatementError = mStatementError;

			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, blazeId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);

				if (ERR_OK == executeStatementError && dbResult != nullptr)
				{
					if (!dbResult->empty())
					{
						DbResult::const_iterator itr = dbResult->begin();
						DbResult::const_iterator itr_end = dbResult->end();

						for (; itr != itr_end; ++itr)
						{
							DbRow* thisRow = (*itr);

							avatarFirstName = thisRow->getString("firstname");
							avatarLastName = thisRow->getString("lastname");
						}
					}
					else
					{
						//Avatar firstname and lastname not found in DB
						executeStatementError = ERR_DB_NO_ROWS_AFFECTED;
					}
				}
			}

			return executeStatementError;
		}

		BlazeRpcError ProclubsCustomDb::insertAvatarName(int64_t blazeId, eastl::string &avatarFirstName, eastl::string &avatarLastName)
		{
			PreparedStatement *statement = getStatementFromId(mCmdInsertAvatarName);
			BlazeRpcError executeStatementError = mStatementError;

			if (statement != nullptr)
			{
				statement->setInt64(0, blazeId);
				statement->setString(1, avatarFirstName.c_str());
				statement->setString(2, avatarLastName.c_str());

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);

				if (dbResult == nullptr)
				{
					executeStatementError = PROCLUBSCUSTOM_ERR_DB;
				}
			}

			return executeStatementError;
		}

		BlazeRpcError ProclubsCustomDb::flagProfaneAvatarName(int64_t blazeId)
		{
			PreparedStatement *statement = getStatementFromId(mCmdFlagProfaneAvatarName);
			BlazeRpcError executeStatementError = mStatementError;

			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, blazeId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);

				if (dbResult == nullptr) 
				{
					executeStatementError = ERR_SYSTEM;
				}
				else if (dbResult->getAffectedRowCount() == 0) //a successful edit will involve one or more rows being affected
				{
					executeStatementError = PROCLUBSCUSTOM_ERR_DB;
				}
			}

			return executeStatementError;
		}


		/*************************************************************************************************/
        /*!
            \brief getStatementFromId

            Helper which returns the statement from the dbConn (if available).
        */
        /*************************************************************************************************/
        PreparedStatement* ProclubsCustomDb::getStatementFromId(PreparedStatementId id)
        {
            PreparedStatement* thisStatement = NULL;
            if (mDbConn != NULL)
            {
                thisStatement = mDbConn->getPreparedStatement(id);
                if (NULL == thisStatement)
                {
					ASSERT_LOG("[ProclubsCustomDb].getStatementFromId() no valid statement found");
                    mStatementError = PROCLUBSCUSTOM_ERR_DB;
                }
            }
            else
            {
				ASSERT_LOG("[ProclubsCustomDb].getStatementFromId() no dbConn specified!");
                mStatementError = ERR_SYSTEM;
            }
            return thisStatement;
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatement

            Helper which attempts to execute the prepared statement.
        */
        /*************************************************************************************************/
        BlazeRpcError ProclubsCustomDb::executeStatement(PreparedStatement* statement, DbResultPtr& result)
        {
            BlazeRpcError statementError = ERR_OK;

            if (mDbConn != NULL)
            {
                if (NULL != statement)
                {
                    eastl::string queryDisplayBuf;
                    TRACE_LOG("[ProclubsCustomDb].executeStatement() trying statement "<<statement<<" @ dbConn "<<mDbConn->getName() <<":");
                    TRACE_LOG("\t"<<statement->toString(queryDisplayBuf) <<" ");

                    BlazeRpcError dbError = mDbConn->executePreparedStatement(statement, result);
                    switch (dbError)
                    {
                        case DB_ERR_OK:
                            break;
                        case DB_ERR_DISCONNECTED:
                        case DB_ERR_NO_SUCH_TABLE:
                        case DB_ERR_NOT_SUPPORTED:
                        case DB_ERR_INIT_FAILED:
                        case DB_ERR_TRANSACTION_NOT_COMPLETE:
                            ERR_LOG("[ProclubsCustomDb].executeStatement() a database error: " << dbError << " has occured");
                            statementError = PROCLUBSCUSTOM_ERR_DB;
                            break;
                        default:
                            ERR_LOG("[ProclubsCustomDb].executeStatement() a general error has occured");
                            statementError = PROCLUBSCUSTOM_ERR_UNKNOWN;
                            break;
                    }
                }
                else
                {
					ASSERT_LOG("[ProclubsCustomDb].executeStatement() - invalid prepared statement!");
                    statementError = PROCLUBSCUSTOM_ERR_UNKNOWN;
                }
            }
            else
            {
                ERR_LOG("[ProclubsCustomDb].executeStatement() no database connection available");
                statementError = PROCLUBSCUSTOM_ERR_DB;
            }

            if (ERR_OK == statementError)
            {
                TRACE_LOG("[ProclubsCustomDb].executeStatement() statement "<< statement<<" @ dbConn "<<mDbConn->getName()<<" successful!");
            }

			return statementError;
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatement

            Helper which attempts to execute the prepared statement registered by the supplied id.
        */
        /*************************************************************************************************/
        BlazeRpcError ProclubsCustomDb::executeStatement(PreparedStatementId statementId, DbResultPtr& result)
        {
			BlazeRpcError statementError = ERR_OK;

            PreparedStatement* statement = getStatementFromId(statementId);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statementError = executeStatement(statement, result);
            }

			return statementError;
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatementInTxn

            Helper which attempts to execute the prepared statement in a transaction
        */
        /*************************************************************************************************/
        BlazeRpcError ProclubsCustomDb::executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result)
        {
			BlazeRpcError statementError = ERR_OK;

            if (mDbConn != NULL)
            {
                // start the database transaction
                BlazeRpcError dbError = mDbConn->startTxn();
                if (dbError == DB_ERR_OK)
                {
                    // execute the prepared statement
                    statementError = executeStatement(statement, result);

                    if (ERR_OK == statementError)
                    {
                        // try committing the transaction
                        dbError = mDbConn->commit();
                        if (dbError != DB_ERR_OK)
                        {
                            dbError = mDbConn->rollback();
							if (dbError != DB_ERR_OK)
							{
								ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() unable to roll back after fail to commit");
							}
                        }
                    }
                    else
                    {
                        // roll back if statementError doesn't indicate things are good
                        dbError = mDbConn->rollback();
						if (dbError != DB_ERR_OK)
						{
							ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() unable to roll back");
						}
                    }
                }
                else
                {
                    ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() unable to start db transaction");
                }
            }
            else
            {
                ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() no database connection available");
                statementError = PROCLUBSCUSTOM_ERR_DB;
            }
			
			return statementError;
        }

    } // ProclubsCustomDb
} // Blaze
