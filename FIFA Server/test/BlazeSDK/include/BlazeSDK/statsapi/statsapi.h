/*************************************************************************************************/
/*!
    \file statsapi.h

    Stats API header.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_STATS_API
#define BLAZE_STATS_API
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/component/stats/tdf/stats.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/apijob.h"
#include "BlazeSDK/component/statscomponent.h"
#include "BlazeSDK/statsapi/statsapilistener.h"
#include "BlazeSDK/usermanager/user.h"

#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "BlazeSDK/blaze_eastl/vector.h"
#include "BlazeSDK/blaze_eastl/list.h"

#include "scope.h"

namespace Blaze
{
    namespace Stats
    {

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//    forward declarations for StatsAPI classes
class StatsGroup;
class StatsView;
class User;

/*! ***************************************************************/
/*! \brief Container for a list of StatsView objects.    
*******************************************************************/
typedef list<StatsView*> StatsViewList;

/*! ***************************************************************/
/*! \brief Callback for retrieving a single StatsGroup.
*******************************************************************/
typedef Functor3<BlazeError, JobId, StatsGroup*> GetStatsGroupCb;

/*! ***************************************************************/
/*! \brief Callback for retrieving the Stats::StatGroupList.
    
    The StatsGroupList returned is managed by the StatsAPI.  Applications
    may keep this pointer knowing that a releaseStatsGroupList() will
    invalidate the pointer.
*******************************************************************/
typedef Functor3<BlazeError, JobId, const Stats::StatGroupList*> GetStatsGroupListCb;

/*! ***************************************************************/
/*! \brief Callback for retrieving the key scope definition for stat
     on server side;

    The KeyScopes returned is managed by the StatsAPI.  Applications
    may keep this pointer knowing that a releaseStatsGroupList() will
    invalidate the pointer. vivi
*******************************************************************/
typedef Functor3<BlazeError, JobId, const Stats::KeyScopes*> GetKeyScopeCb;


/*! ***************************************************************/
/*! \class StatsAPI
    \ingroup _mod_apis

    \brief Provides retrieval and caching of statistical data from the Blaze server.

    The StatsAPI is the starting point for retrieving any statistical data from the Blaze server. 
    The StatsAPI acts as a factory for creating StatsGroup objects which represent logical groupings of
    statistics. The first time a particular StatsGroup is requested, StatsAPI will retrieve the configured
    information regarding that StatsGroup from the Blaze server. For subsequent requests for the StatsGroup
    object, the cached information will be used and a network transaction is unnecessary.
    
    The StatsAPI supports retrieval of the list of all StatsGroups defined on the server or the retrieval of
    a particular specified StatsGroup by name key.

    Once the StatsGroup object is obtained, it can be used as a factory for creating StatsView objects which 
    each contain data for specific users (or other entity types such as clubs). 
    The data is retrieved from the Blaze server and cached within the StatsView object. 
    The StatsView object should be released when the data is no longer of interest. 

    \note If the StatsAPI is deactivated using the method API::deactivate(), all cached data is flushed, including 
    StatsGroup definitions.

    \see GameReportingComponent
    The statistical data which StatsAPI retrieves is updated through the transmission of game result reports using the GameReportingComponent.
    Please review the GameReportingComponent documentation for details.
    
 *******************************************************************/
class BLAZESDK_API StatsAPI: public SingletonAPI
{
public:
    /*! ***************************************************************
        \name StatsAPI methods
     */

    /*! ***************************************************************/
    /*! \brief Creates the API

        \param hub The hub to create the API on.
        \param allocator pointer to the optional allocator to be used for the component; will use default if not specified. 
    *******************************************************************/
    static void createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator = nullptr);

    /*! ***************************************************************/
    /*! \brief Obtain all key scope definitions from the server 
        \param getMapCb Callback that returns the loaded KeyScopes
        \return If true then a map is being retrieved from the server.
    *******************************************************************/
    JobId requestKeyScopes(GetKeyScopeCb& getMapCb);

    /*! ***************************************************************/
    /*! \brief Requests the StatsGroup given the group's name.        
        \param callback    Callback returning the StatsGroup.
        \param name Name of the configured StatsGroup.
        \return If true then the item is being retrieved from the server.
    *******************************************************************/
    JobId requestStatsGroup(GetStatsGroupCb& callback, const char *name);

    /*! ***************************************************************/
    /*! \brief Obtain all StatsGroup definitions from the server if not already
            loaded.
        \param getListCb Callback that returns the loaded StatsGroupList
        \return If true then a list is being retrieved from the server.
    *******************************************************************/
    JobId requestStatsGroupList(GetStatsGroupListCb& getListCb);
    
    /*! ***************************************************************/
    /*! \brief 
        Returns true if user has session defined stats
        \param user - pointer to User object
        \return true if user has session defined stats
    *******************************************************************/
    static bool hasStats(const Blaze::UserManager::User *user);
    
    /*! ***************************************************************/
    /*! \brief 
        Returns Provides access to the session user stats
        This call may return result only if hasStats() returns true
        \param user - pointer to User object
        \param key - stat key
        \param value - holder for returned stat value
        \return true if operation was successful
    *******************************************************************/
    static bool getUserStat(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value);
    
    /*! ***************************************************************/
    /*! \brief 
        Returns true if user has session defined stats
        \param userIndex - index of the local user
        \return true if user has session defined stats
    *******************************************************************/
    bool hasLocalStats(uint32_t userIndex = 0) const;
    
    /*! ***************************************************************/
    /*! \brief 
        Returns Provides access to the session user stats
        This call may return result only if hasLocalStats() returns true
        \param key - stat key
        \param value - holder for returned stat value
        \param userIndex - index of the local user
        \return true if operation was successful
    *******************************************************************/
    bool getLocalUserStat(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex = 0) const;
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED* - use hasStats() instead
        Returns true if user has session defined stats
        \param user - pointer to User object
        \return true if user has session defined stats
    *******************************************************************/
    static bool hasSkills(const Blaze::UserManager::User *user);
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED*  - use getUserStat() instead
        Returns Provides access to the session user stats
        This call may return result only if hasSkills() returns true
        \param user - pointer to User object
        \param key - stat key
        \param value - holder for returned stat value
        \return true if operation was successful
    *******************************************************************/
    static bool getUserSkill(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value);
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED* - use hasLocalStats() instead
        Returns true if user has session defined stats
        \param userIndex - index of the local user
        \return true if user has session defined stats
    *******************************************************************/
    bool hasLocalSkills(uint32_t userIndex = 0) const;
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED* - use getLocalUserStat() instead
        Returns Provides access to the session user stats
        This call may return result only if hasLocalSkills() returns true
        \param key - stat key
        \param value - holder for returned stat value
        \param userIndex - index of the local user
        \return true if operation was successful
    *******************************************************************/
    bool getLocalUserSkill(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex = 0) const;
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED* - use hasStats() instead
        Returns true if user has session defined stats
        \param user - pointer to User object
        \return true if user has session defined stats
    *******************************************************************/
    static bool hasDnf(const Blaze::UserManager::User *user);
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED* - use getUserStat() instead
        Returns Provides access to the session user stats
        This call may return result only if hasDnf() returns true
        \param user - pointer to User object
        \param key - stat key
        \param value - holder for returned stat value
        \return true if operation was successful
    *******************************************************************/
    static bool getUserDnf(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value);
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED* - use hasLocalStats() instead
        Returns true if user has session defined stats
        \param userIndex - index of the local user
        \return true if user has session defined stats
    *******************************************************************/
    bool hasLocalDnf(uint32_t userIndex = 0) const;
    
    /*! ***************************************************************/
    /*! \brief 
        *DEPRICATED* - use getLocalUserStat() instead
        Returns Provides access to the session user stats
        This call may return result only if hasLocalDnf() returns true
        \param key - stat key
        \param value - holder for returned stat value
        \param userIndex - index of the local user
        \return true if operation was successful
    *******************************************************************/
    bool getLocalUserDnf(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex = 0) const;
    
    /*! ***************************************************************/
    /*! \brief deactivate
        Deactivates Stats API
        \return none
    *******************************************************************/
    void deactivate();

    /*! ***************************************************************/
    /*! \brief get the keyscope map which contains key scope setting 
         in stat config file

         Note: we need to do requestKeyScopes before we start other rpc
         since we need to use map to getStatKeyScopeCbvalidate the group data when sending out the request.
        
        \return KeyScopesMap for stats
    *******************************************************************/
    const Stats::KeyScopesMap* getKeyScopesMap() const;

    /*! ****************************************************************************/
    /*! \brief Adds a listener to receive async notifications from the StatsAPI.
        Once an object adds itself as a listener, StatsAPI will dispatch
        events to the object via the methods in StatsAPIListener.
        \param listener The listener to add.
    ********************************************************************************/
    void addListener(StatsAPIListener* listener);

    /*! ****************************************************************************/
    /*! \brief Removes a listener from the StatsAPI, preventing any further
            notifications from being dispatched to the listener.
        \param listener The listener to remove.
    ********************************************************************************/
    void removeListener(StatsAPIListener* listener);

protected:
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
    friend class StatsGroup;
    friend class StatsView;


    StatsAPI(BlazeHub& hub, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~StatsAPI();
 
    void releaseStatsGroupList();
    void releaseKeyScope();

    //    List of StatGroup descriptions.
    Stats::StatGroupListPtr mStatGroupList;
    void getStatGroupListCb(const Stats::StatGroupList *list, BlazeError err, JobId id, GetStatsGroupListCb cb);

    // map of key scopes 
    Stats::KeyScopesPtr mKeyScopes;
    void getStatKeyScopeCb(const Stats::KeyScopes* keyScopes, BlazeError err, JobId id, GetKeyScopeCb cb);
    uint32_t getNextViewId() {return ++mViewId;}

    //    StatsGroup Table.
    typedef hash_map<const char8_t*, 
        StatsGroup*,
        eastl::hash<const char8_t*>,
        eastl::str_equal_to<const char8_t*>
    > StatsGroupTable;

    StatsGroupTable mStatsGroupTable;
    void getStatGroupCb(const Stats::StatGroupResponse *, BlazeError err, JobId id, GetStatsGroupCb cb);

    void onGetStatsAsyncNotification(const Blaze::Stats::KeyScopedStatValues* keyScopedStatValues, uint32_t userIndex);

    uint32_t mViewId;
    
    typedef Dispatcher<StatsAPIListener> StatsDispatcher;
    StatsDispatcher mDispatcher;

    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor

    NON_COPYABLE(StatsAPI);
};


/*! ***************************************************************/
/*! \class StatsGroup
    \brief A template for presenting a set of stats and creating
        StatsView objects.

    A StatsGroup is obtained from the StatsAPI.  

    \nosubgrouping
*******************************************************************/
class BLAZESDK_API StatsGroup
{
public:
    /*! ***************************************************************/
    /*! \brief Returns the name of the StatsGroup
    *******************************************************************/
    const char* getName() const {
        return mStatGroupData->getName();
    }

    /*! ***************************************************************/
    /*! \brief Returns a UTF-8 description string for the StatsGroup.
    *******************************************************************/
    const char* getDescription() const {
        return mStatGroupData->getDesc();
    }

    /*! ***************************************************************/
    /*! \brief Returns the name of the category for the StatsGroup.
    *******************************************************************/
    const char* getCategory() const {
        return mStatGroupData->getCategoryName();
    }

    /*! ***************************************************************/
    /*! \brief Returns the entity type for the StatsGroup.
    *******************************************************************/
    EA::TDF::ObjectType getEntityType() const {
        return mStatGroupData->getEntityType();
    }

    /*! ***************************************************************/
    /*! \brief Returns metadata for the StatsGroup.
    *******************************************************************/
    const char8_t* getMetadata() const {
        return mStatGroupData->getMetadata();
    }

    /*! ***************************************************************/
    /*! \brief Returns the column data containing stat names and
            descriptions.

        This invokes the parent StatsGroup object's equivalent method.
    *******************************************************************/
    const Stats::StatGroupResponse::StatDescSummaryList* getStatKeyColumnsList() const {
        return &mStatGroupData->getStatDescs();
    }

    /*! ***************************************************************/
    /*! \brief Returns the scope name/value pair defined for the group
    *******************************************************************/
    const Stats::ScopeNameValueMap* getScopeNameValueMap() const {
        return &mStatGroupData->getKeyScopeNameValueMap();
    }

    /*! \brief Callback for the result of a createStatsView action. */
    typedef Functor3<BlazeError /*successCode*/, JobId, StatsView*> CreateViewCb;

    /*! ***************************************************************/
    /*! \brief Create a new instance of a StatsView.        

        The method attempts to find a currently created view with the passed
        in parameters and returns that instance in the result callback.  If no
        such view is found, the view is created and a server request is issued
        to retrieve stat data.

        Applications should call the returned StatsView object's Release
        method when finished with the view.

        \param callback Callback issues on create request/stats-retrieval completion. 
        \param entityIDs An array of entities for the request.  If not specified 
            callback returns USER_ERR_USER_NOT_FOUND.
        \param entityIDCount The number of entites specifed in the passed array.
        \param periodType See StatPeriodType constants for more information.
        \param periodOffset "0" for current.  Positive number indicates number of periods
            offset from current.
        \param scopeNameValueMap - scope name/value pairs passed in for
            the view if the current group has "?" in it's scope definition, otherwise should be nullptr
        \param data Application defined data stored in the view.
        \param time epoch time withing period to be retrieved if not zero, periodOffset is used otherwise
        \param periodCtr - number of historical periods to return
        \returns The job id of the create action.
    *******************************************************************/
    JobId createStatsView( CreateViewCb& callback, 
                           const EntityId *entityIDs, size_t entityIDCount, 
                           StatPeriodType periodType,
                           int32_t periodOffset,
                           const ScopeNameValueMap* scopeNameValueMap = nullptr,
                           void *data = nullptr,
                           int32_t time = 0,
                           int32_t periodCtr = 1);

private:
    friend class StatsAPI;
    friend class StatsView;

    StatsGroup(StatsAPI *api, const Stats::StatGroupResponse *StatsGroupData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~StatsGroup();

    //    internal
    virtual void unregisterView(StatsView *view);
    virtual void releaseViews();

    //    view list management
    struct GroupViewNode
    {
        StatsView *view;
    };

    struct MatchViewWithNode: eastl::binary_function<GroupViewNode, const StatsView*, bool>
    {
        bool operator() (const GroupViewNode& node, const StatsView* view) const {
            return const_cast<StatsView*>(node.view) == view;
        }
    };

    StatsAPI *mStatsAPI;
    Stats::StatGroupResponsePtr mStatGroupData;            // Cloned data

    StatsView* getViewById(uint32_t viewId);
    
    typedef vector<GroupViewNode> ViewList;
    ViewList mViewList;
    
    // memory allocation parameters
    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor  

    NON_COPYABLE(StatsGroup);
};


/*! ***************************************************************/
/*! \class StatsView
    \brief An interface to present data for a StatsGroup.

    Obtain StatsView derived objects from a StatsGroup instance.

    \nosubgrouping
*******************************************************************/
class BLAZESDK_API StatsView
{
public:
    /*! \brief Result callback for requesting a set of stats for the StatsView. */
    typedef Functor3<BlazeError /*successCode*/, JobId, StatsView*> GetStatsResultCb;

    /*! ***************************************************************/
    /*! \brief Releases the memory for a StatsView.
    *******************************************************************/
    void release(); 

    /*! ***************************************************************/
    /*! \brief Returns the StatsGroup the view is based on.
    *******************************************************************/
    const StatsGroup* getStatsGroup() const {
        return mGroup;
    }

    /*! ***************************************************************/
    /*! \brief Returns the name of the StatsGroup.
    *******************************************************************/
    const char* getName() const {
        return mGroup->getName();
    }

    /*! ***************************************************************/
    /*! \brief Returns a description string for the StatsGroup.
    *******************************************************************/
    const char* getDescription() const {
        return mGroup->getDescription();
    }

    /*! ***************************************************************/
    /*! \brief Returns the custom data associated with this view.
    *******************************************************************/
    void* getDataPtr() const {
        return mDataPtr;
    }

    /*! ***************************************************************/
    /*! \brief Sets the custom data associated with this view.
        \param data Application-defined value stored in the view.
    *******************************************************************/
    void setDataPtr(void *data) {
        mDataPtr = data;
    }

    /*! ***************************************************************/
    /*! \brief Returns a pointer to the array of entities the view retrieves
            stat data for.

        \param outCount Used to store the number of entities in the 
            returned array.
    *******************************************************************/
    const EntityId* getEntityIDs(size_t *outCount) const;


    /*! ***************************************************************/
    /*! \brief Returns the StatPeriodType used for filtering results.
    *******************************************************************/
    StatPeriodType getPeriodType() const {
        return mPeriodType;
    }

    /*! ***************************************************************/
    /*! \brief Returns the period offset used for filtering results.
    *******************************************************************/
    int32_t getPeriodOffset() const {
        return mPeriodOffset;
    }

    /*! ***************************************************************/
    /*! \brief Returns the cached stat values retrieved following a refresh
        or create.

        Note, this function should be used for group who does not has
        scope settings. 

        \return StatValues for group with no scope
    *******************************************************************/
    const Stats::StatValues* getStatValues() const; 


    /*! ***************************************************************/
    /*! \brief Returns the cached stat values retrieved following a refresh
        or create.

        Note, this function should be used for group who has one dimension
        scope setting

        \param[in] scopeValue - value of the scope specified in the group
        \return StatValues for group scope with passed in scope value
    *******************************************************************/
    const Stats::StatValues* getStatValues(ScopeValue scopeValue) const;

    /*! ***************************************************************/
    /*! \brief Returns the cached stat values retrieved following a refresh
        or create.

        Note, this function should be used for group who has 2 dimensions
        scope settings

        \param[in] scopeName1 - name of the scope specified in the group
        \param[in] scopeValue1 - value of scopeName1
        \param[in] scopeName2 - value of the scope specified in the group
        \param[in] scopeValue2 - value of scopeName2
        \return StatValues for given scopeName1/scopeValue1 and scopeName2/scopeValue2
    *******************************************************************/
    const Stats::StatValues* getStatValues(const char8_t* scopeName1, ScopeValue scopeValue1,
                                           const char8_t* scopeName2, ScopeValue scopeValue2) const; 

    /*! ***************************************************************/
    /*! \brief Returns the cached stat values retrieved following a refresh
        or create.

        Note, this function should be used for group who has more than 2 dimensions
        scope settings

        \param[in] scopeNameValueMap - a list of scope name/value pair
        \return StatValues for given scopeNameValueMap
    *******************************************************************/
    const Stats::StatValues* getStatValues(const ScopeNameValueMap* scopeNameValueMap) const;
    
    /*! ***************************************************************/
    /*! \brief Returns the cached stat values retrieved following a refresh
        or create.

        Note, this function will give all the data send from server. For a group
        has no scope, the map will only have one entry with key SCOPE_NAME_NONE.
        Otherwise, the key for the map is combination of [scope name/scope value].
        which can be generate through genStatValueMapKeyForUnit in scope.h.
        Also a key can be split into 
  
        \return KeyScopeStatsValueMap cached
    *******************************************************************/
    const Stats::KeyScopeStatsValueMap* getKeyScopeStatsValueMap() const 
    {
        return& mKeyScopeStatsValueMap;
    }
    
    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on new search parameters.

        On success the callback returns a StatsValue container that
        the application can use to access returned values.  The object is available
        for the life-span of the view object.

        \param callback Issued on request completion.
        \param entityIDs An array of entities for the request.
        \param entityIDCount The number of entities specifed in the passed array.
        \param periodType See StatPeriodType constants for more information.
        \param periodOffset "0" for current.  Positive number indicates number of periods
            offset from current.
        \param scopeNameValueMap - scope name/value pairs passed in for
            the view if the current group has "?" in it's scope definition, otherwise should be nullptr
        \param time epoch time withing period to be retrieved if not zero, periodOffset is used otherwise
        \param periodCtr - number of historical periods to return
        \returns the job id of the create action.
    *******************************************************************/
    JobId refresh( GetStatsResultCb& callback,
                   const EntityId *entityIDs, size_t entityIDCount, 
                   StatPeriodType periodType,
                   int32_t periodOffset, 
                   const ScopeNameValueMap* scopeNameValueMap = nullptr,
                   int32_t time = 0,
                   int32_t periodCtr = 1
                   );

    /*! ***************************************************************/
    /*! \brief Issues a request for StatValues based on the    specified 
            rank range.

        On success the callback returns a StatsValue container that
        the application must free when done with using the object.

        \param callback Issued on request completion.
        \param scopeNameValueMap - scope name/value pairs passed in for
            the view if the current group has "?" in it's scope definition
    *******************************************************************/
    virtual JobId refresh(GetStatsResultCb& callback, const ScopeNameValueMap* scopeNameValueMap = nullptr);

protected:
    // set stat value with no scope
    void setStatValues(const KeyScopeStatsValueMap* keyScopeStatsValueMap);
    
    void copyStatValues(const Blaze::Stats::KeyScopedStatValues* keyScopedStatValues);
    
    bool isDoneFlag() {return mDoneFlag;}
    void setDoneFlag(bool value) {mDoneFlag = value;}
    bool isCreating() {return mIsCreating;}
    void clearIsCreating() {mIsCreating = false;}
    JobId getJobId() {return mJobId;}
    
private:
    friend class StatsAPI;
    friend class StatsGroup;

    StatsView(StatsAPI *api, StatsGroup *group, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);
    virtual ~StatsView();

    StatsGroup *mGroup;
    StatsAPI *mAPI;

    class StatsViewJob : public ApiJob1<GetStatsResultCb, StatsView*>
    {
    public:
        StatsViewJob(StatsAPI* api, const FunctorBase& cb, StatsView* view) :
            ApiJob1<GetStatsResultCb, StatsView*>(api, cb, view), mView(view) {}

        virtual void cancel(BlazeError err)
        {
            if (mView->mIsCreating)
            {
                setArg1(nullptr);
                dispatchCb(err);
                mView->release();
            }
            else
                dispatchCb(err);
        }
    private:
        StatsView* mView;
    };

    void getStatsByGroupCb(const Blaze::Stats::GetStatsResponse *response, BlazeError err, JobId id, GetStatsResultCb cb);
    void getStatsByGroupAsyncCb(BlazeError err, JobId rpcJobId, JobId jobId);
    void releaseKeyScopeMap();
    bool isValidScopeValue(const KeyScopeItem* scopeItem, ScopeValue scopeValue) const;

    uint32_t getViewId() {return mViewId;}
    void setViewId() {mViewId = mAPI->getNextViewId();}
    
    void *mDataPtr;
    StatPeriodType mPeriodType;
    int32_t mPeriodOffset;
    int32_t mTime;
    int32_t mPeriodCtr;

    KeyScopeStatsValueMap mKeyScopeStatsValueMap;

    typedef vector<EntityId> EntityIDList;
    EntityIDList mEntityIDList;

    bool mIsCreating;
    bool mDoneFlag;
    
    uint32_t mViewId;
    JobId mJobId;

    MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor

    NON_COPYABLE(StatsView);
};

    }   // Stats
}    
// Blaze
#endif
