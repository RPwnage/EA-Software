/**
 * @file game/gamesentitlement.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-prompt';

    function GamesEntitlementFactory(DialogFactory, UtilFactory, ComponentsLogFactory, ObjectHelperFactory, GameRefiner, EntitlementStateRefiner, SubscriptionFactory, GiftingFactory) {
        //events triggered from this service:

        //type of entitlement
        /*jshint unused: true */
        var OriginPermissionsNormal = '0',
            OriginPermissions1102 = '1',
            OriginPermissions1103 = '2';
        /*jshint unused: false */

        // The number of days an entitlement is considered new.
        var NumberOfMillisecondsEntitlementsAreConsideredNew = 432000000; // 5 * 24 * 60 * 60 * 1000

        var rawEntitlementsResponse = [], // raw entitlement response fron most recent /consolidatedentitlements call
            giftFilteredEntitlementsResponse = [], // 'rawEntitlementsResponse' after removing unopened gifts
            allEntitlements = {}, // map of all entitlements indexed by offer ID, after modification due to suppression/includes
            baseGameEntitlementsByOfferId = {}, // map of base game entitlements by offer ID
            extraContentEntitlements = {}, // map of extracontent entitlements indexed by masterTitleId
            extraContentEntitlementsByOfferId = {}, // map of extracontent entitlements indexed by offer ID
            currentlySuppressedOfferIds = [], // list of offer IDs for offers that are currently suppressed
            allCurrrentlySuppressedOfferIds = [], //list of offerIds that are currently suppresed including unowned
            currentlyIncludedOfferIds = [], // list of offer IDs for offers that are currently included
            isNewTimeoutsByOfferId = {}, // map of isNew timeouts indexed by offer ID
            initialRefreshFlag = false,
            myEvents = new Origin.utils.Communicator(),
            isBaseGame = GameRefiner.isBaseGame;

        function getEntitlement(offerId) {
            var ent = null;
            if (allEntitlements.hasOwnProperty(offerId)) {
                ent = allEntitlements[offerId];
            }
            return ent;
        }

        function extractSuppressedOfferIds(entitlements, catalog) {
            var suppressedOffers = [],
                catalogInfo;
            for (var offerId in entitlements) {
                // Subscription entitlements cannot suppress.
                if (isSubscriptionEntitlement(offerId)) {
                    continue;
                }

                catalogInfo = catalog[offerId];
                if (catalogInfo) {
                    suppressedOffers = suppressedOffers.concat(catalogInfo.suppressedOfferIds);
                } else {
                    // EC2 adds suppression hints to the entitlement response.  Use those when catalog data is not yet available for an offer.
                    suppressedOffers = suppressedOffers.concat(entitlements[offerId].suppressedOfferIds);
                }
            }
            return suppressedOffers;
        }

        function extractIncludeAssociations(entitlements, catalog) {
            var includeAssociations = {},
                catalogInfo,
                newEntitlement,
                includedOfferId;
            // Build a map of included offer ID to the entitlement that includes it.
            for (var offerId in entitlements) {
                catalogInfo = catalog[offerId];
                if (catalogInfo) {
                    for(var i = 0, length = catalogInfo.includeOffers.length; i < length; i++) {
                        includedOfferId = catalogInfo.includeOffers[i];
                        // Deep copy the raw entitlement and change the ID.
                        // This is necessary because "public" APIs like baseGameEntitlements() return
                        // an array of entitlement objects instead of a key-value map.
                        // TODO: Consider changing these APIs to return a key-value map.
                        newEntitlement = ObjectHelperFactory.copy(entitlements[offerId]);
                        newEntitlement.offerId = includedOfferId;
                        newEntitlement.productId = includedOfferId;
                        includeAssociations[includedOfferId] = newEntitlement;
                    }
                }
            }
            return includeAssociations;
        }

        function processSuppressions(catalog) {
            var suppressedOffers = [],
                newlySuppressedContent = [],
                newlyUnsuppressedContent = [];

            // Build list of suppressed content based on the entitlements the user owns.
            suppressedOffers = extractSuppressedOfferIds(allEntitlements, catalog);
            allCurrrentlySuppressedOfferIds = suppressedOffers; //keep the entire list before unowned is filtered out, will need it to filter EC

            // Remove unowned content from the list.
            suppressedOffers = suppressedOffers.filter(function(offerId) {
                return allEntitlements.hasOwnProperty(offerId);
            });

            // Calculate which offers are now suppressed and which offers were suppressed but are now unsuppressed.
            newlySuppressedContent = suppressedOffers.filter(function(offerId) {
                return currentlySuppressedOfferIds.indexOf(offerId) < 0;
            });
            newlyUnsuppressedContent = currentlySuppressedOfferIds.filter(function(offerId) {
                return suppressedOffers.indexOf(offerId) < 0;
            });

            // Logging
            if (newlySuppressedContent.length > 0) {
                ComponentsLogFactory.log('[GAMESENTITLEMENTFACTORY] Suppressed offers', newlySuppressedContent);
            }
            if (newlyUnsuppressedContent.length > 0) {
                ComponentsLogFactory.log('[GAMESENTITLEMENTFACTORY] Unsuppressed offers', newlyUnsuppressedContent);
            }

            // Update currently suppressed offers
            currentlySuppressedOfferIds = suppressedOffers;

            return suppressedOffers;
        }

        function processIncludes(catalog) {
            var includeAssociations = {},
                newlyIncludedContent = {},
                newlyUnincludedContent = {};

            // Build a map of included entitlements indexed by offer ID.
            includeAssociations = extractIncludeAssociations(allEntitlements, catalog);

            // Calculate which offers are now included and which offers were included but are no longer included.
            newlyIncludedContent = Object.keys(includeAssociations).filter(function(offerId) {
                return currentlyIncludedOfferIds.indexOf(offerId) < 0;
            });
            newlyUnincludedContent = currentlyIncludedOfferIds.filter(function(offerId) {
                return !includeAssociations.hasOwnProperty(offerId);
            });

            // Logging
            if (newlyIncludedContent.length > 0) {
                ComponentsLogFactory.log('[GAMESENTITLEMENTFACTORY] Included offers:', newlyIncludedContent);
            }
            if (newlyUnincludedContent.length > 0) {
                ComponentsLogFactory.log('[GAMESENTITLEMENTFACTORY] No longer included offers:', newlyUnincludedContent);
            }

            // Update currently included offers
            currentlyIncludedOfferIds = Object.keys(includeAssociations);

            return includeAssociations;
        }

        function doPostRefreshTasks(catalog) {
            var suppressedOffers,
                includeAssociations;

            // Add/remove included entitlements to our entitlement map.
            includeAssociations = processIncludes(catalog);
            angular.forEach(includeAssociations, function(entitlement, offerId) {
                if (!allEntitlements.hasOwnProperty(offerId)) {
                    allEntitlements[offerId] = entitlement;
                }
            });

            // Rebuild entitlement map so that it does not contain suppressed content.
            suppressedOffers = processSuppressions(catalog);
            angular.forEach(suppressedOffers, function(offerId) {
                delete allEntitlements[offerId];
            });

            // In the future, we may want to sort entitlements by originDisplayType here to handle the scenario where a base game becomes a DLC or vice versa.
            // However, that scenario would be exceedingly rare so it doesn't make much sense to waste CPU on every offer change event just to handle this.
        }

        function sortGames(a, b) {
            var aSubType = GameRefiner.getSubTypePriority(a.gameDistributionSubType),
                bSubType = GameRefiner.getSubTypePriority(b.gameDistributionSubType);
                if (aSubType === bSubType) {
                    if (!!a.gameEditionTypeFacetKeyRankDesc && !!b.gameEditionTypeFacetKeyRankDesc) {
                        var aRank = a.gameEditionTypeFacetKeyRankDesc,
                            bRank = b.gameEditionTypeFacetKeyRankDesc;
                    return bRank - aRank;
                    }
                }
            return bSubType - aSubType;
        }


        function determineBestMatch(ents) {
            var ent = null;
            ents.sort(sortGames);
            if (ents.length) {
                ent = ents[0];
            }
            return ent;
        }

        function masterTitleIdFilter(masterTitleId, item) {
            return _.get(item, ['masterTitleId'], '').toLowerCase() === masterTitleId.toLowerCase();
        }

        /**
         * returns all basegame entitlements for a master title id, sorted by rank
         * @param  {string} masterTitleId the master title id
         * @return {object[]} an array of basegame entitlements for the master title id
         */
        function getAllBaseGameEntitlementsByMasterTitleId(masterTitleId) {
            //converts the object of base game entitlements to array, filters out the ones with the matching master title id
            //and sorts based on distribution subttype and facet key rank
            return _.values(baseGameEntitlementsByOfferId)
                .filter(_.partial(masterTitleIdFilter, masterTitleId))
                .sort(sortGames);
        }

        function getBaseGameEntitlementByMasterTitleId(masterTitleId, catalog) {
            var ents = [],
                catalogInfo;
            for (var offerId in baseGameEntitlementsByOfferId) {
                catalogInfo = catalog[offerId];
                if (catalogInfo && !GameRefiner.isAlphaTrialDemoOrBeta(catalogInfo) && catalogInfo.masterTitleId && (masterTitleId.toLowerCase() === catalogInfo.masterTitleId.toLowerCase())) {
                    ents.push(baseGameEntitlementsByOfferId[offerId]);
                }
            }
            return determineBestMatch(ents);
        }

        /**
         * gets highest ranked base game entitlement by master title id
         * for games that might no longer be purchasable
         * @param {string} masterTitleId the master tile id to check
         * @return (string) offerid of entitlment
         */
        function getBaseGameEntitlementOfferIdByMasterTitleId(masterTitleId) {
            var ents = [];

            for (var offerId in baseGameEntitlementsByOfferId) {

                if(GameRefiner.isNormalGame(baseGameEntitlementsByOfferId[offerId]) && baseGameEntitlementsByOfferId[offerId].masterTitleId.toLowerCase() === masterTitleId.toLowerCase()) {
                    ents.push(baseGameEntitlementsByOfferId[offerId]);
                }
            }

            return _.get(determineBestMatch(ents), 'offerId', null);
        }

        function getBaseGameEntitlementByMultiplayerId(multiplayerId, catalog) {
            var ents = [],
                catPlatformData;
            for (var offerId in baseGameEntitlementsByOfferId) {
                catPlatformData = _.get(catalog, [offerId, 'platforms']);
                for (var platform in catPlatformData) {
                    if (_.get(catPlatformData, [platform, 'multiPlayerId']) === multiplayerId) {
                        ents.push(baseGameEntitlementsByOfferId[offerId]);
                    }
                }
            }
            return determineBestMatch(ents);
        }

        function getBaseGameEntitlementByAchievementSetId(achievementSetId, catalog) {
            var ents = [],
                catPlatformData;
            for (var offerId in baseGameEntitlementsByOfferId) {
                catPlatformData = _.get(catalog, [offerId, 'platforms']);
                for (var platform in catPlatformData) {
                    if (_.get(catPlatformData, [platform, 'achievementSet']) === achievementSetId) {
                        ents.push(baseGameEntitlementsByOfferId[offerId]);
                    }
                }
            }
            return determineBestMatch(ents);
        }

        function getMasterTitleIds(ent) {
            var masterTitleIds = [];
            if (ent) {
                masterTitleIds.concat(ent.alternateMasterTitleIds);
                if (ent.masterTitleId) {
                    masterTitleIds.push(ent.masterTitleId);
                }
            }
            return masterTitleIds;
        }

        function fireUpdateIsNew(entitlement) {
            return function() {
                entitlement.isNew = false;
                myEvents.fire('GamesEntitlementFactory:isNewUpdate', {
                    signal: 'isNewUpdate:' + entitlement.offerId,
                    eventObj: false
                });
            };
        }

        function processEntitlementsFromResponse(response) {
            var len = response.length,
                i, ent;

            var currentTime = new Date(Origin.datetime.getTrustedClock());
            var millisecondsUntilNotNew = 0;
            var responseEntitlements = {};

            giftFilteredEntitlementsResponse = response;

            // Loop over the entitlements.
            for (i = 0; i < len; i++) {
                ent = response[i];
                ent.productId = ent.offerId;

                // Convert dates to JS Date type.
                ent.grantDate = ent.grantDate ? new Date(ent.grantDate) : null;
                ent.terminationDate = ent.terminationDate ? new Date(ent.terminationDate) : null;

                // Determine if the entitlement is new.
                if (ent.grantDate) {
                    if (Math.abs(currentTime - ent.grantDate) < NumberOfMillisecondsEntitlementsAreConsideredNew) {
                        ent.isNew = true;
                        millisecondsUntilNotNew = Math.abs((ent.grantDate.getTime() + NumberOfMillisecondsEntitlementsAreConsideredNew) - currentTime);
                    }
                }

                // If the entitlement will be considered not new, and if we haven't already set a timeout for doing so,
                if (millisecondsUntilNotNew > 0 && ! (isNewTimeoutsByOfferId.hasOwnProperty(ent.offerId))) {

                    // Fire an event at the appropriate time to indicate the entitlement is no longer new.
                    isNewTimeoutsByOfferId[ent.offerId] = setTimeout(fireUpdateIsNew(ent), millisecondsUntilNotNew);
                }

                responseEntitlements[ent.offerId] = ent;
            }

            allEntitlements = ObjectHelperFactory.copy(responseEntitlements);

            sortEntitlementsByDisplayType();
        }

        function handleConsolidatedEntitlementSuccess(response) {
            processEntitlementsFromResponse(response);
            setInitialRefreshFlag(true);
            myEvents.fire('GamesEntitlementFactory:getEntitlementSuccess', {});
            return getGiftFilteredOfferIds();
        }

        function handleConsolidatedEntitlementsError(error) {
            DialogFactory.openAlert({
                id: 'web-entitlement-server-down-warning',
                title: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'entitletmentserverdowntitle'),
                description: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'entitletmentserverdowndesc'),
                rejectText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'closeprompt')
            });

            setInitialRefreshFlag(true);
            throw error;
        }

        function sortEntitlementsByDisplayType() {
            var ent,
                masterTitleId,
                masterTitleIds;

            baseGameEntitlementsByOfferId = {};
            extraContentEntitlements = {};
            extraContentEntitlementsByOfferId = {};

            for(var offerId in allEntitlements) {
                ent = allEntitlements[offerId];
                if (ent) {
                    if (isBaseGame(ent)) {
                        baseGameEntitlementsByOfferId[offerId] = ent;
                    } else {
                        masterTitleIds = getMasterTitleIds(ent);
                        for(var i = 0, length = masterTitleIds.length; i < length; i++) {
                            masterTitleId = masterTitleIds[i];
                            if (!extraContentEntitlements.hasOwnProperty(masterTitleId)) {
                                extraContentEntitlements[masterTitleId] = [];
                            }
                            extraContentEntitlements[masterTitleId].push(ent);
                        }
                        extraContentEntitlementsByOfferId[offerId] = ent;
                    }
                }
            }
        }

        function isPrivate(entitlement) {
            return entitlement && (entitlement.originPermissions === OriginPermissions1102 || entitlement.originPermissions === OriginPermissions1103);
        }

        function getPrivateEntitlements() {
            return Object.keys(_.pick(baseGameEntitlementsByOfferId, isPrivate));
        }

        function baseEntitlementOfferIdList() {
            return Object.keys(baseGameEntitlementsByOfferId);
        }

        /**
         * check if an entitlement is owned outright,
         * or an active subscriber vault entitlement
         * @param {Object} ent entitlement data
         * @return {Boolean}
         */
        function isOwnedOrActiveSubscription(ent) {
            var type = _.get(ent, 'externalType');

            return (!type || type !== 'SUBSCRIPTION' || (type === 'SUBSCRIPTION' && SubscriptionFactory.userHasSubscription()));
        }

        function ownsEntitlement(offerId) {
            var ent = _.get(allEntitlements, offerId);

            // check if subscription entitlement and user is subscriber
            if(!!ent && isOwnedOrActiveSubscription(ent)) {
                return true;
            }

            return false;
        }

        function ownsEntitlementByPath(ocdPath, checkOTHEntitlement) {
            for(var offerId in allEntitlements) {
                var ent = allEntitlements[offerId];

                //if the ocd path matches and if the checkOTHEntitlement flag is set see if entitlement was granted via OTH
                if((ent && ent.offerPath === ocdPath) && (!checkOTHEntitlement || EntitlementStateRefiner.isOTHGranted(ent)) && isOwnedOrActiveSubscription(ent)) {
                    return true;
                }
            }
            return false;
        }

        function isSubscriptionEntitlement(offerId) {
            if (allEntitlements.hasOwnProperty(offerId)) {
                return (!!allEntitlements[offerId].externalType && allEntitlements[offerId].externalType === 'SUBSCRIPTION');
            }
            return false;
        }

        function clearEntitlements() {
            rawEntitlementsResponse = [];
            giftFilteredEntitlementsResponse = [];
            allEntitlements = {};
            baseGameEntitlementsByOfferId = {};
            extraContentEntitlements = {};
            extraContentEntitlementsByOfferId = {};
            currentlySuppressedOfferIds = [];
            currentlyIncludedOfferIds = [];
            setInitialRefreshFlag(false);
            for (var offerId in isNewTimeoutsByOfferId) {
                if (isNewTimeoutsByOfferId.hasOwnProperty(offerId)) {
                    clearTimeout(isNewTimeoutsByOfferId[offerId]);
                }
            }
            isNewTimeoutsByOfferId = {};
        }

        function getGiftFilteredOfferIds() {
            return _.map(giftFilteredEntitlementsResponse, function(ent) {
                return ent.offerId;
            });
        }

        function cacheRawEntitlementsResponse(entitlements) {
            rawEntitlementsResponse = entitlements;
            return entitlements;
        }

        function processEntitlements(entitlements, gifts) {
            return Promise.resolve(GiftingFactory.extractUnopenedGifts(entitlements,gifts))
                .then(_.spread(filterOutUnopenedGiftEntitlements))
                .then(handleConsolidatedEntitlementSuccess);
        }

        function addGiftsToEntitlements(forceRetrieveGifts, entitlements) {
            if(_.some(entitlements, {'externalType': 'GIFTING'})) {
                return GiftingFactory.retrieveGifts(forceRetrieveGifts)
                    .then(function(gifts) {
                        return [entitlements, gifts];
                    });
            }

            return [entitlements, []];
        }

        function retrieveEntitlements(forceRetrieve) {
            // If we already have the entitlements and its not a force refresh, return the cached entitlements
            if (initialRefreshFlag && !forceRetrieve) {
                return Promise.resolve(getGiftFilteredOfferIds());
            }

            // Otherwise let's make a new request
            return Origin.games.consolidatedEntitlements(forceRetrieve)
                .then(cacheRawEntitlementsResponse)
                .then(_.partial(addGiftsToEntitlements, true, _))
                .then(_.spread(processEntitlements))
                .catch(handleConsolidatedEntitlementsError);
        }

        function updateGiftEntitlements() {
            return Promise.resolve(rawEntitlementsResponse)
                .then(_.partial(addGiftsToEntitlements, false, _))
                .then(_.spread(processEntitlements))
                .catch(handleConsolidatedEntitlementsError);
        }

        /**
         * Remove unopened gifts from passed in entitlements.
         * @param  {Array<EntitlementObject>}  entitlements  all user entitlements retrieved from server
         * @param  {Array<gifts>}              unopenedGifts list of unopened gifts
         * @return {Array<entitlementObject>}                entitlements with unopened gifts removed
         */
        function filterOutUnopenedGiftEntitlements(entitlements, unopenedGifts) {
            var giftFilteredEntitlementsArray = _.clone(entitlements),
                giftId = '';

            if (unopenedGifts) {
                // loop over each unopened gift and remove it (and related items in case of a bundle) from the entitlements list
                _.forEach(unopenedGifts, function(gift) {
                    giftId = gift.giftId.toString();

                    // remove unopened gift from entitlement list by giftId
                    giftFilteredEntitlementsArray = _.reject(giftFilteredEntitlementsArray, {'externalId': giftId});
                });
            }
            else {
                // remove all gifts from entitlements
                giftFilteredEntitlementsArray = _.reject(giftFilteredEntitlementsArray, {'externalType': 'GIFTING'});
            }

            return giftFilteredEntitlementsArray;
        }

        /**
         * Directly entitle offer to user
         * @param  {string} offerId Offer ID
         * @return {Promise}
         */
        function directEntitle(offerId) {
            return Origin.games.directEntitle(offerId);
        }

        function setInitialRefreshFlag(boolval) {
                initialRefreshFlag = boolval;
        }

        return {

            events: myEvents,

            directEntitle: directEntitle,

            clearEntitlements: clearEntitlements,

            /**
             * returns true if the initial base game entitlement request completed
             * @return {boolean}
             * @method initialRefreshCompleted
             */
            initialRefreshCompleted: function() {
                ComponentsLogFactory.log('GamesEntitlementFactory: initialRefreshCompleted', initialRefreshFlag);
                return initialRefreshFlag;
            },

            /**
             * sets initialRefreshFlag
             * @method initialRefreshCompleted
             */
            setInitialRefreshFlag: setInitialRefreshFlag,

            /**
             * refreshes the user's entitlements
             * @param {boolean} forceRetrieve if true, entitlements will be fetched from server even if cached data exists
             * @return {promise<EntitlementObject[]>}
             * @method retrieveEntitlements
             */
            retrieveEntitlements: retrieveEntitlements,

            /**
             * refreshes the user's entitlements entitlements will be fetched from server even if cached data exists
             * @return {promise<EntitlementObject[]>}
             * @method forceRetrieveEntitlements
             */
            forceRetrieveEntitlements: retrieveEntitlements.bind(this, true),

            /**
             * return the cached unique base game entitlements
             * @return {object[]}
             * @method baseGameEntitlements
             */
            baseGameEntitlements: function() {
                return _.values(baseGameEntitlementsByOfferId);
            },

            /**
             * returns whether or not this entitlement is private; i.e., whether an 1102/1103 code is applicable.
             * @return {boolean}
             * @method isPrivate
             */
            isPrivate: isPrivate,

            /**
             * returns just the list of offerIds for each entitlement
             * @return {string[]}
             * @method baseEntitlementOfferIdList
             */
            baseEntitlementOfferIdList: baseEntitlementOfferIdList,
            /**
             * given a master title id we return all the associated base game entitlements
             * @return {object[]} the array of entitlements
             * @method getAllBaseGameEntitlementsByMasterTitleId
             */
            getAllBaseGameEntitlementsByMasterTitleId: getAllBaseGameEntitlementsByMasterTitleId,
            /**
             * given a master title id we return an owned base game entitlement, return null if no entitlement
             * @return {string}
             * @method getBaseGameEntitlementByMasterTitleId
             */
            getBaseGameEntitlementByMasterTitleId: getBaseGameEntitlementByMasterTitleId,

            /**
             * given a master title id we return the highest ranked base game entitlement, return null if no entitlement
             * can return non purchasable entitlements
             * @return {string} offerId
             * @method getBaseGameEntitlementOfferIdByMasterTitleId
             */
            getBaseGameEntitlementOfferIdByMasterTitleId: getBaseGameEntitlementOfferIdByMasterTitleId,

            /**
             * given a multiplayer id we return an owned base game entitlement, return null if no entitlement
             * @return {string}
             * @method getBaseGameEntitlementByMultiplayerId
             */
            getBaseGameEntitlementByMultiplayerId: getBaseGameEntitlementByMultiplayerId,

            /**
             * given an achievement set id we return an owned base game entitlement, return null if no entitlement
             * @return {string}
             * @method getBaseGameEntitlementByAchievementSetId
             */
            getBaseGameEntitlementByAchievementSetId: getBaseGameEntitlementByAchievementSetId,

            /**
             * given an offerId, returns the entitlement object, otherwise it returns NULL
             * @return {string}
             * @method getEntitlement
             */
            getEntitlement: getEntitlement,

            /**
             * returns all the entitlements for a user
             * @return {Object[]}
             * @method getAllEntitlements
             */
            getAllEntitlements: function() {
                return allEntitlements;
            },

            /**
             * returns all OWNED extra content entitlements indexed by masterTitleId
             * @return {object} a map of owned entitlements indexed by masterTitleId
             * @method extraContentEntitlements
             */
            extraContentEntitlements: function() {
                return extraContentEntitlements;
            },

            /**
             * given a masterTitleId, returns all OWNED extra content entitlements
             * @param {string} masterTitleId
             * @return {string[]} a list of owned entitlements
             * @method getExtraContentEntitlementsByMasterTitleId
             */
            getExtraContentEntitlementsByMasterTitleId: function(masterTitleId) {
                return extraContentEntitlements[masterTitleId];
            },

            /**
             * applies suppression to all entitlements, builds the base game/extracontent lists, and processes offer includes
             * @param {object} catalog data
             * @method doPostRefreshTasks
             */
            doPostRefreshTasks: doPostRefreshTasks,

            /**
             * returns an array of private entitlements
             * @return {string[]}
             * @method getPrivateEntitlements
             */
            getPrivateEntitlements: getPrivateEntitlements,

            /**
             * returns true if an entitlement exists and it was entitled thru subscriptions
             * @param {string} offerId
             * @return {boolean}
             * @method isSubscriptionEntitlement
             */
            isSubscriptionEntitlement: isSubscriptionEntitlement,

            /**
             * returns true if an entitlement exists for the offer
             * @param {string} offerId
             * @return {boolean}
             * @method ownsEntitlement
             */
            ownsEntitlement: ownsEntitlement,

            /**
             * returns true if an entitlement exists for the offer path
             * @param {string} offerPath
             * @return {boolean}
             * @method ownsEntitlementByPath
             */
            ownsEntitlementByPath: ownsEntitlementByPath,

            /**
             * return the cached unique base game entitlements indexed by offer ID
             * @return {object}
             * @method getBaseGameEntitlements
             */
            getBaseGameEntitlements: function() {
                return baseGameEntitlementsByOfferId;
            },

           /**
             * returns all OWNED extra content entitlements indexed by offer ID
             * @return {object} a map of owned entitlements indexed by offer ID
             * @method getExtraContentEntitlements
             */
            getExtraContentEntitlements: function() {
                return extraContentEntitlementsByOfferId;
            },

            /**
             * returns true if the catalog object is a base game
             * @param {object} catalogObject
             * @return {boolean}
             * @method isBaseGame
             */
            isBaseGame: isBaseGame,

            allSuppressedOffers: function() {
                return allCurrrentlySuppressedOfferIds;
            },

            /**
             * Retrieve entitlements with cached entitlements and gift information
             * @return {promise<EntitlementObject[]>}
             */
            updateGiftEntitlements: updateGiftEntitlements
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesEntitlementFactorySingleton(DialogFactory, UtilFactory, ComponentsLogFactory, ObjectHelperFactory, GameRefiner, EntitlementStateRefiner, SubscriptionFactory, GiftingFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesEntitlementFactory', GamesEntitlementFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesEntitlementFactory

     * @description
     *
     * GamesEntitlementFactory
     */
    angular.module('origin-components')
        .factory('GamesEntitlementFactory', GamesEntitlementFactorySingleton);
}());
