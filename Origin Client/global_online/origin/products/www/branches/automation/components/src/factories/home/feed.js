/**
 * @file home/feed.js
 */
(function() {
    'use strict';

    function FeedFactory(GamesDataFactory, ArrayHelperFactory, SubscriptionFactory, FeedModelsFactory, GameRefiner, EntitlementStateRefiner, LegacyTrialRefiner, DateHelperFactory, PlatformRefiner, GiftingFactory, ObjectHelperFactory) {
        var MS_TO_DAYS = (1000 * 60 * 60 * 24);


        /**
         * return the difference in time between the incomingVal and the currentEpoc
         * @param  {number} incomingVal a time stamp in milliseconds since epoch
         * @return {number}             the difference in days
         */
        function dayDiffFromCurrent(incomingVal) {
            var currentEpoch = Date.now();
            return (currentEpoch - incomingVal) / MS_TO_DAYS;
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
         * comparison function used to sort array by grant date high to low
         * @param  {object} a an item of the array
         * @param  {object} b an item of the array
         * @return {number} negative/positive or 0
         */
        function compareGrantDate(a, b) {
            return b.grantDate - a.grantDate;
        }

        /**
         * builds an array of objects with offerids as a property to return as feed data
         * @param  {object[]} gamesArray array of objects with offerId as a property
         * @return {object} array of objects with offer ids as the object
         */
        function buildGameReturnArray(gamesArray) {
            var gamesObjectArray = gamesArray.map(function(item) {
                return {
                    offerid: item.offerId
                };
            });

            return Promise.resolve(gamesObjectArray);
        }

        /**
         * generates an array of unowned games sorted by current friends playing
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedFriendsPlayingUnowned() {
            return FeedModelsFactory.getFriendsPlayingUnowned()
                .then(buildGameReturnArray);
        }

        /**
         * generates an array of owned games sorted by current friends playing
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedFriendsPlayingOwned() {
            return FeedModelsFactory.getFriendsPlaying().then(buildGameReturnArray);
        }

        /**
         * generates an array of recommended games to buy
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedRecommendedGamesToBuy(config) {
            return FeedModelsFactory.getRecommendedGamesToBuy(config);
        }

        /**
         * generates an array of friends in common
         * @return {Object} a sorted array of friend in common objects
         */
        function feedRecommendedMutualFriends() {
            return FeedModelsFactory.getFriendRecommendations();
        }

        /**
         * filters out games that were released for "timeReleased" days
         * @param  {number} timeReleased days released
         * @return {boolean} true if the item has been released for more than "time released" days
         */
        function releasedSinceFilter(timeReleased) {
            return function(baseGameCatalogItem) {
                var diff = dayDiffFromCurrent(getReleaseDate(baseGameCatalogItem));
                return ((diff < timeReleased) && baseGameCatalogItem.downloadable);
            };
        }

        /**
         * generates an array of owned games available for preload or released within a certain amount of days
         * @oaram {number} the max number a days a game has been released to show in this list
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedRecentlyReleased(config) {
            function buildRecentlyReleasedArray(ownedBaseGameCatalogArray) {
                var timeReleased = Number(config.dayssincerelease);
                return ownedBaseGameCatalogArray.filter(releasedSinceFilter(timeReleased));
            }

            return FeedModelsFactory.getBaseGameCatalog()
                .then(buildRecentlyReleasedArray)
                .then(buildGameReturnArray);
        }

        /**
         * generates an array of unowned DLC sorted by date
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedUnownedDLC() {
            return FeedModelsFactory.getUnownedDLC()
                .then(buildGameReturnArray);
        }

        /**
         * if a game falls between the minTime and maxTime return true
         * @param  {number} minTime the lower bound of days not played
         * @param  {number} maxTime the upper bound of days not played
         * @return {boolean} true if matches
         */
        function notRecentlyPlayedFilter(minTime, maxTime) {
            return function(lastPlayedDateItem) {
                var diff = dayDiffFromCurrent(lastPlayedDateItem.lastPlayedDate || 0);
                return ((diff > minTime) && (diff < maxTime));

            };
        }
        /**
         * generats an array of games not recently played
         * @param  {number} minTime the lower bound of days not played
         * @param  {number} maxTime the upper bound of days not played
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedNotRecentlyPlayed(config) {

            function buildNotRecentlyPlayed(lastPlayedDateArray) {
                var notRecentPlayedArray = [],
                    minTime = Number(config.notplayedlow),
                    maxTime = Number(config.notplayedhigh);


                notRecentPlayedArray = lastPlayedDateArray.filter(notRecentlyPlayedFilter(minTime, maxTime));
                ArrayHelperFactory.shuffle(notRecentPlayedArray, Math.random);
                return notRecentPlayedArray;
            }

            return FeedModelsFactory.getLastPlayedDate()
                .then(buildNotRecentlyPlayed)
                .then(buildGameReturnArray);
        }

        /**
         * filters out games that you've owned for at least "daysSince" days but have never played
         * @param  {number} daysSince the min number of days a user has not played this game
         * @return {boolean} true if match
         */
        function neverPlayedFilter(daysSince, lastPlayedDateArray) {
            return function(entitlementsItem) {
                var offerId = entitlementsItem.offerId,
                    diff = dayDiffFromCurrent(entitlementsItem.grantDate || 0);

                return ((lastPlayedDateArray.indexOf(offerId) === -1) && (diff > daysSince));
            };
        }

        /**
         * generates an array of games never played
         * @param  {number} daysSince the min number of days a user has not played this game
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedNeverPlayed(config) {
            function buildNeverPlayedArray(entitlementsAndLastPlayedResponse) {
                var neverPlayedArray = [],
                    entitlementsArray = entitlementsAndLastPlayedResponse[0],
                    lastPlayedDateArray = entitlementsAndLastPlayedResponse[1],
                    daysSince = Number(config.neverplayedsince);


                neverPlayedArray = entitlementsArray.filter(neverPlayedFilter(daysSince, lastPlayedDateArray));

                ArrayHelperFactory.shuffle(neverPlayedArray, Math.random);

                return neverPlayedArray;
            }

            return Promise.all([FeedModelsFactory.getBaseGameEntitlements(), FeedModelsFactory.getLastPlayedDate()])
                .then(buildNeverPlayedArray)
                .then(buildGameReturnArray);
        }

        /**
         * filters out games that you've played within the last "daysSincePlayed" days
         * @param  {number} daysSincePlayed the number of days since the user last played
         * @return {boolean} true if match
         */
        function recentlyPlayedFilter(daysSincePlayed) {
            return function(lastPlayedDateItem) {
                var diff = dayDiffFromCurrent(lastPlayedDateItem.lastPlayedDate || 0);
                return ((diff < daysSincePlayed));
            };
        }

        /**
         * generates an array of games recently played with no friends
         * @param  {number} daysSincePlayed the number of days since the user last played
         * @return {object[]} a sorted array of objects with an offerId property
         */
        function feedRecentlyPlayed(config) {
            function buildRecentlyPlayedNoFriendsArray(lastPlayedDateArray) {
                var daysSincePlayed = Number(config.dayssinceplayed);

                return lastPlayedDateArray.filter(recentlyPlayedFilter(daysSincePlayed));
            }

            return FeedModelsFactory.getLastPlayedDate()
                .then(buildRecentlyPlayedNoFriendsArray)
                .then(buildGameReturnArray);
        }


        /**
         * applies a randomization to the friends, taking the top X*2(where X is the number of initial tiles shown)
         * and randomizing the order
         *
         * @param  {object} stats  generated story information
         * @param  {object} config storyType configuration information
         */
        function applyFriendsRandomization(storyConfig) {
            var topMatches = [],
                numToApplyRandom = storyConfig.storiesAdded * 2,
                seed = (new Date()).setHours(0, 0, 0, 0) / 100000; //use the day as a starting point for hte seed

            //a simple pseudo random number generator, we don't need it to be super random
            //just somthing basic
            //
            //the constants generate a relatively distributed list of numbers from a given seed
            function seededRandom() {
                seed = (seed * 9301 + 49297) % 233280;
                return (seed / 233280);
            }

            //split off the top X friends
            topMatches = storyConfig.feedData.splice(0, numToApplyRandom);

            //randomize it
            ArrayHelperFactory.shuffle(topMatches, seededRandom);

            storyConfig.feedData = topMatches.concat(storyConfig.feedData);
        }

        /******************** RECOMMENDED ACTION  ********************************/

        function feedNextUnopenedGift() {
            var gift = GiftingFactory.getNextUnopenedGift(),
                offerId = _.get(gift, 'offerId', null);

            return Promise.resolve(offerId);
        }

        /**
         * returns offerId back if offer is downloadable, null if its not
         * @param  {string} offerId the offer id to check
         * @return {promise}  resolves with offer id if downloadable, null other wise
         */
        function downloadableFilter(offerId) {
            if(offerId) {
                return GamesDataFactory.getCatalogInfo([offerId])
                    .then(function(catalogObjects) {
                        var downloadStartDate = _.get(catalogObjects[offerId], ['platforms', Origin.utils.os(), 'downloadStartDate'], null);
                        if(downloadStartDate) {
                            return DateHelperFactory.isInThePast(downloadStartDate) ? offerId : null;
                        }

                        return null;
                    })
                    .catch(function() {
                        return null;
                    });

            } else {
                return Promise.resolve(null);
            }
        }
        /**
         * checks if offer is past its 'use end date'
         * @param  {string} offerId the offer id to check
         * @return {bool} returns true if past use end date, false otherwise
         */
        function checkPastUseEndDate(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId])
                    .then(function(catalogObj) {
                        return PlatformRefiner.isExpired(Origin.utils.os())(catalogObj[offerId]);
                    });
        }
        /**
         * checks if offer is not expired
         * @param  {string} offerId the offer id to check
         * @return {string} returns the offer id if not expired, null otherwise
         */
        function checkNotExpired(offerId) {
            if (offerId) {
                var pastTerminationDate = false,
                    entitlement = GamesDataFactory.getEntitlement(offerId);

                //check if any entitlement is past the termination date
                if (entitlement) {
                    pastTerminationDate = EntitlementStateRefiner.isExpired(entitlement);
                }

                //also check for trials (which include ones that don't have termination like Unravel)
                return Promise.all([checkPastUseEndDate(offerId), GamesDataFactory.isTrialExpired(offerId)])
                    .then(_.spread(function(pastUseEndDate, trialExpired) {
                        return (pastUseEndDate || trialExpired || pastTerminationDate) ? null : offerId;
                    }));
            } else {
                return null;
            }
        }

        /**
         * returns the offerid if its installed and released or not installed
         * @param  {string} offerId    the offer id
         * @param  {object} catalogObj the catalog info
         * @return {string} the offer id or NULL
         */

        function returnOfferIfNotInstalledOrInstalledAndReleased(offerId, isReleased, isInstalled) {
            if (offerId && ((isReleased && isInstalled) || !isInstalled)) {
                return offerId;
            }

            return null;
        }

        /**
         * checks to make sure its installed and playable
         * @param  {string} offerId the offer id to check
         * @return {string} returns the offer id if installed and playable, or not installed at all -- returns null otherwse
         */

        function checkIfInstalledThenReleased(offerId) {
            if (!offerId) {
                return Promise.resolve(null);
            }

            return Promise.all([
                    offerId,
                    GamesDataFactory.getCatalogInfo([offerId])
                        .then(ObjectHelperFactory.getProperty(offerId))
                        .then(PlatformRefiner.isReleased(Origin.utils.os())),
                    GamesDataFactory.getClientGamePromise(offerId)
                        .then(ObjectHelperFactory.getProperty('installed'))
                ])
                .then(_.spread(returnOfferIfNotInstalledOrInstalledAndReleased));
        }

        function entitlementInstalled(entitlement) { 
            return GamesDataFactory.isInstalled(entitlement.offerId);
        }

        function feedJustAcquired() {

            function buildJustAcquiredOffer(entitlementsAndLastPlayedResponse) {
                var entitlementsArray = entitlementsAndLastPlayedResponse[0],
                    lastPlayedDateArray = entitlementsAndLastPlayedResponse[1],
                    offerToReturn = null;

                // filter the array down so we remove all installed titles
                entitlementsArray = _.dropWhile(entitlementsArray, entitlementInstalled);

                var isGrantDateLaterThanLastPlayed = entitlementsArray.length && lastPlayedDateArray.length && (entitlementsArray[0].grantDate > lastPlayedDateArray[0].lastPlayedDate),
                    isNeverPlayedGameButOwnsEntitlement = entitlementsArray.length && !lastPlayedDateArray.length;

                //show if the title is installable on current platform but that check is done indirectly by whether there's a downloadStartDate or not which is done
                //via downloadableFilter below
                //if the lastest grant date is newer than the last played set the offerToReturn with the newest grant date offerId (otherwise it iwll be null)
                if (isGrantDateLaterThanLastPlayed || isNeverPlayedGameButOwnsEntitlement) {
                    offerToReturn = entitlementsArray[0].offerId;
                }

                return downloadableFilter(offerToReturn)
                        .then(checkNotExpired);
            }

            return Promise.all([FeedModelsFactory.getBaseGameEntitlements(), FeedModelsFactory.getLastPlayedDate()]).then(buildJustAcquiredOffer);
        }

        /**
         * get the latest downloaded game for the user if newer than last played
         * @return {string} offerId
         */

        function feedDownloadedGameNewerLastPlayed() {
            function buildDownloadedGameNewerOffer(latestDownloadedAndLastPlayedResponses) {
                var offerToReturn = null,
                    latestDownload = latestDownloadedAndLastPlayedResponses[0],
                    lastPlayedDateArray = latestDownloadedAndLastPlayedResponses[1];

                //if the latestDownload is newer than the last played set the offerToReturn with the latestDownload offerId (otherwise it iwll be null)
                if (lastPlayedDateArray.length && latestDownload.initialInstalledTimestamp && (Date.parse(latestDownload.initialInstalledTimestamp)) > lastPlayedDateArray[0].lastPlayedDate) {
                    offerToReturn = latestDownload.productId;
                }

                return offerToReturn;
            }

            return Promise.all([FeedModelsFactory.getLatestDownloadedFromClient(), FeedModelsFactory.getLastPlayedDate()])
                .then(buildDownloadedGameNewerOffer)
                .then(checkIfInstalledThenReleased);
        }

        /**
         * get the last played game for the user
         * @return {string} offerId
         */
        function feedLastPlayedGame(includeExpired) {

            var lastPlayedDateArray = [];

            /**
             * saves off the last played game results
             * @param  {object[]} results array of last played game objects
             */
            function storeLastPlayedResults(results) {
                lastPlayedDateArray = results;
            }

            /**
             * gets the last played offer at "position" index in the array
             * @param  {number} position the position in the array
             * @return {string} returns the offer id if not expired, null otherwise
             */
            function buildLastPlayedOffer(position) {
                var lastPlayed = null;

                if (lastPlayedDateArray.length > position && (lastPlayedDateArray[position].lastPlayedDate !== 0)) {
                    //found one that was played
                    lastPlayed = lastPlayedDateArray[position].offerId;
                }
                return includeExpired ? lastPlayed : checkNotExpired(lastPlayed);
            }

            /**
             * gets "second" last game played as fallback
             * @param  {string} validOfferId the offer id of the first last played game, null if the game was expired
             * @return {string} returns the offer id if not expired, null otherwise
             */
            function buildNextToLastPlayedIfNeeded(validOfferId) {
                if(validOfferId) {
                    return validOfferId;
                } else {
                    return buildLastPlayedOffer(1);
                }
            }

            return FeedModelsFactory.getLastPlayedDate()
                .then(storeLastPlayedResults)
                .then(_.partial(buildLastPlayedOffer, 0))
                .then(buildNextToLastPlayedIfNeeded);

        }

        /**
         * get the newest game the user owns
         * @return {string} offerId
         */
        function feedNewestReleased() {

            function buildNewestRelease(lastPlayedDateArray, ownedBaseGameCatalogArray) {
                var timeReleased = 25,
                    offer = null,
                    diff = 0,
                    releaseDate;

                if (ownedBaseGameCatalogArray.length) {
                    releaseDate = getReleaseDate(ownedBaseGameCatalogArray[0]);

                    diff = dayDiffFromCurrent(releaseDate);
                    if ((diff < timeReleased) && (diff > 0) &&
                        //if the last played game is newer than the release date, do not show the new release tile
                        (!lastPlayedDateArray.length || (lastPlayedDateArray[0].lastPlayedDate < releaseDate))) {
                        offer = ownedBaseGameCatalogArray[0].offerId;
                    }
                }
                return downloadableFilter(offer)
                    .then(checkNotExpired)
                    .then(checkIfInstalledThenReleased);
            }
            return Promise.all([FeedModelsFactory.getLastPlayedDate(),FeedModelsFactory.getBaseGameCatalog()])
                .then(_.spread(buildNewestRelease));
        }

        /**
         * get the newest preload
         * @return {string} offerId
         */
        function feedNewestPreload() {
            function buildNewestPreload(ownedBaseGameCatalogArray) {
                var currentEpoch = Date.now(),
                    offer = null;

                for (var i = 0, length = ownedBaseGameCatalogArray.length; i < length; i++) {
                    if ((getReleaseDate(ownedBaseGameCatalogArray[i]) > currentEpoch) && ownedBaseGameCatalogArray[i].downloadable) {
                        offer = ownedBaseGameCatalogArray[i].offerId;
                        break;
                    }
                }

                return downloadableFilter(offer)
                    .then(checkNotExpired)
                    .then(checkIfInstalledThenReleased);

            }

            return FeedModelsFactory.getBaseGameCatalog().then(buildNewestPreload);
        }

        /**
         * returns the offerId of non-legacy trial if it is the lastplayed AND it is in progress (or expired) or null
         * @param  {string} offerId of non-legacy trial that was last played
         * @return {string} offerId or null
         */
        function nonLegacyTrial(trialsObj, expired) {
            if(trialsObj.nonLegacyTrial) {
                return GamesDataFactory.isTrialInProgress(trialsObj.nonLegacyTrial)
                    .then(function(inProgress) {
                        var setNonLegacyTrial = expired ? !inProgress : inProgress;
                        trialsObj.nonLegacyTrial = setNonLegacyTrial ? trialsObj.nonLegacyTrial : null;
                        return trialsObj;
                    }).catch(function() {
                        trialsObj.nonLegacyTrial = null;
                        return trialsObj;
                    });
             } else {
                return trialsObj;
            }

        }

        /**
         * if nonLegacyTrial exists, then just pass that thru, otherwise, returns the offerId of legacy trial that is most recently played
         * @param  {string[]} legacyTrials list of legacy trial offerIds
         * @return {string} offerId or null
         */
        function fallbackOnLegacyTrial(trialsObj) {
            if (trialsObj.nonLegacyTrial) {
                return trialsObj.nonLegacyTrial;  //just pass thru
            } else if (trialsObj.legacyTrials.length === 0) {
                //no legacy trials to fall back on
                return null;
            } else {
                //find most recently played legacy trial
                return FeedModelsFactory.getLastPlayedDate()
                    .then(function(lastPlayedDateArray) {
                        //since lastPlayedArray is sorted, find the first one that matches legacytrial and use that
                        for (var i = 0; i < lastPlayedDateArray.length; i++) {
                            if (trialsObj.legacyTrials.indexOf(lastPlayedDateArray[i].offerId) >= 0) {
                                return lastPlayedDateArray[i].offerId;
                            }
                        }
                        //couldn't find in the lastplayeddate, just return the first one
                        return trialsObj.legacyTrials [0];

                    })
                    .catch(function () {
                        return trialsObj.legacyTrials[0];
                    }); //since we couldn't get LastPlayeDate(), just return the first one in the list
            }
        }



        /**
         * filter if the trial is activated
         * @param  {string} offerId the offer id of the trial
         * @return {Promise} returns a promise that resolves with the offerid or null depending if something is activated
         */
        function filterForActivatedTrials(offerId) {
            if(offerId) {
                return GamesDataFactory.isTrialAwaitingActivation(offerId)
                    .then(function(notActivated) {
                        return notActivated ? null: offerId;
                    });
            } else {
                return Promise.resolve(null);
            }
        }

        /**
         * sets up the trial info object
         * @param {string[]} legacyTrials     an array of legacy trials
         * @param {string} nonlegacyOfferId the lastplayed non legacyOfferId
         * @return {object} an object of legacy offerid and legacy trials
         */
        function setTrialInfoReturnObject(legacyTrials, nonlegacyOfferId) {
            return {
                nonLegacyTrial: nonlegacyOfferId,
                legacyTrials: legacyTrials
            };
        }

        /**
         * returns the offerId of non-legacy trial if it is the lastplayed OR most recently played legacy trial or null
         * @param {objects[]} ownedBaseGameCatalogArray the sorted owned base game catalog array
         * @param {string} offerId lastplayed game offerId
         * @return {object} trialsobject - nonLegacyTrialOfferId,if exists, otherwise null; legacyTrial = array of legacy trials in progress
         */
        function findLastPlayedNonLegacyAndLegacyTrials(ownedBaseGameCatalogArray, lastPlayed, expired) {
            var catalogInfo = null,
                nonLegacyTrial = null,
                legacyTrials = [],
                inProgress,
                addToLegacyTrials;

            for (var i = 0, length = ownedBaseGameCatalogArray.length; i < length; i++) {
                catalogInfo = ownedBaseGameCatalogArray[i];
                //is it a trial of some kind?
                if (GameRefiner.isEarlyAccess(catalogInfo) || GameRefiner.isTrial(catalogInfo)) {
                    //now check for non legacy
                    if (!LegacyTrialRefiner.isLegacyTrial(catalogInfo)) {
                        //purposely don't want && because we don't want early-access to fall into the else-if
                        if (catalogInfo.offerId === lastPlayed) {
                            nonLegacyTrial = catalogInfo.offerId;
                        }
                    } else {
                        inProgress = LegacyTrialRefiner.isTrialInProgress(catalogInfo, GamesDataFactory.getEntitlement(catalogInfo.offerId));
                        addToLegacyTrials = expired ? !inProgress : inProgress;
                        if (addToLegacyTrials) {
                            legacyTrials.push(catalogInfo.offerId);
                        }
                    }
                }
            }

            //check that the trial has been activated and return the offer id if so
            return filterForActivatedTrials(nonLegacyTrial)
                .then(_.partial(setTrialInfoReturnObject, legacyTrials));
        }

        /**
         * get the newest trial that is in progress (or expired) that is
         * a) legacy trial
         * b) non-legacy and lastplayed
         * @return {string} offerId
         */
        function feedNewestTrial(checkExpired) {
            return Promise.all([FeedModelsFactory.getBaseGameCatalog(), feedLastPlayedGame(checkExpired)])
                .then(_.spread(_.partial(findLastPlayedNonLegacyAndLegacyTrials, _, _, checkExpired)))
                .then(_.partial(nonLegacyTrial, _, checkExpired))
                .then(fallbackOnLegacyTrial)
                .catch(function() {
                    return null;
                });
        }

        /**
         * give an amount of time since if a user has played a game
         * @param  {number} daysSince number of days lapsed since playing a game
         * @return {bool}             has played a game since the daysSince
         */
        function feedPlayedAGameRecently(daysSince) {
            function buildPlayedAGameRecently(lastPlayedDateArray) {
                var playedRecently = false,
                    diff = 0;
                if (lastPlayedDateArray.length) {
                    diff = dayDiffFromCurrent(lastPlayedDateArray[0].lastPlayedDate || 0);
                    if (diff < daysSince) {
                        playedRecently = true;
                    }
                }
                return playedRecently;
            }
            return FeedModelsFactory.getLastPlayedDate()
                .then(buildPlayedAGameRecently);
        }


        function installedGameFilter(entitlement) {
            return GamesDataFactory.isInstalled(entitlement.offerId);
        }

        function feedGamesInstalled() {
            function buildGamesInstalledResponse(entitlementsArray) {
                return entitlementsArray.filter(installedGameFilter);
            }

            return FeedModelsFactory.getBaseGameEntitlements().then(buildGamesInstalledResponse);
        }

        function subsEntitlementFilter(entitlement) {
            return GamesDataFactory.isSubscriptionEntitlement(entitlement.offerId);

        }

        function feedMostRecentlyGrantedSubsEntitlement() {
            function buildMostRecentGrantedSubsResponse(entitlementsArray) {
                var mostRecentEntOfferId = null,
                    subsEnts = entitlementsArray.filter(subsEntitlementFilter);

                if (subsEnts.length > 0) {
                    subsEnts.sort(compareGrantDate);
                    mostRecentEntOfferId = subsEnts[0].offerId;
                }
                return mostRecentEntOfferId;

            }

            return FeedModelsFactory.getBaseGameEntitlements().then(buildMostRecentGrantedSubsResponse);
        }

        /*
         * returns the most recently entitled subs offer that is not installed
         */
        function feedMostRecentSubsEntitlementNotInstalled() {

            function buildMostRecentSubsEntitlementNotInstalled(gamesInstalled) {
                var offerId = null;

                //active subscriber and doesn't have any games installed
                if (Origin.client.isEmbeddedBrowser() && SubscriptionFactory.userHasSubscription() && SubscriptionFactory.isActive() && gamesInstalled.length === 0) {
                    offerId = feedMostRecentlyGrantedSubsEntitlement();
                }
                return offerId;
            }

            return feedGamesInstalled().then(buildMostRecentSubsEntitlementNotInstalled);
        }

        /******************** END RECOMMENDED ACTION  *****************************/

        return {
            /**
             * generates an array of recommended games to buy
             * @return {object[]} a sorted array of objects with an offerId property
             */            
            getRecommendedGamesToBuy: feedRecommendedGamesToBuy,
            /**
             * generates an array of owned games sorted by current friends playing
             * @return {object[]} a sorted array of objects with an offerId property
             */
            getFriendsPlayingOwned: feedFriendsPlayingOwned,
            /**
             * generates an array of owned games available for preload or released within a certain amount of days
             * @oaram {number} the max number a days a game has been released to show in this list
             * @return {object[]} a sorted array of objects with an offerId property
             */
            getRecentlyReleased: feedRecentlyReleased,
            /**
             * generats an array of games not recently played
             * @param  {number} minTime the lower bound of days not played
             * @param  {number} maxTime the upper bound of days not played
             * @return {object[]} a sorted array of objects with an offerId property
             */
            getNotRecentlyPlayed: feedNotRecentlyPlayed,
            /**
             * generates an array of games never played
             * @param  {number} daysSince the min number of days a user has not played this game
             * @return {object[]} a sorted array of objects with an offerId property
             */
            getNeverPlayed: feedNeverPlayed,
            /**
             * generates an array of unowned games sorted by current friends playing
             * @return {object[]} a sorted array of objects with an offerId property
             */
            getFriendsPlayingUnowned: feedFriendsPlayingUnowned,
            /**
             * generates an array of games recently played with no friends
             * @param  {number} daysSincePlayed the number of days since the user last played
             * @return {object[]} a sorted array of objects with an offerId property
             */
            getRecentlyPlayedNoFriends: feedRecentlyPlayed,
            /**
             * get the next unopened gift (if any)
             * @return {string} offerId
             */
            getNextUnopenedGift: feedNextUnopenedGift,
            /**
             * get offer that is acquired after the last played game (if there is any)
             * @return {string} offerId
             */
            getJustAcquired: feedJustAcquired,
            /**
             * return the newest trial that is
             * a) legacy trial
             * b) non-legacy trial && lastplayed
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
             * get the last downloaded game for the user if its newer than the last played
             * @return {string} offerId
             */
            getDownloadedGameNewerLastPlayed: feedDownloadedGameNewerLastPlayed,
            /**
             * give an amount of time since if a user has played a game
             * @param  {number} daysSince number of days lapsed since playing a game
             * @return {bool}             has played a game since the daysSince
             */
            getPlayedAGameRecently: feedPlayedAGameRecently,
            /**
             * generates an array of unowned DLC sorted by date
             * @return {object[]} a sorted array of objects with an offerId property
             */
            getUnownedDLC: feedUnownedDLC,
            /**
             * generates an array of objects containg the userid and and string of mutual friends
             * @return {object[]} retName array of id/mutual friend pairs sorted by number of mutual friends
             */
            getRecommendedFriends: feedRecommendedMutualFriends,
            /*
             * returns the most recently entitled game from the vault that is not yet installed, null if it doesn't exist
             * @return {string} offerId
             */
            getMostRecentSubsEntitlementNotInstalled: feedMostRecentSubsEntitlementNotInstalled,
            /**
             * returns the feed base game entitlement model
             * @return {object[]} array of objects with an offerId property
             */
            getBaseGameEntitlements: FeedModelsFactory.getBaseGameEntitlements,
            /**
             * applies randomization to the friends feed data based on the amount of friends show
             * @return {object[]} array of friends data
             */
            applyFriendsRandomization: applyFriendsRandomization
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */


    function FeedFactorySingleton(GamesDataFactory, ArrayHelperFactory, SubscriptionFactory, FeedModelsFactory, GameRefiner, EntitlementStateRefiner, LegacyTrialRefiner, DateHelperFactory, PlatformRefiner, GiftingFactory, ObjectHelperFactory, SingletonRegistryFactory) {
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
