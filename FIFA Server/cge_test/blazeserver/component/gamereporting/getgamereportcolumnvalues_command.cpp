/*************************************************************************************************/
/*!
    \file   getgamereportviewinfo_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetGameReportViewInfo

    Retrieves game report view info allowing the client to explicitly specify view name.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/util/localization.h"
#include "framework/identity/identity.h"

// gamereporting includes
#include "gamereporting/tdf/gamehistory.h"
#include "gamereportingslaveimpl.h"
#include "gamereporting/rpc/gamereportingslave/getgamereportcolumnvalues_stub.h"
#include "gamehistoryconfig.h"

#include "stats/statstypes.h"

namespace Blaze
{
namespace GameReporting
{

    class GetGameReportColumnValuesCommand : public GetGameReportColumnValuesCommandStub
    {
    public:
        GetGameReportColumnValuesCommand(Message* message, GetGameReportViewRequest* request, GameReportingSlaveImpl* componentImpl);
        ~GetGameReportColumnValuesCommand() override {}

    private:
        GameReportingSlaveImpl* mComponent;
        GetGameReportColumnValuesCommandStub::Errors execute() override;        

    };
    DEFINE_GETGAMEREPORTCOLUMNVALUES_CREATE()

    GetGameReportColumnValuesCommand::GetGameReportColumnValuesCommand(Message* message, GetGameReportViewRequest* request, GameReportingSlaveImpl* componentImpl)
        : GetGameReportColumnValuesCommandStub(message, request),
        mComponent(componentImpl)
    {
    }


    GetGameReportColumnValuesCommandStub::Errors GetGameReportColumnValuesCommand::execute()
    {
        //Check if we trust this user
        if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_GET_GAMEREPORT_INFORMATION))
        {
            WARN_LOG("[GetGameReportColumnValuesCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to get report info ["<< mRequest.getName() << "], no permission!");
            return ERR_AUTHORIZATION_REQUIRED;
        }

        const GameReportViewConfig* viewConfig = mComponent->getGameReportViewConfig(mRequest.getName());
        if (viewConfig == nullptr)
        {
            return GAMEHISTORY_ERR_UNKNOWN_VIEW;
        }

        const GameReportView* view = viewConfig->getGameReportView();
        const char8_t* gameTypeName = view->getViewInfo().getTypeName();

        // start the query by getting a connection.
        DbConnPtr conn = mComponent->getDbReadConnPtr(gameTypeName);
        if (conn == nullptr)
        {
            ERR_LOG("[GetGameReportViewCommand.execute(): Failed to obtain connection.");
            return ERR_SYSTEM;
        }

        if (mRequest.getQueryVarValues().size() < viewConfig->getMaxVar())
        {
            ERR_LOG("[GetGameReportViewCommand.execute(): Number of query variables provided is not enough.");
            return GAMEHISTORY_ERR_MISSING_QVARS;
        }

        // set the max number of rows to return
        uint32_t maxRowsToReturn = view->getMaxGames();
        if ((mRequest.getMaxRows() > 0) && (maxRowsToReturn > mRequest.getMaxRows()))
        {
            maxRowsToReturn = mRequest.getMaxRows();
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);


        query->append(viewConfig->getQueryStringPrependIndex0Filter());

        QueryVarValuesList::const_iterator qIter, qEnd;
        qEnd = mRequest.getQueryVarValues().end();
        GameReportFilterList::const_iterator fIter, fEnd;
        fEnd = view->getFilterList().end();

        bool invalidValueFound = false;
        bool needAndOperator = false;

        for (uint32_t index = 0; index <= viewConfig->getMaxIndex() && !invalidValueFound; ++index)
        {
            qIter = mRequest.getQueryVarValues().begin();
            fIter = view->getFilterList().begin();

            if (index > 0)
            {
                query->append(viewConfig->getQueryStringPrependIndex1Filter(), maxRowsToReturn);
                needAndOperator = true;
            }

            for (; fIter != fEnd && qIter != qEnd; ++fIter)
            {
                if ((*fIter)->getHasVariable() && (*fIter)->getIndex() == index)
                {
                    if (!validateQueryVarValue((*qIter).c_str()))
                    {
                        ERR_LOG("[GetGameReportViewCommand].execute(): Invalid value in one of the provided query variable.");
                        invalidValueFound = true;
                        break;
                    }

                    (needAndOperator) && (query->append(" AND "));

                    if ((*fIter)->getEntityType() == EA::TDF::OBJECT_TYPE_INVALID)
                    {
                        query->append((*fIter)->getExpression(), qIter->c_str());
                    }
                    else
                    {
                        invalidValueFound = true;
                        EntityId id;
                        if (blaze_str2int(qIter->c_str(), &id) != qIter->c_str())
                        {
                            EA::TDF::ObjectId bobjId((*fIter)->getEntityType(), id);
                            const eastl::string expandedList = expandBlazeObjectId(bobjId).c_str();
                            if (!expandedList.empty())
                            {
                                invalidValueFound = false;
                                query->append((*fIter)->getExpression(), expandedList.c_str());
                            }
                        }

                        if (invalidValueFound)
                        {
                            ERR_LOG("[GetGameReportsCommand].execute(): Failed to convert '" << qIter->c_str() << "' to blaze object id of type " 
                                << (*fIter)->getEntityType().toString().c_str() << ".");
                            break;
                        }
                    }

                    needAndOperator = true;
                    ++qIter;
                }
            }
        }

        if (invalidValueFound)
        {
            return GAMEHISTORY_ERR_INVALID_QVARS;
        }

        query->append(viewConfig->getQueryStringAppendFilter(), maxRowsToReturn);

        DbResultPtr result;
        BlazeRpcError dberr = conn->executeQuery(query, result);

        if (dberr != DB_ERR_OK)
        {
            TRACE_LOG("[GetGameReportViewCommand].execute(): Error executing query: " << getDbErrorString(dberr) << ".");
            return GAMEHISTORY_ERR_INVALID_QUERY;
        }

        const GameReportView::GameReportColumnList& columnList = viewConfig->getGameReportView()->getColumns();
        GameReportView::GameReportColumnList::const_iterator begin, iter, end;
        begin = columnList.begin();
        iter = columnList.begin();
        end = columnList.end();

        const char8_t* typeName = view->getViewInfo().getTypeName();
        const GameType *gameType = mComponent->getGameTypeCollection().getGameType(typeName);
        const HistoryTableDefinitions &historyTableDef = gameType->getHistoryTableDefinitions();
        const HistoryTableDefinitions &builtInTableDef = gameType->getBuiltInTableDefinitions();
        const HistoryTableDefinitions &gameKeyTableDef = gameType->getGamekeyTableDefinitions();

        mResponse.getColumnValues().reserve(columnList.size());

        for (uint32_t i = 1; iter != end; ++iter, ++i)
        {
            GameReportColumnValues* column = (mResponse.getColumnValues()).pull_back(); 
            const GameReportColumn *configColumn = (*iter);

            if (!result->empty())
            {
                EntityIdList keys;
                IdentityInfoByEntityIdMap identityMap;

                DbResult::const_iterator rowIter = result->begin();
                DbResult::const_iterator rowEnd = result->end();

                for (; rowIter != rowEnd; ++rowIter)
                {
                    const DbRow *row = *rowIter;

                    if (iter == begin)
                        mResponse.getEntityIds().push_back(static_cast<EntityId>(row->getInt64((uint32_t)0)));

                    if (configColumn->getEntityType() != EA::TDF::OBJECT_TYPE_INVALID)
                    {
                        keys.push_back(row->getInt64(i));
                    }
                    else
                    {
                        HistoryTableDefinition::ColumnList::const_iterator colIter;
                        char8_t asStr[Collections::MAX_ATTRIBUTEVALUE_LEN];
                        HistoryTableDefinitions::const_iterator tableDef = historyTableDef.find(configColumn->getKey().getTable());
                        if (tableDef == historyTableDef.end())
                        {
                            tableDef = builtInTableDef.find(configColumn->getKey().getTable());
                            if (tableDef == builtInTableDef.end())
                            {
                                tableDef = gameKeyTableDef.find(configColumn->getKey().getTable());
                                if (tableDef == gameKeyTableDef.end())
                                {
                                    column->getValues().push_back(asStr);
                                    continue;
                                }
                            }
                        }

                        const char8_t* attributeName = configColumn->getKey().getAttributeName();
                        colIter = tableDef->second->getColumnList().find(attributeName);
                        if (colIter == tableDef->second->getColumnList().end())
                        {
                            if (blaze_strcmp(attributeName, "timestamp") == 0)
                            {
                                blaze_snzprintf(asStr, sizeof(asStr), "%" PRId64, row->getInt64(i));
                            }
                            else if (blaze_strcmp(attributeName, "game_id") == 0)
                            {
                                blaze_snzprintf(asStr, sizeof(asStr), "%" PRId64, row->getUInt64(i));
                            }
                        }
                        else
                        {  
                            switch (colIter->second)
                            {
                                case HistoryTableDefinition::INT:
                                    blaze_snzprintf(asStr, sizeof(asStr), "%d", row->getInt(i));
                                    break;
                                case HistoryTableDefinition::UINT:
                                    blaze_snzprintf(asStr, sizeof(asStr), "%u", row->getUInt(i));
                                    break;
                                case HistoryTableDefinition::ENTITY_ID:
                                    blaze_snzprintf(asStr, sizeof(asStr), "%" PRId64, row->getInt64(i));
                                    break;
                                case HistoryTableDefinition::FLOAT:
                                    blaze_snzprintf(asStr, sizeof(asStr), "%.2f", (double_t) row->getFloat(i));
                                    break;
                                case HistoryTableDefinition::STRING:
                                    blaze_snzprintf(asStr, sizeof(asStr), "%s", row->getString(i));
                                    break;
                                default:
                                    break;
                            }
                        }
                        column->getValues().push_back(asStr);
                    }
                }

                if (configColumn->getEntityType() != EA::TDF::OBJECT_TYPE_INVALID)
                {
                    BlazeIdToUserInfoMap map;
                    const char *localizedUnknownValue = gLocalization->localize(configColumn->getUnknownValue(), gCurrentUserSession->getSessionLocale());
                    bool hasCoreIdent = configColumn->getEntityType() == Blaze::ENTITY_TYPE_USER && configColumn->getUserCoreIdentName()[0] != '\0';
                    if (hasCoreIdent)
                    {
                        BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeIds(keys, map);
                        if (err != Blaze::ERR_OK)
                        {       
                            WARN_LOG("[GetGameReportColumnValuesCommand] ignoring " << (ErrorHelp::getErrorName(err)) << " error looking up users ids");
                        }
                    }
                    else 
                    {
                        // ignore the error code such that we can return the rest of identity names even if one of the entity ids is invalid
                        BlazeRpcError err = gIdentityManager->getIdentities(configColumn->getEntityType(), keys, identityMap);
                        if (err != Blaze::ERR_OK)
                        {
                            WARN_LOG("[GetGameReportColumnValuesCommand] ignoring " << (ErrorHelp::getErrorName(err)) << " error getting identity names");
                        }
                    }

                    IdentityInfoByEntityIdMap::const_iterator identEnd = identityMap.end();
                    for (rowIter = result->begin(); rowIter != rowEnd; ++rowIter)
                    {
                        const DbRow *row = *rowIter;

                        const char * columnValue = nullptr;
                        eastl::string valueTemp;
                        if (hasCoreIdent)
                        {
                            BlazeIdToUserInfoMap::iterator mapItr = map.find(row->getInt64(i));
                            if (mapItr != map.end())
                            {
                                valueTemp = mComponent->populateUserCoreIdentHelper(mapItr->second, configColumn->getUserCoreIdentName()).c_str();
                                columnValue = valueTemp.c_str();
                            }
                        }
                        else
                        {
                            IdentityInfoByEntityIdMap::const_iterator identItr = identityMap.find(row->getInt64(i));
                            if (identItr != identEnd)
                                columnValue = identItr->second->getIdentityName();
                        }
                        if (columnValue == nullptr)
                            columnValue = localizedUnknownValue;
                        
                        column->getValues().push_back(columnValue);
                    }
                }
            }
        }

        return ERR_OK;
    }
   
} // GameReporting
} // Blaze

