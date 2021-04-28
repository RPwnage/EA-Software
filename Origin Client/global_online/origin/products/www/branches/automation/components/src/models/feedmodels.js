/**
 * @file home/feedmodels.js
 */

(function() {
    'use strict';
    var INITIAL_PRESENCE_DELAY = 5000;

    function FeedModelsFactory(AuthFactory, GamesDataFactory, RosterDataFactory, PinRecommendationGamesFactory,
                               ObjectHelperFactory, ComponentsLogFactory, PlatformRefiner, UtilFactory) {
        var feedDataMap = {},
            feedDataEnums = {
                ENTITLEMENTS: 'entitlements',
                BASEGAMECATALOG: 'basegamecatalog',
                UNOWNEDDLCCATALOG: 'unowneddlccatalog',
                FRIENDSPLAYING: 'friendsplaying',
                LASTPLAYED: 'lastplayed',
                FRIENDRECOMMENDATIONS: 'friendrecommendations',
                LATESTDOWNLOADED: 'latestdownloaded',
                FRIENDSPLAYINGUNOWNED: 'friendsplayingunowned',
                RECOMMENDEDGAMESTOBUY: 'recommendedgamestobuy'
            },
            currentFriendsPlayingList = {},
            timerPromise = null,
            observerSet = false,
            myEvents = new Origin.utils.Communicator();

        var isValidNucleusId = UtilFactory.isValidNucleusId;

        /**
         * reset/set the feed data map for a particular model
         * @param  {string} name             identifier for the data
         * @param  {function} retrieveFunction the function that retrieves the data from server
         * @return {void}
         */
        function resetFeedData(name, retrieveFunction) {
            feedDataMap[name] = {
                retrievePromise: null,
                data: null,
                retrieveFunction: retrieveFunction
            };
        }

        /**
         * clears out the feedData map and resets all model/presence info
         * @return {void}
         */
        function resetAllFeedData() {
            feedDataMap = {};
            currentFriendsPlayingList = {};

            resetFeedData(feedDataEnums.ENTITLEMENTS, retrieveBaseGameEntitlements);
            resetFeedData(feedDataEnums.BASEGAMECATALOG, retrieveOwnedBaseGameCatalogInfo);
            resetFeedData(feedDataEnums.UNOWNEDDLCCATALOG, retrieveUnownedDLC);
            resetFeedData(feedDataEnums.FRIENDSPLAYING, retrieveFriendsPlaying);
            resetFeedData(feedDataEnums.LASTPLAYED, retrieveLastPlayedGames);
            resetFeedData(feedDataEnums.FRIENDRECOMMENDATIONS, retrieveFriendRecommendations(1,50));
            resetFeedData(feedDataEnums.LATESTDOWNLOADED, retrieveLatestDownloaded);
            resetFeedData(feedDataEnums.FRIENDSPLAYINGUNOWNED, retrieveFriendsPlayingUnowned);
            resetFeedData(feedDataEnums.RECOMMENDEDGAMESTOBUY, PinRecommendationGamesFactory.retrieveRecommendedGamesToBuy);
        }

        /**
         * retrieves the data either from the cache or the server or returns the promise if a request is in progress
         * @param  {string} id string identifier for the data type
         * @return {object} the model data
         */
        function getData(id, alwaysFreshData, args) {

            var feedData = feedDataMap[id];

            if (feedData.retrievePromise) {
                return feedData.retrievePromise;
            }

            if (feedData.data && !alwaysFreshData) {
                return Promise.resolve(feedData.data);
            }

            feedData.retrievePromise = feedData.retrieveFunction.apply(this, args);

            return feedData.retrievePromise.then(function(response) {
                feedData.retrievePromise = null;
                feedData.data = response;
                return feedData.data;
            });
        }

        /**
         * get the base game entitlement model
         * @return {object} model
         */
        function getBaseGameEntitlements() {
            return getData(feedDataEnums.ENTITLEMENTS);
        }

        /**
         * get the base game catalog model
         * @return {object} model
         */
        function getBaseGameCatalog() {
            return getData(feedDataEnums.BASEGAMECATALOG);
        }

        /**
         * get the unowened DLC catalog
         * @return {object} model
         */
        function getUnownedDLC() {
            return getData(feedDataEnums.UNOWNEDDLCCATALOG);
        }

        /**
         * get the friends playing model
         * @return {object} model
         */
        function getFriendsPlaying() {
            return getData(feedDataEnums.FRIENDSPLAYING, true);
        }

        /**
         * get the friends playing unowned model
         * @return {object} model
         */
        function getFriendsPlayingUnowned() {
            return getData(feedDataEnums.FRIENDSPLAYINGUNOWNED, true);
        }

        /**
         * get the last played date model
         * @return {object} model
         */
        function getLastPlayedDate() {
            return getData(feedDataEnums.LASTPLAYED);
        }

        /**
         * get the friends in common model
         * @return {object} model
         */
        function getFriendRecommendations() {
            return getData(feedDataEnums.FRIENDRECOMMENDATIONS);
        }

        /**
         * get the latest downloaded model
         * @return {object} model
         */
        function getLatestDownloadedFromClient() {
            return getData(feedDataEnums.LATESTDOWNLOADED);
        }

        /**
         * get get the recommended games to buy
         * @param {object} config passed as an array used as arguments in the getData function
         * @return {object} model
         */
        function getRecommendedGamesToBuy(config) {
            return getData(feedDataEnums.RECOMMENDEDGAMESTOBUY, false, [config]);
        }

        /**
         * comparison function used to sort array by grant date high to low
         * @param  {object} a an item of the array
         * @param  {object} b an item of the array
         * @return {number} negative/positive or 0
         */
        function compareGrantDate(a, b) {
            return b.grantDate - a.grantDate;
        }


        /**
         * comparison function used to sort array by release date
         * @param  {object} a an item of the array
         * @param  {object} b an item of the array
         * @return {number} negative/positive or 0
         */
        function compareReleasedDate(a, b) {
            return getReleaseDate(b) - getReleaseDate(a);
        }

        /**
         * comparison function used to sort array by number of friends
         * @param  {object} a an item of the array
         * @param  {object} b an item of the array
         * @return {number} negative/positive or 0
         */
        function compareNumFriends(a, b) {
            //if the num friends are the same sort by recently played
            if ((a.friendsPlaying.length) && (a.friendsPlaying.length === b.friendsPlaying.length)) {
                return b.lastPlayed - a.lastPlayed;
            }
            return b.friendsPlaying.length - a.friendsPlaying.length;
        }

        /**
         * comparison function used to sort array by last played date high to low
         * @param  {object} a an item of the array
         * @param  {object} b an item of the array
         * @return {number} negative/positive or 0
         */
        function compareLastPlayedDate(a, b) {
            return b.lastPlayedDate - a.lastPlayedDate;
        }

        /**
         * mapping function to extract offerId
         * @param  {object} element the element of the array
         * @return {string} the offer id
         */
        function findArrayElementByOfferId(infoArray, offerId) {
            var info = null;

            for (var i = 0, length = infoArray.length; i < length; i++) {
                if (infoArray[i].offerId === offerId) {
                    info = infoArray[i];
                    break;
                }
            }
            return info;
        }
        /*
         * given the catalogInfo, extract out the release date since it's platform specific
         * @param {Object} catalog Info
         * @return {Date} releaseDate
         * @method getReleaseDate
         */
        function getReleaseDate(catalogInfo) {
            var releaseDate = Origin.utils.getProperty(catalogInfo, ['platforms', Origin.utils.os(), 'releaseDate']);
            if (!releaseDate) {
                releaseDate = new Date(releaseDate); //this will create a date that's really old, Wed Dec 31, 1969 16:00:00 GMT-0800
            }
            return releaseDate;
        }

        /**
         * retrieve the base game entitlements from games data factory, either get the stored games model or wait for the initial refresh to complete.
         * We make a copy of the references so we can shift the objects around
         * @return {promise} resolves with an array of entitlement objects
         */
        function retrieveBaseGameEntitlements() {
            return new Promise(function(resolve) {
                function updateFeedEntitlementsArray(entitlements) {
                    //make a copy of the references
                    var feedEntitlements = entitlements.slice();
                    feedEntitlements.sort(compareGrantDate);
                    resolve(feedEntitlements);
                }

                //if the GamesDataFactory has the game data ready, lets just use that
                //other wise we will wait till we get the entitlements
                if (GamesDataFactory.initialRefreshCompleted()) {
                    updateFeedEntitlementsArray(GamesDataFactory.baseGameEntitlements());
                } else {
                    //we only want to listen for the entitlements if we dont already have base game entitlements
                    //otherwise the refresh of home is on a timer and we don't need to know when entitlements are updated. We just prompt the user
                    //periodically to refresh and at that point we will grab the latest base game entitlements to use
                    GamesDataFactory.events.once('games:baseGameEntitlementsUpdate', updateFeedEntitlementsArray);
                    GamesDataFactory.events.once('games:baseGameEntitlementsError', updateFeedEntitlementsArray);
                }
            });
        }

        /**
         * error triggered if the retrieve catalog is not successful
         * @param  {Error} error errorobject
         */
        function handleBaseGameCatalogRetrieveError(error) {
            ComponentsLogFactory.error('[FeedModelsFactory:handleBaseGameCatalogRetrieveError]', error);
            return [];
        }

        /**
         * take the base game catalog response and convert to sortedArray
         * @param  {object} response an object of catalog entries referenced by offer id
         */
        function convertToSortedArray(response) {
            var catalogArray = ObjectHelperFactory.toArray(response);
            catalogArray.sort(compareReleasedDate);
            return catalogArray;
        }

        /**
         * retrieve the base game catalog info from the games data factory
         */
        function retrieveOwnedBaseGameCatalogInfo() {
            return getBaseGameEntitlements()
                .then(toMapOfOfferIds)
                .then(GamesDataFactory.getCatalogInfo)
                .then(convertToSortedArray)
                .catch(handleBaseGameCatalogRetrieveError);
        }

        /**
         * filter function used to parse out the mutual friends entry from the friends in common response
         */
        function parseOutMutualFriendEntries(item) {
            return _.get(item, 'reasons', false) && _.get(item, 'reasons.mf.length', 0);
        }


        /**
         * build our own model data from the response for friends in common
         * anonymous will be pushed to the end of list after sort, and they must have unique value.
         * @param  {object} data the raw data from the friends in common response
         * @return {object[]} array of friends in common data
         */
        function handlefriendRecommendationsResponse(data) {
            var filteredResponse = data.recs.filter(parseOutMutualFriendEntries);
            function uniqueAnonymous(id, index) {
                return isValidNucleusId(id) ? id : id + index;
            }
            return filteredResponse.map(function(item) {
                return {
                    userid: _.get(item, 'id'),
                    friendsInCommonStr: _.map(_.get(item, 'reasons.mf',[]).sort(),uniqueAnonymous)
                };
            });
        }

        /**
         * returns an empty array if we hit any error
         * @return {object[]} empty array
         */
        function handleFriendRecommendationsError(error) {
            ComponentsLogFactory.error('[FRIENDRECOMMENDATIONS]', error);
            return [];
        }

        /**
         * retrieve the friends in common
         */
        function retrieveFriendRecommendations(start, pageSize) {
            return function() {
                return Origin.friends.friendRecommendations(start, pageSize)
                    .then(handlefriendRecommendationsResponse)
                    .catch(handleFriendRecommendationsError);
            };
        }

        /**
         * given a response from the last played service, match that up with offer ids we own
         * @param  {object} response an array of last played games
         */
        function generateLastPlayedDateArray(response) {
            var lastPlayedDateArray = [],
                baseGameEntitlements;

            response.forEach(function(lastPlayedItem) {
                baseGameEntitlements = GamesDataFactory.getAllBaseGameEntitlementsByMasterTitleId(lastPlayedItem.masterTitleId);
                if (baseGameEntitlements) {
                    lastPlayedDateArray.push({
                        baseGameEntitlements: baseGameEntitlements,
                        lastPlayedDate: new Date(lastPlayedItem.timestamp)
                    });
                }
            });

            lastPlayedDateArray.sort(compareLastPlayedDate);

            return lastPlayedDateArray;
        }

        /**
         * give a list of sorted base game entitlements, return the first one that is released
         * @param  {object[]} baseGameEntitlements an array of sorted ENTITLEMENTS
         * @param  {object} catalogInfoMap catalog info that corresponds to the entitlement object
         * @return {string} returns the offer id
         */
        function pickBestReleasedOfferId(baseGameEntitlements, catalogInfoMap) {
            var i = 0;

            for (i = 0; i < baseGameEntitlements.length; i++) {
                var offerId = _.get(baseGameEntitlements[i], 'offerId');
                if (PlatformRefiner.isReleased(Origin.utils.os())(_.get(catalogInfoMap, [offerId]))) {
                    return offerId;
                }
            }
        }

        /**
         * given a list of sorted entitlements return the best released offer id
         * @param  {object[]} baseGameEntitlements a list of sorted base game entitlements
         * @return {Promise} returns a promise the will resolve with the offer id
         */
        function determineLastPlayedOfferId(baseGameEntitlements) {
            return GamesDataFactory.getCatalogInfo(_.map(baseGameEntitlements, 'offerId'))
                .then(_.partial(pickBestReleasedOfferId, baseGameEntitlements));

        }

        /**
         * update the lastPlayedDateArray with the offer ids and clear out the base game entitlements
         * @param  {object[]} lastPlayedDateArray the last played date array info
         * @param  {string[]} offerIds an array off offerids in the same order as lastPlayedDateArray
         * @return {object[]} the last played date array
         */
        function updateLastPlayedArrayProperties(lastPlayedDateArray, offerIds) {

            //shouldn't happen but just a fail safe
            if (!_.isArray(lastPlayedDateArray)) {
                lastPlayedDateArray = [];
            }

            //assign an offer id to the lastPlayedDate Array and clear out the baseGameEntitlements
            lastPlayedDateArray.forEach(function(item, index) {
                item.offerId = offerIds[index];
                item.baseGameEntitlements = [];
            });

            return lastPlayedDateArray;
        }

        /**
         * If an master title from the lastPlayedDateArray has multiple base game offers associated with it
         * Lets determine the best one
         *
         * @param  {object[]} lastPlayedDateArray an array of lastplayed date information, sorted with most recent first
         * @return {Promise} resolves with an updated lastPlayedDateArray with a single offer id per entry
         */
        function filterMultipleOfferIds(lastPlayedDateArray) {
            var offerIdPromises = [];

            //shouldn't happen but just a fail safe
            if (!_.isArray(lastPlayedDateArray)) {
                lastPlayedDateArray = [];
            }

            lastPlayedDateArray.forEach(function(lastPlayedItem) {
                if (lastPlayedItem.baseGameEntitlements.length === 1) {
                    offerIdPromises.push(Promise.resolve(lastPlayedItem.baseGameEntitlements[0].offerId));
                } else {
                    offerIdPromises.push(determineLastPlayedOfferId(lastPlayedItem.baseGameEntitlements));
                }
            });

            return Promise.all(offerIdPromises)
                .then(_.partial(updateLastPlayedArrayProperties, lastPlayedDateArray));
        }

        /**
         * error triggered if the retrieve last played is not successful
         * @param  {Error} error errorobject
         */
        function handleLastPlayedGameRetrieveError(error) {
            ComponentsLogFactory.error('[FEEDFACTORY] lastplayed game retrieval error', error);
            return [];
        }

        /**
         * retrieve the last played game info
         */
        function retrieveLastPlayedGames() {
            return Origin.atom.atomGameLastPlayed()
                .then(generateLastPlayedDateArray)
                .then(filterMultipleOfferIds)
                .catch(handleLastPlayedGameRetrieveError);
        }


        /**
         * retrieve the friends playing info from the roster data factory
         */
        function updateFriendsPlaying(entitlementsAndLastPlayedDateArray) {
            var entitlementsArray = entitlementsAndLastPlayedDateArray[0],
                lastPlayedDateArray = entitlementsAndLastPlayedDateArray[1],
                friendsPlaying = [],
                friendsPlayingArray = [];
            for (var i = 0, length = entitlementsArray.length; i < length; i++) {
                friendsPlaying = RosterDataFactory.friendsWhoArePlaying(entitlementsArray[i].offerId);
                if (friendsPlaying.length) {
                    var lastPlayingItem = findArrayElementByOfferId(lastPlayedDateArray, entitlementsArray[i].offerId);
                    friendsPlayingArray.push({
                        offerId: entitlementsArray[i].offerId,
                        friendsPlaying: friendsPlaying,
                        lastPlayed: lastPlayingItem ? lastPlayingItem.lastPlayedDate : 0
                    });
                }
            }
            friendsPlayingArray.sort(compareNumFriends);
            return friendsPlayingArray;
        }

        /**
         * if this is the first run through we need to wait for some time for the presence to come in before grabbing data
         * @return {promise} resolves after the presenceDelay time elapses
         */
        function waitForFriendsInfoReady() {
            if (!timerPromise) {
                timerPromise = new Promise(function(resolve) {
                    setTimeout(resolve, INITIAL_PRESENCE_DELAY);
                });
            }
            return timerPromise;
        }


        /**
         * error triggered if the retrieve unowned content is not successful
         * @param  {Error} error errorobject
         */
        function handleUnownedExtraContentError() {
            return {};
        }

        /**
         * take the extra content catalog response and massage it to the format we need
         * @param  {object} response response an object of unowned dlc catalog entries referenced by offer id
         */
        function handleUnownedExtraContentResponse(unownedDLCArrayGroupedByParent) {
            var unownedDLCOfferIds = [],
                unownedDLCCatalogArray = [];

            unownedDLCArrayGroupedByParent.forEach(function(unownedDLCGroup) {
                for (var offerId in unownedDLCGroup) {
                    if (unownedDLCGroup.hasOwnProperty(offerId)) {
                        //make sure we didn't already process this and make sure its the same platform
                        if ((unownedDLCOfferIds.indexOf(offerId) < 0) && Origin.utils.getProperty(unownedDLCGroup[offerId], ['platforms', Origin.utils.os()])) {
                            unownedDLCOfferIds.push(offerId);
                            unownedDLCCatalogArray.push(unownedDLCGroup[offerId]);
                        }
                    }
                }
            });

            return unownedDLCCatalogArray.sort(compareReleasedDate);
        }

        /**
         * takes an array of objects an creates array of offer ids
         * @param  {object[]} array of objects
         * @return {string[]} returns array of offer ids
         */
        function toMapOfOfferIds(array) {
            return array.map(function(item) {
                return item.offerId;
            });
        }

        /**
         * get the unowned dlc from the server
         * @param  {objects[]} entitlementsAndCatalog base game entitlements and base game catalog info
         * @return {promise}                        [description]
         */
        function getUnownedDLCFromServer(entitlementsAndCatalog) {
            var entitlementsArray = entitlementsAndCatalog[0],
                baseGameCatalogArray = entitlementsAndCatalog[1],
                promisesArray = [],
                i = 0,
                length = 0,
                catalogObj;

            for (i = 0, length = entitlementsArray.length; i < length; i++) {
                catalogObj = findArrayElementByOfferId(baseGameCatalogArray, entitlementsArray[i].offerId);
                //see if this parent game is matches the platform
                if (catalogObj && Origin.utils.getProperty(catalogObj, ['platforms', Origin.utils.os()])) {
                    promisesArray.push(GamesDataFactory.retrieveExtraContentCatalogInfo(entitlementsArray[i].offerId, 'UNOWNEDONLY').catch(handleUnownedExtraContentError));
                }
            }

            return Promise.all(promisesArray)
                .then(handleUnownedExtraContentResponse);

        }
        /**
         * retrieve the unowned DLC catalog for all the entitlements we own
         */
        function retrieveUnownedDLC() {
            return Promise.all([getBaseGameEntitlements(), getBaseGameCatalog()]).then(getUnownedDLCFromServer);
        }

        /**
         * resolves when the basegame entitlements and lastplayed data is ready
         * @return {promise} model
         */
        function getBaseGamesAndLastPlayed() {
            return Promise.all([getBaseGameEntitlements(), getLastPlayedDate()]);
        }

        /**
         * Determines if there is a need to show refresh home page button
         *
         * @param {Object} friendsPlayingList
         */
        function suggestHomeRefresh(friendsPlayingList) {
            _.forEach(friendsPlayingList, function(value, masterTitleId) {
                if (_.isObject(value)) {
                    initializeFriendCountIfNotDefined(masterTitleId);
                    //If it wasn't already shown and the friend count is changing from 0 to >1
                    if (!currentFriendsPlayingList[masterTitleId].present && currentFriendsPlayingList[masterTitleId].count === 0 && value.data.friends.length > 0) {
                        myEvents.fire('FeedModelsFactory:modelDirty');
                        currentFriendsPlayingList[masterTitleId].present = true;
                    }

                    //Set to new friend count
                    currentFriendsPlayingList[masterTitleId].count = value.data.friends.length;
                }
            });
        }

        /**
         * Set up observer for friends playing list from Roster Data Factory
         * @param {Object[]} friendsArray pass through object
         * @returns {Promise}
         */
        function observeFriendsPlayingList(friendsArray) {
            if (!observerSet) {
                RosterDataFactory.observeFriendsPlayingList(suggestHomeRefresh);
                observerSet = true;
            }
            return friendsArray;
        }

        function initializeFriendCountIfNotDefined(masterTitleId) {
            if (angular.isUndefined(currentFriendsPlayingList[masterTitleId])) {
                currentFriendsPlayingList[masterTitleId] = {
                    present: false,
                    count: 0
                };
            }
        }

        /**
         * Updates friends count
         *
         * @param {Object[]} friendsArray pass through object
         * @returns {Promise}
         */
        function updateFriendPlayingCount(friendsArray) {
            var friendsListObservables = RosterDataFactory.getFriendsPlaying();
            _.forEach(friendsListObservables, function(value, masterTitleId) {
                //Initialize friend playing list object for master title ID
                initializeFriendCountIfNotDefined(masterTitleId);
            });
            return friendsArray;
        }

        /**
         * retrieve friends playing
         * @return {promise}
         */
        function retrieveFriendsPlaying() {
            return waitForFriendsInfoReady()
                .then(getBaseGamesAndLastPlayed)
                .then(updateFriendsPlaying)
                .then(updateFriendPlayingCount)
                .then(observeFriendsPlayingList);
        }

        /**
         * retrieve friends playing unowned games
         * @return {promise} object with unowned friends playing info
         */
        function retrieveFriendsPlayingUnowned() {
            return waitForFriendsInfoReady()
                .then(getBaseGamesAndLastPlayed)
                .then(RosterDataFactory.populateUnownedGamesList)
                .then(updateFriendPlayingCount)
                .then(observeFriendsPlayingList);
        }

        /**
         * promise that waits for the initial client state to be received
         */
        function waitClientStateReady() {
            return new Promise(function(resolve) {
                if (GamesDataFactory.initialClientStatesReceived()) {
                    resolve();
                }

                GamesDataFactory.events.once('games:initialClientStateReceived', resolve);

            });
        }


        /**
         * figure out what is the latest downloaded game
         * @param  {object} entitlementsArray array of base game entitlements
         * @return {object} latest download object
         */
        function calculateLatestDownloaded(entitlementsAndClient) {
            var i = 0,
                length = 0,
                currentClientGame = null,
                entitlementsArray = entitlementsAndClient[0],
                latestDownload = {
                    initialInstalledTimestamp: 0
                };

            for (i = 0, length = entitlementsArray.length; i < length; i++) {
                currentClientGame = GamesDataFactory.getClientGame(entitlementsArray[i].offerId);
                //if we have a valid currentClientGame info AND
                //the latestDownload doesn't have a valid download date OR the current initialInstalledTimeStamp is newer than the one we have stored
                if (currentClientGame && (!latestDownload.initialInstalledTimestamp || currentClientGame.initialInstalledTimestamp > latestDownload.initialInstalledTimestamp)) {
                    latestDownload = currentClientGame;
                }
            }

            return latestDownload;
        }

        /**
         * get the latest downloaded information from client
         * @return {promise} resolves with the latest downloaded data
         */
        function retrieveLatestDownloaded() {
            return Promise.all([getBaseGameEntitlements(), waitClientStateReady()])
                .then(calculateLatestDownloaded);
        }

        /**
         * force refresh the cached LastPlayed info
         */
        function refreshLastPlayed() {
            resetFeedData(feedDataEnums.LASTPLAYED, retrieveLastPlayedGames);
        }

        /*
        return FeedModelsFactory.getBaseGameEntitlements()
            .then(RosterDataFactory.populateUnownedGamesList)
            .then(buildGameReturnArray);
            */

        /**
         * any time the auth changes we clear the model data
         */
        function onAuthChanged() {
            resetAllFeedData();
            timerPromise = null;
        }

        AuthFactory.events.on('myauth:change', onAuthChanged);
        resetAllFeedData();

        //listen for gamesplayed
        GamesDataFactory.events.on('games:finishedPlaying', refreshLastPlayed);

        return {
            /**
             * get the base game entitlement model
             * @return {object} model
             */
            getBaseGameEntitlements: getBaseGameEntitlements,

            /**
             * get the base game catalog model
             * @return {object} model
             */
            getBaseGameCatalog: getBaseGameCatalog,
            /**
             * get the unowned DLC catalog
             * @return {object} model
             */
            getUnownedDLC: getUnownedDLC,
            /**
             * get the friends playing model
             * @return {object} model
             */
            getFriendsPlaying: getFriendsPlaying,
            /**
             * get the friends playing unowned model
             * @return {object} model
             */
            getFriendsPlayingUnowned: getFriendsPlayingUnowned,
            /**
             * get the last played date model
             * @return {object} model
             */
            getLastPlayedDate: getLastPlayedDate,
            /**
             * get the friends in common model
             * @return {object} model
             */
            getFriendRecommendations: getFriendRecommendations,
            /**
             * get the latest downloaded model
             * @return {object} model
             */
            getLatestDownloadedFromClient: getLatestDownloadedFromClient,
            /**
             * get the recommended games to buy
             * @return {object} model
             */
            getRecommendedGamesToBuy: getRecommendedGamesToBuy,
            /**
             * resets all the model data
             * @return {void}
             */
            reset: resetAllFeedData,
            /**
             * events
             * @return {object} the events object
             */
            events: myEvents

        };


    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */

    function FeedModelsFactorySingleton(AuthFactory, GamesDataFactory, RosterDataFactory, PinRecommendationGamesFactory,
                                        ObjectHelperFactory, ComponentsLogFactory, PlatformRefiner, UtilFactory,
                                        SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('FeedModelsFactory', FeedModelsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.FeedModelsFactory

     * @description
     *
     * FeedModelsFactory
     */
    angular.module('origin-components')
        .factory('FeedModelsFactory', FeedModelsFactorySingleton);
}());
