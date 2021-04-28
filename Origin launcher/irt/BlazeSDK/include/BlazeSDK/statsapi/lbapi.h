/*************************************************************************************************/
/*!
    \file lbapi.h

    Leaderboard API header.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_LEADERBOARD_API
#define BLAZE_LEADERBOARD_API
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/component/stats/tdf/stats.h"
#include "BlazeSDK/component/statscomponent.h"
#include "BlazeSDK/statsapi/lbapilistener.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/apijob.h"

#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "EASTL/functional.h"
#include "BlazeSDK/blaze_eastl/vector.h"

namespace Blaze
{
    namespace Stats
    {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class Leaderboard;
class LeaderboardFolder;
class LeaderboardView;
class LeaderboardTree;

/*! ***************************************************************/
/*! \brief Container for a list of Leaderboard objects.  

    This may hang off of the top-level Leaderboard system or as a list 
    of children for a parent LeaderboardFolder.
*******************************************************************/
typedef vector<Leaderboard*> LeaderboardList;

/*! ***************************************************************/
/*! \brief Container for a list of LeaderboardFolder objects.  

    This may hang off of the top-level Leaderboard system or as a list 
    of sub-folders for a parent LeaderboardFolder.
*******************************************************************/
typedef vector<LeaderboardFolder*> LeaderboardFolderList;

/*! ***************************************************************/
/*! \brief Callback for retrieving the contents of a LeaderboardFolder 
        from the server (or cache.)
*******************************************************************/
typedef Functor3<BlazeError, JobId, LeaderboardFolder* > GetLeaderboardFolderCb;

/*! ***************************************************************/
/*! \brief Callback for retrieving the contents of a Leaderboard from 
        the server (or cache.)
*******************************************************************/
typedef Functor3<BlazeError, JobId, Leaderboard* > GetLeaderboardCb;

/*! ***************************************************************/
/*! \brief Callback for retrieving the Leaderboard tree from 
        the server (or cache.)
*******************************************************************/
typedef Functor3<BlazeError, JobId, LeaderboardTree* > GetLeaderboardTreeCb;

/*! ***************************************************************/
/*! \class LeaderboardAPI
    \ingroup _mod_apis

    \brief Provides retrieval and caching of leaderboards from the Blaze server.

    The LeaderboardAPI is the starting point for retrieving leaderboards from the Blaze server. 
    Leaderboards are ordered lists representing the highest/lowest values for a particular statistic and are defined
    in the Blaze server configurations. 
    Each leaderboard contains data for a particular type of entity, such as users or clubs.
    Each row of the leaderboard represents data for a particular single entity.
    The rows of the leaderboard can also contain additional statistic data other than the value used to sort the leaderboard. 

    The LeaderboardAPI acts as a factory for creating Leaderboard objects which represent a specific leaderboard 
    defined in the Blaze server configurations. The first time a particular Leaderboard is requested, LeaderboardAPI 
    will retrieve the definition information regarding that Leaderboard from the Blaze server. For subsequent requests 
    for the Leaderboard object, the cached information will be used and a network transaction is unnecessary.

    To help facilitate construction of UI to navigate a large number of Leaderboard objects, the Blaze server configuration
    allow for the definition of a hierarchy of folders to logically organize the Leaderboards. The LeaderboardAPI acts as 
    a factory for LeaderboardFolder and LeaderboardFolderList objects. Each LeaderboardFolder has a list of contained Leaderboard 
    definitions and/or other nested LeaderboardFolder objects. Similar to the construction of Leaderboard objects, 
    a network transaction is only necessary the first time the LeaderboardFolder or LeaderboardFolderList is requested.

    Retrieval of a specific Leaderboard by name key is also supported.

    Once the Leaderboard object is obtained, it can be used as a factory for creating LeaderboardView objects which 
    each contain data for specific entities. 
    The data is retrieved from the Blaze server and cached within the LeaderboardView object. 

    There are specific LeaderboardView subtypes for specific use cases. 
    A GlobalLeaderboardView can represent the top of the leaderboard, the rows centered around a specific entity, or a particular
    section of the leaderboard (useful for paging through the leaderboard).
    The FilteredLeaderboardView represents data for a particular subset of entities. 
    This is useful for displaying friend-only leaderboards, leaderboards for members of a club, or other specific sets of entitites.

    The parameters of a GlobalLeaderboardView or FilteredLeaderboardView can be modified and refreshed. 
    This allows UI to page up and down in a leaderboard without constructing a new LeaderboardView object -- 
    it may be appropriate to tie the lifecycle of a LeaderboardView object to the lifecycle of a particular screen in the UI.
    Otherwise the LeaderboardView object should be released when the data is no longer of interest.

    \note If the LeaderboardAPI is deactivated using the method API::deactivate(), all cached data is flushed, including 
    Leaderboard definitions, LeaderboardFolder and LeaderboardView objects.

    \see GameReportingComponent
    The data from which leaderboards are derived is updated through the transmission of game result reports using the GameReportingComponent.
    Please review the GameReportingComponent documentation for details.


 *******************************************************************/
class BLAZESDK_API LeaderboardAPI: public SingletonAPI
{
public:
    /*! ***************************************************************
        \name LeaderboardAPI methods
     */
    /*! ***************************************************************/
    /*! \brief Creates the API

        \param hub The hub to create the API on.
        \param allocator pointer to the optional allocator to be used for the component; will use default if not specified. 
    *******************************************************************/
    static void createAPI(BlazeHub &hub,EA::Allocator::ICoreAllocator* allocator = nullptr);

    /*! ***************************************************************/
    /*! \brief Obtain the top-most LeaderboardFolder "root" for Leaderboard
            tree iteration.

        The "root" node is a client-construct.  It contains the top-most
        LeaderboardFolderList and LeaderboardList.

        \param cb Callback.  On success use returns the "root" folder
            for the LeaderboardFolder and Leaderboard hierarchy. 
        \return The job id of this request.
    *******************************************************************/
    JobId requestTopLeaderboardFolder(GetLeaderboardFolderCb& cb);

    /*! ***************************************************************/
    /*! \brief Obtains the specified LeaderboardFolder.

        Can be used to retrieve LeaderboardFolder objects without
        iterating through the tree via the top leaderboard folder.

        \param cb Callback.  On success use returns the LeaderboardFolder. 
        \param name Name of the LeaderboardFolder to retrieve.
        \return The job id of this request.
    *******************************************************************/
    JobId requestLeaderboardFolder(GetLeaderboardFolderCb& cb, const char *name);

    /*! ***************************************************************/
    /*! \brief Returns the Leaderboard object given the ID.

        \param cb Callback.  On success use returns the Leaderboard. 
        \param name Name of the Leaderboard configuration.
        \return The job id of this request.
    *******************************************************************/
    JobId requestLeaderboard(GetLeaderboardCb& cb, const char *name);
    

    /*! ***************************************************************/
    /*! \brief Returns Leaderboard Tree starting from the given node

        \param cb Callback.  On success returns the Leaderboard tree. 
        \param name Name of the Leaderboard tree node.
        \return The job id of this request.
    *******************************************************************/
    JobId requestLeaderboardTree(GetLeaderboardTreeCb& cb, const char *name);
    
    
    /*! ***************************************************************/
    /*! \brief Removes Leaderboard Tree from memory. Next call
         to requestLeaderboardTree() will result in calling the server
        \param name Name of the Leaderboard tree (root folder) to be removed
        \return true if tree existed false otherwise
    *******************************************************************/
    bool removeLeaderboardTree(const char8_t* name);
    
    
    /*! ***************************************************************/
    /*! \brief deactivate
        Deactivates leaderboards API
        \return none
    *******************************************************************/
    void deactivate();
    

    /*! ***************************************************************/
    /*! \brief 
        Returns true if user has session defined ranks
        \param user - pointer to User object
        \return true if user has session defined ranks
    *******************************************************************/
    static bool hasRanks(const Blaze::UserManager::User *user);

    /*! ***************************************************************/
    /*! \brief 
        Returns Provides access to the session user ranks
        This call may return result only if hasRanks() returns true
        \param user - pointer to User object
        \param key - rank key
        \param value - holder for returned rank value
        \return true if operation was successful
    *******************************************************************/
    static bool getUserRank(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value);

    /*! ***************************************************************/
    /*! \brief 
        Returns true if user has session defined ranks
        \param userIndex - index of the local user
        \return true if user has session defined ranks
    *******************************************************************/
    bool hasLocalRanks(uint32_t userIndex) const;
    
    /*! ***************************************************************/
    /*! \brief 
        Returns Provides access to the session user ranks
        This call may return result only if hasLocalRanks() returns true
        \param key - rank key
        \param value - holder for returned rank value
        \param userIndex - index of the local user
        \return true if operation was successful
    *******************************************************************/
    bool getLocalUserRank(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex) const;

    /*! ****************************************************************************/
    /*! \brief Adds a listener to receive async notifications from the LeaderboardAPI.
        Once an object adds itself as a listener, LeaderboardAPI will dispatch
        events to the object via the methods in LeaderboardAPIListener.
        \param listener The listener to add.
    ********************************************************************************/
    void addListener(LeaderboardAPIListener* listener);

    /*! ****************************************************************************/
    /*! \brief Removes a listener from the LeaderboardAPI, preventing any further
            notifications from being dispatched to the listener.
        \param listener The listener to remove.
    ********************************************************************************/
    void removeListener(LeaderboardAPIListener* listener);

protected:
    /*! ****************************************************************************
        \name LeaderboardAPI protected and private methods
        
        \param[in] hub         pointer to blaze hub
        \param[in] memGroupId  mem group to be used by this class to allocate memory
     */
    LeaderboardAPI(BlazeHub& hub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~LeaderboardAPI();
    
    //    from API
    virtual void logStartupParameters() const;
    
    /*! ************************************************************************************************/
    /*! \brief Notification that a local user has lost authentication (logged out) of the blaze server
        The provided user index indicates which local user  lost authentication.  If multiple local users
        loose authentication, this will trigger once per user.

        \param[in] userIndex - the user index of the local user that authenticated.
    ***************************************************************************************************/
    virtual void onDeAuthenticated(uint32_t userIndex);

private:    

    friend class LeaderboardFolder;
    friend class Leaderboard;
    friend class LeaderboardView;
    friend class GlobalLeaderboardView;
    friend class FilteredLeaderboardView;
    friend class LeaderboardTree;

    LeaderboardAPI(const LeaderboardAPI&);
    LeaderboardAPI& operator=(const LeaderboardAPI&);

    //    lookup of  (useful for tree traversal)
    Leaderboard* findLeaderboard(const char *name);
    LeaderboardFolder* findLeaderboardFolder(int32_t id);
    LeaderboardFolder* findLeaderboardFolderByName(const char *name);

    //    factory for Leaderboard and LeaderboardFolder objects - retrieves data from
    //    server and adds the result to the appropriate table.  
    //    note: for now, calls must be chained - callback must finish before starting a new request.
    //    If id is specified then it is used.  Pass id = 0, then request uses name field for lookup.  
    JobId retrieveLeaderboard(GetLeaderboardCb& cb, int32_t id, const char *name);
    JobId retrieveLeaderboardFolder(GetLeaderboardFolderCb&, int32_t id, const char *name);

    //    call to invalidate hiearchy - on disconnection/destruction
    void releaseLeaderboardHierarchy();

    //    Leaderboard Table Management
    typedef hash_map<const char8_t*, 
        Leaderboard*,
        eastl::hash<const char8_t*>,
        eastl::str_equal_to<const char8_t*>
    > LeaderboardTable;

    LeaderboardTable mLBTable;

    void getLeaderboardGroupCb(const Stats::LeaderboardGroupResponse *resp, BlazeError err, JobId id, GetLeaderboardCb cb);

    //    LeaderboardFolder Table Management
       typedef hash_map<int32_t, 
        LeaderboardFolder*, 
        eastl::hash<int32_t>, 
        eastl::equal_to<int32_t>
    > LeaderboardFolderTable;

    typedef hash_map<const char8_t*, LeaderboardFolder*,
        eastl::hash<const char8_t*>, 
        eastl::str_equal_to<const char8_t*>
    > LeaderboardFolderNameTable;

    LeaderboardFolderTable mLBFolderTable;
    LeaderboardFolderNameTable mLBFolderNameTable;

    void getLeaderboardFolderGroupCb(const Stats::LeaderboardFolderGroup *resp, BlazeError err, JobId id, GetLeaderboardFolderCb cb);
    
    void getLeaderboardTreeAsyncCb(BlazeError err, JobId rpcJobId, const GetLeaderboardTreeCb cb, LeaderboardTree* leaderboardTree, JobId jobId);
    
    void onGetLeaderboardTreeNotification(const LeaderboardTreeNodes* treeNodes, uint32_t userIndex);
    
    typedef hash_map<const char8_t*, LeaderboardTree*, 
    eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > LeaderboardTreeMap;

    typedef ApiJob1<GetLeaderboardTreeCb, LeaderboardTree*> LeaderboardTreeJob;

    LeaderboardTreeMap mLeaderboardTreeMap;
    typedef Dispatcher<LeaderboardAPIListener> LeaderboardDispatcher;
    LeaderboardDispatcher mDispatcher;
    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
};


/*! ***************************************************************/
/*! \class LeaderboardFolder
    \brief Provides access to the child LeaderboardFolderList or 
        the child LeaderboardList for this folder.

    The top-most LeaderboardFolder is obtained from the LeaderboardAPI.
    To retrieve the child LeaderboardFolderList or child LeaderboardList
    for a folder, use the corresponding request methods in the class
    definition.

    \nosubgrouping
*******************************************************************/
class BLAZESDK_API LeaderboardFolder
{
public:
    /*! ***************************************************************/
    /*! \brief Returns the name of the LeaderboardFolder
    *******************************************************************/
    const char* getName() const {
        return mFolderData.getName();
    }

    /*! ***************************************************************/
    /*! \brief Returns a UTF-8 description string for the LeaderboardFolder.
    *******************************************************************/
    const char* getDescription() const {
        return mFolderData.getDescription();
    }

    /*! ***************************************************************/
    /*! \brief Returns metadata for the LeaderboardFolder.
    *******************************************************************/
    const char8_t* getMetadata() const {
        return mFolderData.getMetadata();
    }

    /*! ***************************************************************/
    /*! \brief Returns whether the folder has child folders.
        Use requestChildLeaderboardFolderList() to retrieve them.
    *******************************************************************/
    bool hasChildFolders() const {
        return mHasChildFolders;
    }

    /*! ***************************************************************/
    /*! \brief Returns whether the folder has child leaderboards.
        Use requestChildLeaderboardList() to retrieve them.
    *******************************************************************/
    bool hasChildLeaderboards() const {
        return mHasChildLeaderboards;
    }

    /*! ***************************************************************/
    /*! \brief Callback for retrieving the contents of a LeaderboardFolderList
            from the server.
    *******************************************************************/
    typedef Functor3<BlazeError, JobId, const LeaderboardFolderList* > GetLeaderboardFolderListCb;

    /*! ***************************************************************/
    /*! \brief Callback for retrieving the contents of a LeaderboardList
            from the server.
    *******************************************************************/
    typedef Functor3<BlazeError, JobId, const LeaderboardList* > GetLeaderboardListCb;

    /*! ***************************************************************/
    /*! \brief Returns the child LeaderboardFolderList if there are 
            LeaderboardFolder children for this folder.
        \param cb Callback returning the LeaderboardFolderList.
        \return The job id of this request.
    *******************************************************************/
    JobId requestChildLeaderboardFolderList(GetLeaderboardFolderListCb& cb);

    /*! ***************************************************************/
    /*! \brief Returns the child LeaderboardList if there are 
            Leaderboard children for this folder.
        \param cb Callback returning the LeaderboardList.
        \return The job id of this request.
    *******************************************************************/
    JobId requestChildLeaderboardList(GetLeaderboardListCb& cb);

    /*! ******* ********************************************************/
    /*! \brief Returns the parent LeaderboardFolder for this folder.
        \param cb Callback returning the LeaderboardFolder.
        \return The job id of this request.
    *******************************************************************/
    JobId requestParentLeaderboardFolder(GetLeaderboardFolderCb& cb);

protected:
    LeaderboardFolder(LeaderboardAPI *api, const Stats::LeaderboardFolderGroup *folderData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~LeaderboardFolder();

    Stats::LeaderboardFolderGroup mFolderData;

private:
    friend class LeaderboardAPI;

    bool mHasChildFolders;
    bool mHasChildLeaderboards;

    LeaderboardAPI *mLBAPI;

    LeaderboardFolder::GetLeaderboardFolderListCb mFolderListCb;
    LeaderboardFolderList mFolderList;
    
    Stats::LeaderboardFolderGroup::FolderDescriptorList::iterator mBuildFolderListIt;
    JobId fillFolderListAtCurrentIt();            // if true, requires a server get/retrieveItem call
    void getLeaderboardFolderCb(BlazeError err, JobId id, LeaderboardFolder* folder);

    LeaderboardFolder::GetLeaderboardListCb mLeaderboardListCb;
    LeaderboardList mLeaderboardList;
    JobId mJobId;

    Stats::LeaderboardFolderGroup::FolderDescriptorList::iterator mBuildLeaderboardListIt;
    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
    JobId fillLeaderboardListAtCurrentIt();        // if true, requires a server get/retrieveItem call
    void getLeaderboardCb(BlazeError err, JobId id, Leaderboard* lb);
    
    NON_COPYABLE(LeaderboardFolder)
};


//    forward class declarations for LeaderboardView.
class GlobalLeaderboardView;
class FilteredLeaderboardView;

/*! ***************************************************************/
/*! \class Leaderboard
    \brief Provides a factory for LeaderboardView objects used for
        leaderboard stat retrieval.

    A Leaderboard is obtained from a LeaderboardList returned by the 
    LeaderboardAPI or from a LeaderboardFolder instance's child list 
    of leaderboards.

    Access the stat keys used to populate Leaderboard columns.

    Create LeaderboardView instances to manage leaderboard stat data based
    on presentation preference.   The supported views are:

        GlobalLeaderboardView
        FilteredLeaderboardView

    \nosubgrouping
*******************************************************************/
class BLAZESDK_API Leaderboard
{
public:
    /*! ***************************************************************/
    /*! \brief Returns the name of the Leaderboard
    *******************************************************************/
    const char* getName() const {
        return mGroupData.getBoardName();
    }

    /*! ***************************************************************/
    /*! \brief Returns a UTF-8 description string for the Leaderboard.
    *******************************************************************/
    const char* getDescription() const {
        return mGroupData.getDesc();
    }

    /*! ***************************************************************/
    /*! \brief Returns the entity type for the Leaderboard.
    *******************************************************************/
    EA::TDF::ObjectType getEntityType() const {
        return mGroupData.getEntityType();
    }

    /*! ***************************************************************/
    /*! \brief Returns metadata for the Leaderboard.
    *******************************************************************/
    const char8_t* getMetadata() const {
        return mGroupData.getMetadata();
    }

    /*! ***************************************************************/
    /*! \brief Returns the scope name/values pair defined for the leaderboard
    *******************************************************************/
    const Stats::ScopeNameValueListMap* getScopeNameValueListMap() const {
        return &mGroupData.getKeyScopeNameValueListMap();
    }

    /*! ***************************************************************/
    /*! \brief Returns the Leaderboard column data containing stat names and
            descriptions.
    *******************************************************************/
    const Stats::StatDescSummaryList* getStatKeyColumnsList() const {
        return &mGroupData.getStatDescSummaries();
    }

    /*! ***************************************************************/
    /*! \brief Returns the the maximum leaderboard size defined in configuration file
    *******************************************************************/
    int32_t getMaximumLeaderboardSize() const {
        return mGroupData.getLeaderboardSize();
    }

    /*! ***************************************************************/
    /*! \brief Returns the stat name that the leaderboard is ranked on
    *******************************************************************/
    const char8_t* getRankingStat() const {
        return mGroupData.getStatName();
    }

    /*! ***************************************************************/
    /*! \brief Returns whether the leaderboard is ascending or not
    *******************************************************************/
    bool isAscending() const {
        return mGroupData.getAscending();
    }

    /*! \brief Callback for the getLeaderboardEntityCount. */
    typedef Functor3<BlazeError /*successCode*/, JobId, int32_t /* count */> GetEntityCountCb;

    /*! ***************************************************************/
    /*! \brief Returns number of entries in the leaderboard
        \param callback called when operation is completed
        \param scopeNameValueMap pointer to the map holding keyscope name/value pirs
        \param periodOffset period offset for peiodical leadrboards
        \returns The job id of the operation.
    *******************************************************************/
    JobId getEntityCount(GetEntityCountCb& callback, 
        Stats::ScopeNameValueMap* scopeNameValueMap = nullptr, int32_t periodOffset = 0); 

    /*! \brief Callback for the result of a createGlobalLeaderboardView action. */
    typedef Functor3<BlazeError /*successCode*/, JobId, GlobalLeaderboardView*> CreateGlobalViewCb;

    /*! ***************************************************************/
    /*! \brief Create a new instance of a GlobalLeaderboardView.

        The method attempts to find a currently created view with the passed
        in parameters and returns that instance in the result callback.  If no
        such view is found, the view is created and a server request is issued
        to retrieve stat data.

        Applications should call the returned LeaderboardView object's Release
        method when finished with the view.

        \param callback Callback issues on create request/stats-retrieval completion. 
        \param startRank Indicates what rank the first row of data will be.
        \param endRank "1" based value that marks the last rank row to
            retrieve.  Must be greater than or equal to the startRank.
        \param scopeNameValueMap is a pointer to the scope name/value map. These values 
            are used if corresponding value in the configuration file is equal to '?'
        \param periodOffset (default is 0) defines which historical period to return. 
            For period type STAT_PERIOD_DAILY 0 means today, 1 - yesterday and so on.
            For weekly and monthly periods units are 1 week and 1 month, respectively. 
            For period type STAT_PERIOD_ALL_TIME it should be 0.
        \param data (optional) Application defined data stored in the view.
        \param time (optional) is time within period to be returned, if zero periodOffset is used.
        \param userSetId (optional) is id of user set to filter leaderboard.
            If equal to EA::TDF::OBJECT_ID_INVALID no filtering is done.
        \param useRawStats (optional) If set, the view will return raw stat values, instead of strings.
        \returns The job id of the creation action.
    *******************************************************************/
    JobId createGlobalLeaderboardView(CreateGlobalViewCb& callback, int32_t startRank, int32_t endRank,
        Stats::ScopeNameValueMap *scopeNameValueMap = nullptr, int32_t periodOffset = 0,
        void *data=nullptr, int32_t time = 0, EA::TDF::ObjectId userSetId = EA::TDF::OBJECT_ID_INVALID,
        bool useRawStats = false);

    /*! ***************************************************************/
    /*! \brief Create a new instance of a GlobalLeaderboardView centered on an
            entity.

        The method attempts to find a currently created view with the passed
        in parameters and returns that instance in the result callback.  If no
        such view is found, the view is created and a server request is issued
        to retrieve stat data.

        Applications should call the returned LeaderboardView object's Release
        method when finished with the view.

        \param callback Callback issues on create request/stats-retrieval completion. 
        \param centeredEntityId Required - The entity ID to center this leaderboard row
            request around.
        \param count The number of rows to return.
        \param scopeNameValueMap is a pointer to the scope name/value map. These values 
            are used if corresponding value in the configuration file is equal to '?'
        \param periodOffset (default is 0) defines which historical period to return. 
            For period type STAT_PERIOD_DAILY 0 means today, 1 - yestoday and so on.
            For weekly and monthly periods units are 1 week and 1 month, respectively. 
            For period type STAT_PERIOD_ALL_TIME it should be 0.
        \param data (optional) Application defined data stored in the view.
        \param time (optional) is time within period to be returned, if zero periodOffset is used.
        \param userSetId (optional) is id of user set to filter leaderboard.
            If equal to EA::TDF::OBJECT_ID_INVALID no filtering is done.
        \param showAtBottomIfNotFound (optional) If centered entity is unranked, indicates whether
            to show entity at bottom or to show no rows at all.
        \param useRawStats (optional) If set, the view will return raw stat values, instead of strings.
        \returns The job id of the creation action.
     *******************************************************************/
    JobId createGlobalLeaderboardViewCentered(CreateGlobalViewCb& callback, EntityId centeredEntityId, size_t count,
        Stats::ScopeNameValueMap *scopeNameValueMap = nullptr, int32_t periodOffset = 0, 
        void *data=nullptr, int32_t time = 0, EA::TDF::ObjectId userSetId = EA::TDF::OBJECT_ID_INVALID,
        bool showAtBottomIfNotFound = false, bool useRawStats = false);

    /*! \brief Callback for the result of a createFilteredLeaderboardView action. */
    typedef Functor3<BlazeError /*successCode*/, JobId, FilteredLeaderboardView*> CreateFilteredViewCb;

    /*! ***************************************************************/
    /*! \brief Create a new instance of a FilteredLeaderboardView used to display
            ranked data for a select set of users.

        The method attempts to find a currently created view with the passed
        in parameters and returns that instance in the result callback.  If no
        such view is found, the view is created and a server request is issued
        to retrieve stat data.

        Applications should call the returned LeaderboardView object's Release
        method when finished with the view.

        \param callback Callback issues on create request/stats-retrieval completion. 
        \param entityIDs An array of entities for the request.  Can be combined with a
            userSetId.
        \param entityIDCount The number of entities specified in the passed array.
        \param scopeNameValueMap (optional) is a pointer to the scope name/value map. These values 
            are used if corresponding value in the configuration file is equal to '?'
        \param periodOffset (default is 0) defines which historical period to return. 
            For period type STAT_PERIOD_DAILY 0 means today, 1 - yestoday and so on.
            For weekly and monthly periods units are 1 week and 1 month, respectively. 
            For period type STAT_PERIOD_ALL_TIME it should be 0.
        \param data (optional) Application defined data stored in the view.
        \param time (optional) is time within period to be returned, if zero periodOffset is used.
        \param userSetId (optional) is the ID of user set.  Can be combined with an
            entityIDs list.
        \param includeStatlessEntities (optional) is a flag to include or exclude entities that do not 
            have a value for the ranking stat.  If included, the entity will display the 
            default stat value.
        \param limit (optional) Maximum count of lb results to return.
        \param useRawStats (optional) If set, the view will return raw stat values, instead of strings.
        \param enforceCutoffStatValue (optional) is a flag to include or exclude entities that do not
            meet the cutoff value for the ranking stat.  If enforced, the entity will be excluded
            and 'includeStatlessEntities' is treated as 'false' regardless.
        \returns The job id of the creation action.
    *******************************************************************/
    JobId createFilteredLeaderboardView(CreateFilteredViewCb& callback, const EntityId *entityIDs, size_t entityIDCount,
        Stats::ScopeNameValueMap *scopeNameValueMap = nullptr, int32_t periodOffset = 0,
        void *data = nullptr, int32_t time = 0, EA::TDF::ObjectId userSetId = EA::TDF::OBJECT_ID_INVALID, 
        bool includeStatlessEntities = true, uint32_t limit = UINT32_MAX, bool useRawStats = false, bool enforceCutoffStatValue = false);

protected:
    friend class LeaderboardAPI;
    friend class LeaderboardFolder;
    friend class LeaderboardView;

    Leaderboard(LeaderboardAPI *api, const Stats::LeaderboardGroupResponse *groupData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~Leaderboard();

    const Stats::LeaderboardGroupResponse* getGroupData() const {
        return &mGroupData;
    }

private:
    //    view list management
    struct GroupViewNode
    {
        LeaderboardView *view;
        CreateGlobalViewCb globalCb;
        CreateFilteredViewCb filteredCb;
    };

    struct MatchViewWithNode: eastl::binary_function<GroupViewNode, const LeaderboardView*, bool>
    {
        bool operator() (const GroupViewNode& node, const LeaderboardView* view) const {
            return (const_cast<LeaderboardView*>(node.view) == view);
        }
    };

    LeaderboardAPI *mLBAPI;
    Stats::LeaderboardGroupResponse mGroupData;

    //    internal
    void unregisterView(LeaderboardView *view);
    void releaseViews();

    //    view creation callback - initial stats refresh of the LeaderboardView
    void initialViewRefreshCb(BlazeError error, JobId id, LeaderboardView*);
    void getEntityCountResultCb(const Blaze::Stats::EntityCount* entityCount, BlazeError err, 
        JobId id, GetEntityCountCb cb);

    typedef vector<GroupViewNode> ViewList;
    ViewList mViewList;

    NON_COPYABLE(Leaderboard)
};


/*! ***************************************************************/
/*! \class LeaderboardView
    \brief Base class for all the view objects avaliable to present
        Leaderboard data.

    Obtain LeaderboardView derived objects from a Leaderboard 
    instance.

    Provides access to both what stats a leaderboard uses and
        values for these stats.

    Access the stat keys used to populate Leaderboard's UI columns.

    \nosubgrouping
*******************************************************************/
class BLAZESDK_API LeaderboardView
{
public:
/*! ****************************************************************************/
/*! \name LeaderboardView Methods
 */
    /*! \brief Result callback for requesting a set of StatValues for the LeaderboardView. */
    typedef Functor3<BlazeError /*successCode*/, JobId, LeaderboardView*> GetStatsResultCb;

    /*! ***************************************************************/
    /*! \brief Releases the memory for a view.
    *******************************************************************/
    void release(); 

    /*! ***************************************************************/
    /*! \brief Returns the Leaderboard the view is based on.
    *******************************************************************/
    const Leaderboard* getLeaderboard() const {
        return mLeaderboard;
    }

    /*! ***************************************************************/
    /*! \brief Returns the UTF-8 name of the Leaderboard view.
    *******************************************************************/
    const char* getName() const {
        return mLeaderboard->getName();
    }

    /*! ***************************************************************/
    /*! \brief Returns a UTF-8 description string for the Leaderboard view.
    *******************************************************************/
    const char* getDescription() const {
        return mLeaderboard->getDescription();
    }
    
    /*! ***************************************************************/
    /*! \brief Returns the custom data associated with this view.
    *******************************************************************/
    void* getDataPtr() const {
        return mData;
    }

    /*! ***************************************************************/
    /*! \brief Sets the custom data associated with this view.
        \param data Application-defined value stored in the view.
    *******************************************************************/
    void setDataPtr(void *data) {
        mData = data;
    }
    
    /*! ***************************************************************/
    /*! \brief Returns the period offset used for stat value retrieval.
    *******************************************************************/
    int32_t getPeriodOffset() const {
        return mPeriodOffset;
    }

    /*! ***************************************************************/
    /*! \brief Returns the cached stat values retrieved following a refresh
            or create.

        These returned container's size and contents are based on the 
        current attributes for the view (i.e. returned stat count.)
    *******************************************************************/
    const Stats::LeaderboardStatValues* getStatValues() const {
        return &mStatValues;
    }

    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on the    specified 
            rank range.

        On success the callback returns a StatsValue container that
        the application must free when done with using the object.

        \param callback Issued on request completion.
    *******************************************************************/
    virtual JobId refresh(LeaderboardView::GetStatsResultCb& callback) = 0;

    /*! ***************************************************************/
    /*! \brief Returns whether stat value retrieval returns raw stats.
    *******************************************************************/
    bool getUseRawStats() const {
        return mUseRawStats;
    }
    /*! ***************************************************************/
    /*! \brief Sets whether stat value retrieval returns raw stats for this view.
        \param useRawStats Value indicating whether to return raw stats.
    *******************************************************************/
    void setUseRawStats(bool useRawStats) {
        mUseRawStats = useRawStats;
    }

protected:
    friend class Leaderboard;

    LeaderboardView(LeaderboardAPI &api, Leaderboard &leaderBoard, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~LeaderboardView();

    void setStatValues(const Stats::LeaderboardStatValues *stats);
    void setPeriodOffset(int32_t periodOffset) {
        mPeriodOffset = periodOffset;
    }

    LeaderboardAPI *mLBAPI;
    Leaderboard *mLeaderboard;
    int32_t mTime;
    EA::TDF::ObjectId mUserSetId;
    bool mUseRawStats;
    
    void setScopeNameValueMap(Stats::ScopeNameValueMap *map);
    void copyScopeMap(Stats::ScopeNameValueMap *srcMap, Stats::ScopeNameValueMap *destMap) const;
    void copyScopeMap(Stats::ScopeNameValueMap *destMap);

    Stats::LeaderboardStatValues mStatValues;

private:
    void *mData;
    int32_t mPeriodOffset;
    Stats::ScopeNameValueMap mScopeNameValueMap;
    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor

    NON_COPYABLE(LeaderboardView)
};


/*! ***************************************************************/
/*! \class GlobalLeaderboardView
    \brief LeaderboardView used to display members of a leaderboard
        based on a reference rank.
    
    \nosubgrouping
*******************************************************************/
class BLAZESDK_API GlobalLeaderboardView: public LeaderboardView
{
public:
/*! ****************************************************************************/
/*! \name GlobalLeaderboardView Methods
 */
    /*! ***************************************************************/
    /*! \brief Returns the reference rank used for stat value retrieval.
    *******************************************************************/
    int32_t getStartRank() const {
        return mStartRank;
    }

    /*! ***************************************************************/
    /*! \brief Returns the end rank used for stat value retrieval.
    *******************************************************************/
    int32_t getEndRank() const {
        return mEndRank;
    }

    /*! ***************************************************************/
    /*! \brief Returns the entityID used to center rows of ranked data around.
        \returns Entity ID passed in via refresh or 0 if not specified.
    *******************************************************************/
    EntityId getCenteredEntityId() const {
        return mCenteredEntityId;
    }

    /*! ***************************************************************/
    /*! \brief Returns the number of rows returned by the server.
    *******************************************************************/
    size_t getRowCount() const {
        return mStatValues.getRows().size();
    }

    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on new search parameters.

        On success the callback returns a StatsValue container that
        the application can use to access returned values.  The object is available
        for the life-span of the view object.
    
        \param callback Issued on request completion.
        \param startRank "1" based value where 1 is the top rank.
        \param endRank "1" based value that marks the last rank row to
            retrieve.  Must be greater than or equal to the startRank.
        \param periodOffset (default is 0) defines which historical period to return. 
            For period type STAT_PERIOD_DAILY 0 means today, 1 - yestoday and so on.
            For weekly and monthly periods units are 1 week and 1 month, respectively. 
            For period type STAT_PERIOD_ALL_TIME it should be 0.
        \param time (optional) is time within period to be returned, if zero periodOffset is used.
        \param userSetId (optional) is id of user set to filter leaderboard.
            If equal to EA::TDF::OBJECT_ID_INVALID no filtering is done.
        \return The JobId of the refresh action.
    *******************************************************************/
    JobId refresh(LeaderboardView::GetStatsResultCb& callback, int32_t startRank, int32_t endRank, 
        int32_t periodOffset = 0, int32_t time = 0, EA::TDF::ObjectId userSetId = EA::TDF::OBJECT_ID_INVALID);

    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on new search parameters
            to center rows around an entity.

        On success the callback returns a StatsValue container that
        the application can use to access returned values.  The object is available
        for the life-span of the view object.
    
        \param callback Issued on request completion.
        \param centeredEntityId The entity ID to center this leaderboard row
            request around.
        \param count The number of rows to return.
        \param periodOffset (default is 0) defines which historical period to return. 
            For period type STAT_PERIOD_DAILY 0 means today, 1 - yestoday and so on.
            For weekly and monthly periods units are 1 week and 1 month, respectively. 
            For period type STAT_PERIOD_ALL_TIME it should be 0.
        \param time (optional) is time within period to be returned, if zero periodOffset is used.
        \param userSetId (optional) is id of user set to filter leaderboard.
            If equal to EA::TDF::OBJECT_ID_INVALID no filtering is done.
        \param showAtBottomIfNotFound (optional) If centered entity is unranked, indicates whether
            to show entity at bottom or to show no rows at all.
        \return The JobId of the refresh action.
    *******************************************************************/
    JobId refresh(LeaderboardView::GetStatsResultCb& callback, EntityId centeredEntityId, size_t count, 
        int32_t periodOffset = 0, int32_t time = 0, EA::TDF::ObjectId userSetId = EA::TDF::OBJECT_ID_INVALID,
        bool showAtBottomIfNotFound = false);

    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on the    specified 
            rank range.

        On success the callback returns a StatsValue container that
        the application must free when done with using the object.

        \param callback Issued on request completion.
        \return The JobId of the refresh action.
    *******************************************************************/
    virtual JobId refresh(LeaderboardView::GetStatsResultCb& callback);

protected:
    friend class Leaderboard;

    GlobalLeaderboardView(LeaderboardAPI &api, Leaderboard &leaderBoard, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~GlobalLeaderboardView();

private:
    void getStatsByGroupCb(const Stats::LeaderboardStatValues* stats, BlazeError err, JobId id, GetStatsResultCb cb);

    int32_t mStartRank;
    int32_t mEndRank;
    EntityId mCenteredEntityId;
    size_t mRowCount;
    bool mIsCentered;
    bool mShowAtBottomIfNotFound; //!< used by centered leaderboard view

    NON_COPYABLE(GlobalLeaderboardView)
};


/*! ***************************************************************/
/*! \class FilteredLeaderboardView
    \brief LeaderboardView used to display leaderboard data based
        on a set of entities.
    
    \nosubgrouping
*******************************************************************/
class BLAZESDK_API FilteredLeaderboardView: public LeaderboardView
{
public:    
    /*! ****************************************************************************/
    /*! \name FilteredLeaderboardView Methods
     */
    /*! ***************************************************************/
    /*! \brief Returns a pointer to the array of entities the view retrieves
            stat data for.

        \param outCount Used to store the number of entities in the 
            returned array.
    *******************************************************************/
    const EntityId* getEntityIDs(size_t *outCount) const;

    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on new search parameters.

        On success the callback returns a StatsValue container that
        the application can use to access returned values.  The object is available
        for the life-span of the view object.

        On success the callback returns a StatsValue container that
        the application must free when done with using the object.

        \param callback Issued on request completion.
        \param entityIDs An array of entities for the request.  Can be combined with a
            userSetId.
        \param entityIDCount The number of entities specified in the passed array.
        \param periodOffset (default is 0) defines which historical period to return. 
            For period type STAT_PERIOD_DAILY 0 means today, 1 - yestoday and so on.
            For weekly and monthly periods units are 1 week and 1 month, respectively. 
            For period type STAT_PERIOD_ALL_TIME it should be 0.
        \param time is time within period to be returned, if zero periodOffset is used .
        \param userSetId is the ID of user set.  Can be combined with an
            entityIDs list.
        \param includeStatlessEntities is a flag to include or exclude entities that do not 
            have a value for the ranking stat.  If included, the entity will display the 
            default stat value.
        \param limit  Maximum count of lb results to return.
        \param enforceCutoffStatValue is a flag to include or exclude entities that do not
            meet the cutoff value for the ranking stat.  If enforced, the entity will be excluded
            and 'includeStatlessEntities' is treated as 'false' regardless.
        \returns The job id of the creation action.
    *******************************************************************/
    JobId refresh(LeaderboardView::GetStatsResultCb& callback, const EntityId *entityIDs, 
        size_t entityIDCount, int32_t periodOffset, int32_t time = 0,
        EA::TDF::ObjectId userSetId = EA::TDF::OBJECT_ID_INVALID,
        bool includeStatlessEntities = true, uint32_t limit = UINT32_MAX, bool enforceCutoffStatValue = false);

    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on the    specified 
            rank range.

        On success the callback returns a StatsValue container that
        the application must free when done with using the object.

        \param callback Issued on request completion.
    *******************************************************************/
    virtual JobId refresh(LeaderboardView::GetStatsResultCb& callback);

protected:
    friend class Leaderboard;

    FilteredLeaderboardView(LeaderboardAPI &api, Leaderboard &leaderBoard, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~FilteredLeaderboardView();
        
private:
    void getStatsByGroupCb(const Stats::LeaderboardStatValues* stats, BlazeError err, JobId id, GetStatsResultCb cb);    

    typedef vector<EntityId> EntityIDList;
    EntityIDList mEntityIDList;   
    bool mIncludeStatlessEntities;
    bool mEnforceCutoffStatValue;
    uint32_t mLimit;

    NON_COPYABLE(FilteredLeaderboardView)
};

// leaderboard tree
/*! ***************************************************************/
/*! \class LeaderboardTreeNodeBase
    \brief base class for LeaderboardTreeFolder and 
    LeaderboardTreeLeaderboard classes and contains common structures
    \nosubgrouping
*******************************************************************/
class BLAZESDK_API LeaderboardTreeNodeBase
{
public:
/*! \name LeaderboardTreeNodeBase Methods
 */
    LeaderboardTreeNodeBase(const char8_t* name, const char8_t* shortDesc, 
        uint32_t firstChild, uint32_t childCount, uint32_t id, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~LeaderboardTreeNodeBase();

    /*! ***************************************************************/
    /*! \brief Returns name of the node 
    *******************************************************************/
    const char8_t* getName() const {return mName;}

    /*! ***************************************************************/
    /*! \brief Returns localized short description of the node
    *******************************************************************/
    const char8_t* getShortDesc() const {return mShortDesc;}

    /*! ***************************************************************/
    /*! \brief Returns ID of the node
    *******************************************************************/
    uint32_t getId() const {return mId;}
    
private:
    friend class LeaderboardTree;
    LeaderboardTreeNodeBase(const LeaderboardTreeNodeBase&);
    LeaderboardTreeNodeBase& operator=(const LeaderboardTreeNodeBase&);

    uint32_t getFirstChild() {return mFirstChild;}
    uint32_t getChildCount() {return mChildCount;}

    uint32_t mFirstChild;
    uint32_t mChildCount;
    uint32_t mId;
    char8_t* mName;
    char8_t* mShortDesc;
    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
};

/*! ***************************************************************/
/*! \class LeaderboardTreeLeaderboard
    \brief holds leaderboard tree leaderboard structures
    \nosubgrouping
*******************************************************************/
class BLAZESDK_API LeaderboardTreeLeaderboard: public LeaderboardTreeNodeBase
{
public:
    LeaderboardTreeLeaderboard(const char8_t* name, const char8_t* shortDesc, 
        uint32_t firstChild, uint32_t childCount, uint32_t id)
    : LeaderboardTreeNodeBase(name, shortDesc, firstChild, childCount, id)
    {}
private:
};

/*! ***************************************************************/
/*! \class LeaderboardTreeFolder
    \brief holds leaderboard tree folder structures
    \nosubgrouping
*******************************************************************/
class BLAZESDK_API LeaderboardTreeFolder: public LeaderboardTreeNodeBase
{
public:
/*! ****************************************************************************/
/*! \name LeaderboardTreeFolder Methods
 */
     typedef Blaze::vector<LeaderboardTreeFolder*> LeaderboardTreeFolderList;
     typedef Blaze::vector<LeaderboardTreeLeaderboard*> LeaderboardTreeLeaderboardList;
 
    LeaderboardTreeFolder(const char8_t* name, const char8_t* shortDesc, uint32_t firstChild, uint32_t childCount, 
                          uint32_t id, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP)
    : LeaderboardTreeNodeBase(name, shortDesc, firstChild, childCount, id),
      mLeaderboardList(memGroupId, MEM_NAME(memGroupId, "LeaderboardTreeFolder::mLeaderboardList")),
      mFolderList(memGroupId, MEM_NAME(memGroupId, "LeaderboardTreeFolder::mFolderList"))
    {}

    virtual ~LeaderboardTreeFolder();
    
    /*! ***************************************************************/
    /*! \brief Returns the list of leaderboards the folder holds 
    *******************************************************************/
    const LeaderboardTreeLeaderboardList* getLeaderboardList() const { return &mLeaderboardList;}

    /*! ***************************************************************/
    /*! \brief Returns the list of folders the folder holds 
    *******************************************************************/
    const LeaderboardTreeFolderList* getFolderList() const {return &mFolderList;}
    
    /*! ***************************************************************/
    /*! \brief Returns true if folder has leaderboards 
    *******************************************************************/
    bool hasLeaderboards() const { return (mLeaderboardList.size() != 0);}

    /*! ***************************************************************/
    /*! \brief Returns true if folder has folders 
    *******************************************************************/
    bool hasFolders() const { return (mFolderList.size() != 0);}

private:
    friend class LeaderboardTree;
    LeaderboardTreeLeaderboardList mLeaderboardList;
    LeaderboardTreeFolderList mFolderList;
    
    void addLeaderboard(LeaderboardTreeNodeBase* treeNode);
    void addFolder(LeaderboardTreeNodeBase* treeNode);
};

/*! ***************************************************************/
/*! \class LeaderboardTree
    \brief holds leaderboard tree structures
    \nosubgrouping
*******************************************************************/
class BLAZESDK_API LeaderboardTree
{
public:
    typedef hash_map<const char8_t*, LeaderboardTreeNodeBase*, 
    eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > LeaderboardTreeNodeMap;
    typedef hash_map<uint32_t, LeaderboardTreeNodeBase*, eastl::hash<uint32_t> > LeaderboardTreeIdMap;

/*! ****************************************************************************/
/*! \name LeaderboardTree Methods
 */
    LeaderboardTree(const char8_t* name, LeaderboardAPI* lbapi, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    ~LeaderboardTree();

    /*! ***************************************************************/
    /*! \brief Returns the Leaderboard tree folder or nullptr if folder does 
         not exist
    *******************************************************************/
    const LeaderboardTreeFolder* getLeaderboardTreeFolder(const char8_t* name);

    /*! ***************************************************************/
    /*! \brief Returns the Leaderboard tree top folder or nullptr if folder 
        does not exist
    *******************************************************************/
    const LeaderboardTreeFolder* getLeaderboardTreeTopFolder();
    
    /*! ***************************************************************/
    /*! \brief Returns the name of leaderboard tree which is the same 
         as a name of top folder
    *******************************************************************/
    const char8_t* getName() {return mName;}

private:
    NON_COPYABLE(LeaderboardTree)
    friend class LeaderboardAPI;
    
    char8_t* mName;

    LeaderboardTreeNodeMap mLeaderboardTreeNodeMap;
    
    LeaderboardTreeIdMap mLeaderboardTreeIdMap;
    
    void addNode(const Blaze::Stats::LeaderboardTreeNode*);
    
    void setJobId(JobId id) {mJobId = id;}
    
    bool isInProgress() { return mInProgress;}
    void setInProgress(bool progress) { mInProgress = progress;}
    
    JobId mJobId;
    LeaderboardAPI* mLbAPI; 
    bool mLoadingDone;   
    bool mInProgress;
    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor  
};

}   // Stats
}    
// Blaze
#endif

