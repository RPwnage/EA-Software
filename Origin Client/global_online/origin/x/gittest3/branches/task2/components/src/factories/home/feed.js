/**
 * @file home/feed.js
 */
(function() {
    'use strict';

    function FeedFactory($timeout, GamesDataFactory, RosterDataFactory, ComponentsLogFactory, ObjectHelperFactory) {
        var MS_TO_DAYS = (1000 * 60 * 60 * 24),
            entitlementsArray = [],
            baseGameCatalogArray = [],
            unownedDLCCatalogArray = [],
            unownedDLCOfferIds = [],
            friendsPlayingArray = [],
            lastPlayedDateArray = [],
            myEvents = new Origin.utils.Communicator(),
            gameInfoWithFriendsReady = false,
            gameInfoReady = false,
            initialPresenceReceived = false,
            refreshInterval = 60 * 1000, //temporarily set to 1 minute
            refreshSuggested = false,
            refreshTimeOutPromise;

        /**
         * resets all globals
         */
        function resetFlagsAndGlobals() {
            gameInfoWithFriendsReady = false;
            gameInfoReady = false;

            entitlementsArray = [];
            baseGameCatalogArray = [];
            unownedDLCCatalogArray = [];
            unownedDLCOfferIds = [];
            friendsPlayingArray = [];
            lastPlayedDateArray = [];
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

        /**
         * return the difference in time between the incomingVal and the currentEpoc
         * @param  {number} incomingVal a time stamp in milliseconds since epoch
         * @return {number}             the difference in days
         */
        function dayDiffFromCurrent(incomingVal) {
            var currentEpoch = (new Date()).getTime();
            return (currentEpoch - incomingVal) / MS_TO_DAYS;
        }

        /**
         * given an offerId return the last played date in millisecs since epoch time
         * @param  {string} offerId the offer id
         * @return {number}         last played time in millisecs since epoch time
         */
        function getLastPlayedDate(offerId) {
            var infoObj = findArrayElementByOfferId(lastPlayedDateArray, offerId),
                date = 0;

            if (infoObj) {
                date = infoObj.lastPlayedDate.valueOf();
            }

            return date;
        }

        /**
         * given an offerId return the grant date in millisecs since epoch time
         * @param  {string} offerId the offer id
         * @return {number}         grant datetime in millisecs since epoch time
         */
        function getGrantDate(offerId) {
            var infoObj = findArrayElementByOfferId(entitlementsArray, offerId),
                date = 0;

            if (infoObj) {
                date = infoObj.grantDate.valueOf();
            }

            return date;
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
         * comparison function used to sort array by release date
         * @param  {object} a an item of the array
         * @param  {object} b an item of the array
         * @return {number} negative/positive or 0
         */
        function compareReleasedDate(a, b) {
            return getReleaseDate(b) - getReleaseDate(a);
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
         * comparison function used to sort array by number of friends
         * @param  {object} a an item of the array
         * @param  {object} b an item of the array
         * @return {number} negative/positive or 0
         */
        function compareNumFriends(a, b) {
            //if the num friends are the same sort by recently played
            if ((a.friendsPlaying.length) && (a.friendsPlaying.length === b.friendsPlaying.length)) {
                return getLastPlayedDate(b.offerId) - getLastPlayedDate(a.offerId);
            }
            return b.friendsPlaying.length - a.friendsPlaying.length;
        }

        /**
         * generates an array of unowned games sorted by current friends playing
         * @return {string[]} a sorted array of offerIds
         */
        function feedPopularUnowned() {
            return RosterDataFactory.popularUnownedGamesList(entitlementsArray);
        }

        /**
         * generates an array of owned games sorted by current friends playing
         * @return {string[]} a sorted array of offerIds
         */
        function feedFriendsPlayingOwned() {
            return friendsPlayingArray;
        }

        /**
         * generates an array of owned games available for preload or released within a certain amount of days
         * @oaram {number} the max number a days a game has been released to show in this list
         * @return {string[]} a sorted array of offerIds
         */
        function feedRecentlyReleased(config) {

            var recentArray = [],
                i = 0,
                length = 0,
                baseGameCatalogItem = null,
                diff = 0,
                timeReleased = Number(config.dayssincerelease);

            for (i = 0, length = baseGameCatalogArray.length; i < length; i++) {
                baseGameCatalogItem = baseGameCatalogArray[i];
                diff = dayDiffFromCurrent(getReleaseDate(baseGameCatalogItem));
                if ((diff < timeReleased) && baseGameCatalogItem.downloadable) {
                    recentArray.push(baseGameCatalogItem.offerId);
                } else {
                    break;
                }
            }

            return recentArray;
        }

        /**
         * generates an array of unowned DLC sorted by date
         * @return {string[]} a sorted array of offerIds
         */
        function feedUnownedDLC() {
            return unownedDLCCatalogArray.map(function(e) {
                return e.offerId;
            });
        }

        /**
         * generats an array of games not recently played
         * @param  {number} minTime the lower bound of days not played
         * @param  {number} maxTime the upper bound of days not played
         * @return {string[]} a sorted array of offerIds
         */
        function feedNotRecentlyPlayed(config) {
            var notRecentArray = [],
                i = 0,
                length = 0,
                minTime = Number(config.notplayedlow),
                maxTime = Number(config.notplayedhigh),
                diff = 0;


            for (i = 0, length = lastPlayedDateArray.length; i < length; i++) {
                diff = dayDiffFromCurrent(getLastPlayedDate(lastPlayedDateArray[i].offerId));
                if ((diff > minTime) && (diff < maxTime)) {
                    notRecentArray.push(lastPlayedDateArray[i].offerId);
                }

                if (diff >= maxTime) {
                    break;
                }
            }

            return notRecentArray;
        }

        /**
         * generates an array of games never played
         * @param  {number} daysSince the min number of days a user has not played this game
         * @return {string[]} a sorted array of offerIds
         */
        function feedNeverPlayed(config) {
            var neverPlayedArray = [],
                i = 0,
                daysSince = Number(config.neverplayedsince),
                diff = 0;

            for (i = lastPlayedDateArray.length - 1; i >= 0; i++) {
                diff = dayDiffFromCurrent(getGrantDate(lastPlayedDateArray[i].offerId));

                if (getLastPlayedDate(lastPlayedDateArray[i].offerId) === 0) {
                    if (diff > daysSince) {
                        neverPlayedArray.push(lastPlayedDateArray[i].offerId);
                    }
                } else {
                    break;
                }

            }

            return neverPlayedArray;
        }

        /**
         * generates an array of games recently played with no friends
         * @param  {number} daysSincePlayed the number of days since the user last played
         * @return {string[]} a sorted array of offerIds
         */
        function feedRecentlyPlayedNoFriends(config) {
            var recentArray = [],
                i = 0,
                length = 0,
                daysSincePlayed = Number(config.dayssinceplayed),
                diff = 0;

            for (i = 0, length = lastPlayedDateArray.length; i < length; i++) {
                diff = dayDiffFromCurrent(getLastPlayedDate(lastPlayedDateArray[i].offerId));
                if ((diff < daysSincePlayed) && (friendsPlayingArray.indexOf(lastPlayedDateArray[i].offerId) === -1)) {
                    recentArray.push(lastPlayedDateArray[i].offerId);
                }
            }
            return recentArray;
        }

        /******************** RECOMMENDED ACTION  ********************************/

        /**
         * get the last played game for the user
         * @return {string} offerId
         */
        function feedLastPlayedGame() {
            var lastPlayed = null;

            if (lastPlayedDateArray.length > 0 && getLastPlayedDate(lastPlayedDateArray[0].offerId) !== 0) {
                //found one that was played
                lastPlayed = lastPlayedDateArray[0].offerId;
            }
            return lastPlayed;
        }

        /**
         * get the newest game the user owns
         * @return {string} offerId
         */
        function feedNewestReleased() {
            var timeReleased = 25,
                offer = null,
                diff = 0;

            if (baseGameCatalogArray.length) {
                diff = dayDiffFromCurrent(getReleaseDate(baseGameCatalogArray[0]).valueOf());
                if (diff < timeReleased) {
                    offer = baseGameCatalogArray[0].offerId;
                }
            }
            return offer;

        }

        /**
         * get the newest preload
         * @return {string} offerId
         */
        function feedNewestPreload() {
            var currentEpoch = (new Date()).getTime(),
                offer = null;

            for (var i = 0, length = baseGameCatalogArray.length; i < length; i++) {
                if ((getReleaseDate(baseGameCatalogArray[i]).valueOf() > currentEpoch) && baseGameCatalogArray[i].downloadable) {
                    offer = baseGameCatalogArray[i].offerId;
                    break;
                }
            }

            return offer;

        }

        /**
         * get the newest trial
         * @return {string} offerId
         */
        function feedNewestTrial() {
            var offer = null;

            for (var i = 0, length = baseGameCatalogArray.length; i < length; i++) {
                if (baseGameCatalogArray[i].trial) {
                    offer = baseGameCatalogArray[i].offerId;
                    break;
                }
            }
            return offer;
        }

        /**
         * give an amount of time since if a user has played a game
         * @param  {number} daysSince number of days lapsed since playing a game
         * @return {bool}             has played a game since the daysSince
         */
        function feedPlayedAGameRecently(daysSince) {
            var playedRecently = false,
                diff = 0;
            if (lastPlayedDateArray.length) {
                diff = dayDiffFromCurrent(getLastPlayedDate(lastPlayedDateArray[0].offerId));
                if (diff < daysSince) {
                    playedRecently = true;
                }
            }
            return playedRecently;
        }

        /******************** END RECOMMENDED ACTION  *****************************/

        /**
         * error triggered if the retrieve catalog is not successful
         * @param  {Error} error errorobject
         */
        function handleBaseGameCatalogRetrieveError(error) {
            ComponentsLogFactory.error(error.message);
            baseGameCatalogArray = [];
        }

        /**
         * take the base game catalog response and store it in an array
         * @param  {object} response an object of catalog entries referenced by offer id
         */
        function storeBaseGameCatalogResponse(response) {
            baseGameCatalogArray = ObjectHelperFactory.toArray(response);
            baseGameCatalogArray.sort(compareReleasedDate);
        }

        /**
         * retrieve the base game catalog info from the games data factory
         */
        function retrieveBaseGameCatalogInfo() {
            return GamesDataFactory.getCatalogInfo(GamesDataFactory.baseEntitlementOfferIdList())
                .then(storeBaseGameCatalogResponse)
                .catch(handleBaseGameCatalogRetrieveError);
        }

        /**
         * error triggered if the retrieve last played is not successful
         * @param  {Error} error errorobject
         */
        function handleLastPlayedGameRetrieveError(error) {
            ComponentsLogFactory.error('[FEEDFACTORY] lastplayed game retrieval error', error.message);
            return [];
        }

        /**
         * retrieve the last played game info
         */
        function retrieveLastPlayedGames() {
            return Origin.atom.atomGameLastPlayed()
                .catch(handleLastPlayedGameRetrieveError);
        }

        /**
         * error triggered if the retrieve unowned content is not successful
         * @param  {Error} error errorobject
         */
        function handleUnownedExtraContentError(error) {
            ComponentsLogFactory.error('[FEEDFACTORY] extra content retrieval error', error.message);
        }

        /**
         * take the extra content catalog response and store it in a array
         * @param  {object} response response an object of unowned dlc catalog entries referenced by offer id
         */
        function storeUnownedExtraContent(response) {
            for (var p in response) {
                if (response.hasOwnProperty(p)) {
                    if (unownedDLCOfferIds.indexOf(response[p].offerId) < 0) {
                        unownedDLCOfferIds.push(response[p].offerId);
                        unownedDLCCatalogArray.push(response[p]);
                    }
                }
            }
            unownedDLCCatalogArray.sort(compareReleasedDate);
        }

        /**
         * retrieve the unowned DLC catalog for all the entitlements we own
         */
        function retrieveUnownedDLC() {
            var promisesArray = [],
                i = 0,
                length = 0;

            for (i = 0, length = entitlementsArray.length; i < length; i++) {
                promisesArray.push(GamesDataFactory.retrieveExtraContentCatalogInfo(entitlementsArray[i].offerId, 'UNOWNEDONLY')
                    .then(storeUnownedExtraContent)
                    .catch(handleUnownedExtraContentError));
            }
            return Promise.all(promisesArray);
        }

        /**
         * given a response from the last played service, match that up with offer ids we own
         * @param  {object} response an array of last played games
         */
        function generateLastPlayedDateArray(response) {
            for (var k = 0, responseLength = response.length; k < responseLength; k++) {
                for (var i = 0, catLength = baseGameCatalogArray.length; i < catLength; i++) {
                    if (baseGameCatalogArray[i].masterTitleId === response[k].masterTitleId) {
                        if (response[k].timestamp) {
                            lastPlayedDateArray.push({
                                offerId: baseGameCatalogArray[i].offerId,
                                lastPlayedDate: new Date(response[k].timestamp)
                            });
                        }
                    }
                }
            }
            lastPlayedDateArray.sort(compareLastPlayedDate);
        }

        /**
         * function called after all the feed info promises are resolved
         * @param  {object[]} response an array of promise responses
         */
        function processFeedInfo(response) {
            //1 is the index for the last played promise
            generateLastPlayedDateArray(response[1]);
        }

        /**
         * error triggered if retrieving the feed info is not successful
         * @param  {Error} error errorobject
         */
        function handleFeedInfoError(error) {
            ComponentsLogFactory.error('[FEEDFACTORY] feedInfo retrieval error', error.message);
        }

        /**
         * retrieve the friends playing info from the roster data factory
         */
        function updateFriendsPlaying() {
            var friendsPlaying = [];
            for (var i = 0, length = entitlementsArray.length; i < length; i++) {
                friendsPlaying = RosterDataFactory.friendsWhoArePlaying(entitlementsArray[i].offerId);
                if (friendsPlaying.length) {
                    friendsPlayingArray.push({
                        offerId: entitlementsArray[i].offerId,
                        friendsPlaying: friendsPlaying
                    });
                }
            }
            friendsPlayingArray.sort(compareNumFriends);
            friendsPlayingArray = friendsPlayingArray.map(function(e) {
                return e.offerId;
            });
        }

        /**
         * if this is the first run through we need to wait for some time for the presence to come in before grabbing data
         * @return {promise} resolves after the presenceDelay time elapses
         */
        function waitForFriendsInfoReady() {
            return new Promise(function(resolve) {
                var presenceDelay = 3000;

                //otherwise just updateFriendsPlaying right away
                if (initialPresenceReceived) {
                    presenceDelay = 0;
                } else {
                    initialPresenceReceived = true;
                }

                $timeout(resolve, presenceDelay, false);

            });
        }

        /**
         * retrieve friends playing
         * @return {promise}
         */
        function retrieveFriendsPlaying() {
            return waitForFriendsInfoReady().then(updateFriendsPlaying);
        }

        /**
         * fires a signal that the game info is ready
         */
        function notifyGameInfoReady() {
            gameInfoReady = true;
            myEvents.fire('story:ownedGameInfoReady');
        }

        /**
         * fires a signal that the game info with friends is ready
         */
        function notifyfriendsPlayingArrayReady() {
            gameInfoWithFriendsReady = true;
            myEvents.fire('story:ownedGamesFriendsInfoReady');
        }

        /**
         * this function kicks off the feed data building process and is triggered when the base game entitlements are ready
         * @param  {object[]} entitlements array of entitlement objects
         */
        function onUpdateEntitlements(entitlements) {
            var feedInfoPromises = [];

            //make a copy of the references
            entitlementsArray = entitlements.slice();

            feedInfoPromises.push(retrieveBaseGameCatalogInfo());
            feedInfoPromises.push(retrieveLastPlayedGames());

            Promise.all(feedInfoPromises)
                .then(processFeedInfo)
                .then(retrieveUnownedDLC)
                .then(notifyGameInfoReady)
                .then(retrieveFriendsPlaying)
                .then(notifyfriendsPlayingArrayReady)
                .catch(handleFeedInfoError);
        }

        /**
         * rebuilds the feed objects with current data
         */
        function setupGameInfo() {
            resetFlagsAndGlobals();

            //if the GamesDataFactory has the game data ready, lets just use that
            //other wise we will wait till we get the entitlements
            if (GamesDataFactory.initialRefreshCompleted()) {
                onUpdateEntitlements(GamesDataFactory.baseGameEntitlements());
            } else {
                //we only want to listen for the entitlements if we dont already have base game entitlements
                //otherwise the refresh of home is on a timer and we don't need to know when entitlements are updated. We just prompt the user
                //periodically to refresh and at that point we will grab the latest base game entitlements to use
                GamesDataFactory.events.once('games:baseGameEntitlementsUpdate', onUpdateEntitlements);
                GamesDataFactory.events.once('games:baseGameEntitlementsError', onUpdateEntitlements);
            }
        }

        /**
         * sets up listener for events and runs the setup game info once -- this function should be called only once
         */
        function init() {
            Origin.events.on(Origin.events.AUTH_LOGGEDOUT, resetFlagsAndGlobals);
            Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, setupGameInfo);

            if (Origin.auth.isLoggedIn()) {
                setupGameInfo();
            }
            updateRefreshTimer();
        }

        /**
         * updates the refresh time for feed data
         * @param  {number} newInterval the time in milliseconds of when we should notify the user they should refresh
         */
        function updateRefreshTimer(newInterval) {

            //stop the running timer if there is one
            if (refreshTimeOutPromise) {
                $timeout.cancel(refreshTimeOutPromise);
            }

            //reset the refresh suggested flag
            refreshSuggested = false;

            //update the interval value
            if (newInterval) {
                refreshInterval = newInterval;
            }

            //kick off a timer
            refreshTimeOutPromise = $timeout(function() {
                //let everyone know who cares that we should refresh the feeds
                myEvents.fire('suggestRefresh');

                //set a flag so directives can check this value when they are initialized
                refreshSuggested = true;
            }, refreshInterval);
        }

        init();

        return {
            events: myEvents,
            /**
             * generates an array of owned games sorted by current friends playing
             * @return {string[]} a sorted array of offerIds
             */
            getFriendsPlayingOwned: feedFriendsPlayingOwned,
            /**
             * generates an array of owned games available for preload or released within a certain amount of days
             * @oaram {number} the max number a days a game has been released to show in this list
             * @return {string[]} a sorted array of offerIds
             */
            getRecentlyReleased: feedRecentlyReleased,
            /**
             * generats an array of games not recently played
             * @param  {number} minTime the lower bound of days not played
             * @param  {number} maxTime the upper bound of days not played
             * @return {string[]} a sorted array of offerIds
             */
            getNotRecentlyPlayed: feedNotRecentlyPlayed,
            /**
             * generates an array of games never played
             * @param  {number} daysSince the min number of days a user has not played this game
             * @return {string[]} a sorted array of offerIds
             */
            getNeverPlayed: feedNeverPlayed,
            /**
             * generates an array of unowned games sorted by current friends playing
             * @return {string[]} a sorted array of offerIds
             */
            getPopularUnownedGames: feedPopularUnowned,
            /**
             * generates an array of games recently played with no friends
             * @param  {number} daysSincePlayed the number of days since the user last played
             * @return {string[]} a sorted array of offerIds
             */
            getRecentlyPlayedNoFriends: feedRecentlyPlayedNoFriends,
            /**
             * get the newest trial
             * @return {string} offerId
             */
            getNewestTrial: feedNewestTrial,
            /**
             * get the last played game for the user
             * @return {string} offerId
             */
            getLastPlayedGame: feedLastPlayedGame,
            /**
             * get the newest game the user owns
             * @return {string} offerId
             */
            getNewestRelease: feedNewestReleased,
            /**
             * get the newest preload
             * @return {string} offerId
             */
            getNewestPreload: feedNewestPreload,
            /**
             * give an amount of time since if a user has played a game
             * @param  {number} daysSince number of days lapsed since playing a game
             * @return {bool}             has played a game since the daysSince
             */
            getPlayedAGameRecently: feedPlayedAGameRecently,
            /**
             * generates an array of unowned DLC sorted by date
             * @return {string[]} a sorted array of offerIds
             */
            getUnownedDLC: feedUnownedDLC,
            /**
             * updates the refresh time for feed data
             * @param  {number} newInterval the time in milliseconds of when we should notify the user they should refresh
             */
            updateRefreshTimer: updateRefreshTimer,
            refreshSuggested: function() {
                return refreshSuggested;
            },
            /**
             * rebuilds the feed objects with current data
             */
            reset: setupGameInfo,
            /**
             * return if the owned games info with friends info is ready
             * @return {Boolean} true if ready false if not
             */
            isOwnedGamesFriendsPlayingReady: function() {
                return gameInfoWithFriendsReady;
            },
            /**
             * return if the owned games info is ready
             * @return {Boolean} true if ready false if not
             */
            isOwnedGameInfoReady: function() {
                return gameInfoReady;
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */

    function FeedFactorySingleton($timeout, GamesDataFactory, RosterDataFactory, ComponentsLogFactory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('FeedFactory', FeedFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.FeedFactory

     * @description
     *
     * FeedFactory
     */
    angular.module('origin-components')
        .factory('FeedFactory', FeedFactorySingleton);
}());