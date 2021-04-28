/**
 * @file game/gamesentitlement.js
 */
(function() {
    'use strict';

    function GamesEntitlementFactory(DialogFactory, ComponentsLogFactory) {
        //events triggered from this service:

        //type of entitlement
        /*jshint unused: true */
        var OriginPermissionsNormal = '0',
            OriginPermissions1102 = '1',
            OriginPermissions1103 = '2';
        /*jshint unused: false */

        // The number of days an entitlement is considered new.
        var NumberOfMillisecondsEntitlementsAreConsideredNew = 432000000; // 5 * 24 * 60 * 60 * 1000

        var baseGameEntitlements = {}, // map of base game entitlements indexed by offer ID
            extraContentEntitlements = {}, // map of extracontent entitlements indexed by offer ID
            baseGameOfferIds = [], // array of all base game offer IDs
            extraContentOfferIds = [], // array of all extra content offer IDs
            bestMatchBaseGameEntitlements = [], // list of all unsuppressed "best match" base game entitlements
            bestMatchExtraContentEntitlements = {}, // map of all unsuppressed "best match" extracontent entitlements indexed by masterTitleId
            currentlySuppressedOfferIds = [], // list of offer IDs for offers that are currently suppressed
            isNewTimeoutsByOfferId = {}, // map of isNew timeouts indexed by offer ID
            initialRefreshFlag = false,
            myEvents = new Origin.utils.Communicator();

        function findEntitlementInEntitlementArray(entitlementId, entList) {
            var ent = null;

            for (var i = 0; i < entList.length; i++) {
                if (entList[i].entitlementId === entitlementId) {
                    ent = entList[i];
                    break;
                }
            }

            return ent;
        }

        function findEntitlementInEntitlementMap(entitlementId, entitlementMap) {
            var id = null;
            var entitlement;
            for (id in entitlementMap) {
                if (entitlementMap.hasOwnProperty(id)) {
                    entitlement = findEntitlementInEntitlementArray(entitlementId, entitlementMap[id]);
                    if (entitlement) {
                        return entitlement;
                    }
                }
            }
        }

        function entitlementByEntitlementId(entitlementId) {
            var id = null;
            var entitlement = findEntitlementInEntitlementMap(entitlementId, baseGameEntitlements);

            if (!entitlement) {
                entitlement = findEntitlementInEntitlementMap(entitlementId, extraContentEntitlements);
            }

            return entitlement;
        }

        function getEntitlement(offerId, entitlementId) {
            var ent, entList = [];
            if (baseGameEntitlements.hasOwnProperty(offerId)) {
                entList = baseGameEntitlements[offerId];
            } else if (extraContentEntitlements.hasOwnProperty(offerId)) {
                entList = extraContentEntitlements[offerId];
            }

            // If caller specified an entitlementId, return him that specific entitlement. If not, return him the best match.

            ent = entitlementId ? findEntitlementInEntitlementArray(entitlementId, entList) : entList[0];

            return ent;
        }

        function removeEntitlement(offerId, entitlementId) {
            var entitlementMap;
            if (baseGameEntitlements.hasOwnProperty(offerId)) {
                entitlementMap = baseGameEntitlements;
            } else if (extraContentEntitlements.hasOwnProperty(offerId)) {
                entitlementMap = extraContentEntitlements;
            }

            if (entitlementMap) {
                var i, found = false;
                for (i = 0; i < entitlementMap[offerId].length; i++) {
                    if (entitlementMap[offerId][i].entitlementId === entitlementId) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    // If it does exist, remove it
                    entitlementMap[offerId].splice(i, 1);
                    // Remove the offer ID from our map if there are no more entitlements for this offer
                    if (entitlementMap[offerId].length === 0) {
                        delete entitlementMap[offerId];
                    }
                }
            }
        }

        function extractBestMatchEntitlements(entitlements) {
            var bestMatchEntitlements = [],
                offerIds = Object.keys(entitlements);
            for (var i = 0; i < offerIds.length; i++) {
                // First entitlement in the list is the "best match"
                bestMatchEntitlements.push(entitlements[offerIds[i]][0]);
            }
            return bestMatchEntitlements;
        }

        function extractSuppressedOfferIds(entitlements, catalog) {
            var suppressedOffers = [];
            for (var i = 0; i < entitlements.length; i++) {
                var catalogInfo = catalog[entitlements[i].offerId];
                if (catalogInfo) {
                    suppressedOffers = suppressedOffers.concat(catalogInfo.suppressedOfferIds);
                } else {
                    // EC2 adds suppression hints to the entitlement response.  Use those when catalog data is not yet available for an offer.
                    suppressedOffers = suppressedOffers.concat(entitlements[i].suppressedOfferIds);
                }
            }
            return suppressedOffers;
        }

        function addExtraContent(ent, catalog) {
            var catalogInfo = catalog[ent.offerId];
            if (catalogInfo && catalogInfo.originDisplayType !== 'Full Game') {
                var masterTitleIds = [];
                if (catalogInfo.masterTitleId) {
                    masterTitleIds.push(catalogInfo.masterTitleId);
                }
                if (catalogInfo.alternateMasterTitleIds.length > 0) {
                    masterTitleIds = masterTitleIds.concat(catalogInfo.alternateMasterTitleIds);
                }
                for (var i = 0; i < masterTitleIds.length; i++) {
                    var masterTitleId = masterTitleIds[i];
                    if (!bestMatchExtraContentEntitlements.hasOwnProperty(masterTitleId)) {
                        bestMatchExtraContentEntitlements[masterTitleId] = [];
                    }
                    bestMatchExtraContentEntitlements[masterTitleId].push(ent);
                }
            }
        }

        function processSuppressions(catalog) {
            bestMatchBaseGameEntitlements = [];
            bestMatchExtraContentEntitlements = {};

            var allBaseGames = extractBestMatchEntitlements(baseGameEntitlements),
                allExtraContent = extractBestMatchEntitlements(extraContentEntitlements),
                suppressedOffers = [],
                newlySuppressedContent = [],
                newlyUnsuppressedContent = [],
                i;

            // Build list of suppressed content based on the entitlements the user owns.
            suppressedOffers = suppressedOffers.concat(extractSuppressedOfferIds(allBaseGames, catalog));
            suppressedOffers = suppressedOffers.concat(extractSuppressedOfferIds(allExtraContent, catalog));

            // Remove unowned content from the list.
            suppressedOffers = suppressedOffers.filter(function(offerId) {
                return baseGameEntitlements.hasOwnProperty(offerId) || extraContentEntitlements.hasOwnProperty(offerId);
            });

            // Calculate which offers are now suppressed and which offers were suppressed but are now unsuppressed.
            newlySuppressedContent = suppressedOffers.filter(function(offerId) {
                return currentlySuppressedOfferIds.indexOf(offerId) < 0;
            });
            newlyUnsuppressedContent = currentlySuppressedOfferIds.filter(function(offerId) {
                return suppressedOffers.indexOf(offerId) < 0;
            });

            // Rebuild "best match" lists so that they do not contain suppressed content.
            for (i = 0; i < allBaseGames.length; i++) {
                if (suppressedOffers.indexOf(allBaseGames[i].offerId) < 0) {
                    // If this entitlement's offer is not suppressed, add it to our bestMatchBaseGameEntitlements list.
                    bestMatchBaseGameEntitlements.push(allBaseGames[i]);

                    // If we have catalog data for it and it's not a full game, add it to our bestMatchExtraContentEntitlements map as well.
                    // This is typical of a legacy expansion, i.e. Sims3 expansions.
                    addExtraContent(allBaseGames[i], catalog);
                }
            }

            for (i = 0; i < allExtraContent.length; i++) {
                if (suppressedOffers.indexOf(allExtraContent[i].offerId) < 0) {
                    // If this entitlement's offer is not suppressed, add it to our bestMatchExtraContentEntitlements map.
                    addExtraContent(allExtraContent[i], catalog);
                }
            }

            // Logging
            for (i = 0; i < newlySuppressedContent.length; i++) {
                ComponentsLogFactory.log('[GAMESENTITLEMENTFACTORY] Suppressed offer', newlySuppressedContent[i]);
            }
            for (i = 0; i < newlyUnsuppressedContent.length; i++) {
                ComponentsLogFactory.log('[GAMESENTITLEMENTFACTORY] Unsuppressed offer', newlyUnsuppressedContent[i]);
            }

            // Update currently suppressed offers
            currentlySuppressedOfferIds = suppressedOffers;

            return Promise.resolve({
                newlySuppressedContent: newlySuppressedContent,
                newlyUnsuppressedContent: newlyUnsuppressedContent
            });
        }

        function compareEntitlements(entA, entB) {
            var entAIsOwned = (entA.entitlementId !== 0);
            var entBIsOwned = (entB.entitlementId !== 0);
            var entAIsDownload = entA.entitlementTag === 'ORIGIN_DOWNLOAD';
            var entBIsDownload = entB.entitlementTag === 'ORIGIN_DOWNLOAD';
            var entAIsPreorder = entA.entitlementTag === 'ORIGIN_PREORDER';
            var entBIsPreorder = entB.entitlementTag === 'ORIGIN_PREORDER';
            var entAHasCdKey = entA.cdKey !== undefined;
            var entBHasCdKey = entB.cdKey !== undefined;

            var entAWins = false;
            if (entB === undefined) {
                entAWins = true;
            } else if (entA.grantDate !== entB.grantDate) {
                entAWins = (entA.grantDate > entB.grantDate);
            } else if (entAIsOwned !== entBIsOwned) {
                entAWins = entAIsOwned;
            } else if (entAIsDownload !== entBIsDownload) {
                // ORIGIN_DOWNLOAD entitlements are preferred above all others, because they are the only entitlements that will contain cdkey
                entAWins = entAIsDownload;
            } else if (entAIsPreorder !== entBIsPreorder) {
                // Add the new entitlement only if it is not the pre-order entitlement
                entAWins = entBIsPreorder;
            } else if (entA.originPermissions !== entB.originPermissions) {
                // Add the new entitlement only if it has the better Origin permissions.
                entAWins = (entA.originPermissions > entB.originPermissions);
            } else if (entAHasCdKey !== entBHasCdKey) {
                // We want to preserve a cdkey if one is available
                entAWins = !entBHasCdKey;
            } else if (entA.entitlementId !== entB.entitlementId) {
                // NOTE: This check needs to always be last.  We only want to use IDs to sort if the entitlements
                // are the same in every way that matters, and we just need a way to deterministically sort so
                // the same entitlement is always chosen as "best."

                // Consistently choose the higher entitlement id when everything else is equal
                entAWins = (entA.entitlementId > entB.entitlementId);
            }

            return (entAWins ? -1: 1);
        }

        function getBaseGameEntitlementByMasterTitleId(masterTitleId, catalog) {
            var ent = null;
            for (var i = 0; i < bestMatchBaseGameEntitlements.length; i++) {
                var catalogInfo = catalog[bestMatchBaseGameEntitlements[i].productId];
                if (catalogInfo && catalogInfo.masterTitleId && (Number(masterTitleId) === catalogInfo.masterTitleId)) {
                    ent = bestMatchBaseGameEntitlements[i];

                    break;
                }
            }
            return ent;
        }

        function getBaseGameEntitlementByMultiplayerId(multiplayerId, catalog) {
            var ent = null;
            for (var i = 0; i < bestMatchBaseGameEntitlements.length; i++) {
                var productId = bestMatchBaseGameEntitlements[i].productId;
                var mpId = Origin.utils.getProperty(catalog, [productId, 'platforms', Origin.utils.os(), 'multiPlayerId']);
                if (mpId === multiplayerId) {
                    ent = bestMatchBaseGameEntitlements[i];

                    break;
                }
            }
            return ent;
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

        function processEntitlementsFromResponse(response, entitlementMap) {
            var len = response.length,
                updatedOffers = [],
                i, j, ent, existingEnt;

            var currentTime = new Date(Origin.datetime.getTrustedClock());
            var millisecondsUntilNotNew = 0;

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


                existingEnt = getEntitlement(ent.productId, ent.entitlementId);
                if (existingEnt) {
                    if (ent.status === 'DELETED') {
                        removeEntitlement(ent.productId, ent.entitlementId);
                        updatedOffers.push(ent.productId);
                        //TODO: emit signal to indicate entitlement has been deleted
                    } else {
                        // Update the entitlement in-place.
                        Origin.utils.mix(existingEnt, ent);
                        updatedOffers.push(ent.productId);
                    }
                } else if (ent.status !== 'DELETED') {
                    if (!entitlementMap.hasOwnProperty(ent.productId)) {
                        entitlementMap[ent.productId] = [];
                    }
                    entitlementMap[ent.productId].push(ent);
                    updatedOffers.push(ent.productId);
                    //TODO: emit signal to indicate new entitlement
                }
            }

            // Re-sort "best match" entitlement list for any offers that have new/modified/deleted entitlements.
            for (i = 0; i < updatedOffers.length; i++) {
                if (entitlementMap.hasOwnProperty(updatedOffers[i])) {
                    entitlementMap[updatedOffers[i]].sort(compareEntitlements);
                }
            }
        }


        function processEntitlementsFromResponseBaseGames(response) {
            processEntitlementsFromResponse(response, baseGameEntitlements);
        }

        function processEntitlementsFromResponseExtraContent(response) {
            processEntitlementsFromResponse(response, extraContentEntitlements);
        }

        function handleBaseGamesEntitlementSuccess(response) {
            baseGameEntitlements = {};
            baseGameOfferIds = [];

            //for now, filter only those that are downloadable
            var i, filteredResponse = [];
            for (i = 0; i < response.length; i++) {
                var ent = response[i];
                if ((ent.entitlementTag === 'ORIGIN_DOWNLOAD' || ent.entitlementTag === 'ORIGIN_PREORDER') && ent.productId !== 'undefined' && ent.entitlementId !== 'undefined') {
                    filteredResponse.push(response[i]);
                }
            }

            processEntitlementsFromResponse(filteredResponse, baseGameEntitlements);
            baseGameOfferIds = Object.keys(baseGameEntitlements);

            initialRefreshFlag = true;
            return baseGameOfferIds;
        }

        function handleBaseGamesEntitlementError(error) {
            DialogFactory.openAlert({
                id: 'web-entitlement-server-down-warning',
                title: 'Entitlement server is down.',
                description: 'Not all elements of the page will load. Please refresh the page. If this happens for an extended period of time please email originxcore@ea.com',
                rejectText: 'OK'
            });
            initialRefreshFlag = true;
            throw error;
        }

        function catchBaseGamesEntitlementError(error) {
            initialRefreshFlag = true;
            ComponentsLogFactory.error('[GAMESENTITLEMENTFACTORY:refreshBaseGameEntitlements]', error.stack);
            throw error;
        }

        //force the call to go out to the serve to retrieve entitlements
        function refreshBaseGameEntitlements() {
            ComponentsLogFactory.log('GamesEntitlementFactory: refreshBaseGameEntitlements');

            return Origin.games.baseGameEntitlements(true)
                    .then(handleBaseGamesEntitlementSuccess, handleBaseGamesEntitlementError)
                    .catch(catchBaseGamesEntitlementError);
        }


            function handleExtraContentEntitlmentResponse(responseObj) {
                processEntitlementsFromResponse(responseObj.entitlements, extraContentEntitlements);
                extraContentOfferIds = Object.keys(extraContentEntitlements);
            return extraContentOfferIds;
            }

            function handleExtraContentEntitlementError(errorObj) {
                ComponentsLogFactory.error('handleExtraContentEntitlementError:', errorObj.masterTitleId, errorObj.stack);
                // By returning [] here, we prevent the Promise.all from failing so we can continue looking up other extracontent
                return [];
            }

        function refreshExtraContentEntitlementForMasterTitleId(masterTitleId) {

            return Origin.games.extraContentEntitlements(masterTitleId, true)
                .then(handleExtraContentEntitlmentResponse, handleExtraContentEntitlementError);
        }

        function refreshExtraContentEntitlements(catalog) {

            var extraContentEntitlementRequests = [];

            //clear previously retrieved
            extraContentEntitlements = {};

            //iterate thru each entitlement, and extract the masterTitleId from catalog associated with entitlement
            //pass that masterTitleId to retrieve the owned extra content entitlement associated with the masterTitleId
            var queriedMasterTitleIds = [];
            for (var i = 0; i < baseGameOfferIds.length; i++) {
                var entOfferId = baseGameOfferIds[i];
                if (catalog[entOfferId] && catalog[entOfferId].extraContent.length > 0) {
                    //check and make sure we don't already have it for this masterTitleId
                    if (catalog[entOfferId].masterTitleId) {
                        var masterTitleId = catalog[entOfferId].masterTitleId;
                        if (queriedMasterTitleIds.indexOf(masterTitleId) === -1) {
                            extraContentEntitlementRequests.push(refreshExtraContentEntitlementForMasterTitleId(masterTitleId));
                            queriedMasterTitleIds.push(masterTitleId);
                        }
                    } else {
                        ComponentsLogFactory.warn('refreshExtraContentEntitlements - missing masterTitleId:', entOfferId);
                    }
                } else {
                    if (!catalog[entOfferId]) {
                        ComponentsLogFactory.warn('refreshExtraContentEntitlements - catalog not loaded for:', entOfferId);
                    } else {
                        ComponentsLogFactory.warn('refreshExtraContentEntitlements - no extra content for:', entOfferId);
                    }
                }
            }

            return Promise.all(extraContentEntitlementRequests).then(function() {
                return extraContentOfferIds;
            });

        }

        function isPrivate(entitlement) {
            return entitlement && (entitlement.originPermissions === OriginPermissions1102 || entitlement.originPermissions === OriginPermissions1103);
        }

        function getPrivateEntitlements() {
            return bestMatchBaseGameEntitlements.filter(isPrivate).map(function(ent) {
                return ent.offerId;
            });
        }

        function baseEntitlementOfferIdList() {
            return baseGameOfferIds;
        }

        function ownsEntitlement(offerId) {
            return (baseGameEntitlements.hasOwnProperty(offerId) || extraContentEntitlements.hasOwnProperty(offerId));
        }

        function isSubscriptionEntitlement(offerId) {
            if (baseGameEntitlements.hasOwnProperty(offerId)) {
                return (!!baseGameEntitlements[offerId][0].externalType && baseGameEntitlements[offerId][0].externalType === 'SUBSCRIPTION');
            }

            if (extraContentEntitlements.hasOwnProperty(offerId)) {
                return (!!extraContentEntitlements[offerId][0].externalType && extraContentEntitlements[offerId][0].externalType === 'SUBSCRIPTION');
            }
            return false;
        }

        function clearEntitlements() {
            baseGameEntitlements = {};
            extraContentEntitlements = {};
            baseGameOfferIds = [];
            bestMatchBaseGameEntitlements = [];
            bestMatchExtraContentEntitlements = {};
            currentlySuppressedOfferIds = [];
            initialRefreshFlag = false;
            for (var offerId in isNewTimeoutsByOfferId) {
                if (isNewTimeoutsByOfferId.hasOwnProperty(offerId)) {
                    clearTimeout(isNewTimeoutsByOfferId[offerId]);
                }
            }
            isNewTimeoutsByOfferId = {};
        }

        return {

            events: myEvents,

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
            setInitialRefreshFlag: function(boolval) {
                initialRefreshFlag = boolval;
            },

            /**
             * return the cached unique base game entitlements
             * @return {string[]}
             * @method baseGameEntitlements
             */
            baseGameEntitlements: function() {
                return bestMatchBaseGameEntitlements;
            },

            /**
             * returns whether or not this entitlement is private; i.e., whether an 1102/1103 code is applicable.
             * @return {boolean}
             * @method isPrivate
             */
            isPrivate: isPrivate,

            /**
             * Removes an entitlement from our model
             * @param {string} offerId offerId of the game
             * @param {string} entitlementId entitlementId for the game
             * @method
             */
            removeEntitlement: removeEntitlement,
            /**
             * Finds an entitlement by the entitlement id
             * @param {string} entitlementId entitlementId of the game
             * @method
             */
            entitlementByEntitlementId: entitlementByEntitlementId,
            /**
             * returns just the list of offerIds for each entitlement
             * @return {string[]}
             * @method baseEntitlementOfferIdList
             */
            baseEntitlementOfferIdList: baseEntitlementOfferIdList,
            /**
             * force retrieve the base game entitlements from the
             * @return {promise<string[]>}
             * @method refreshBaseGameEntitlements
             */
            refreshBaseGameEntitlements: refreshBaseGameEntitlements,

            /**
             * given a master title id we return an owned base game entitlement, return null if no entitlement
             * @return {string}
             * @method getBaseGameEntitlementByMasterTitleId
             */
            getBaseGameEntitlementByMasterTitleId: getBaseGameEntitlementByMasterTitleId,

            /**
             * given a multiplayer id we return an owned base game entitlement, return null if no entitlement
             * @return {string}
             * @method getBaseGameEntitlementByMultiplayerId
             */
            getBaseGameEntitlementByMultiplayerId: getBaseGameEntitlementByMultiplayerId,

            /**
             * given an offerId, returns the entitlement object, otherwise it returns NULL
             * @return {string}
             * @method getEntitlement
             */
            getEntitlement: getEntitlement,

            /**
             * force retrieve the extra content entitlements for a given master titleid
             * @return {promise<EntitlementObject[]>}
             * @method refreshExtraContentEntitlement
             */

            refreshExtraContentEntitlementForMasterTitleId: refreshExtraContentEntitlementForMasterTitleId,

            /**
             * force retrieve from server ALL the extra content entitlements associated with each offer in the user's entitlement list
             * assumes base game entitlements and associated catalog info have already been retrieved
             * @return {promise<string[]>}
             * @method refreshExtraContentEntitlements
             */
            refreshExtraContentEntitlements: refreshExtraContentEntitlements,

            /**
             * returns all OWNED extra content entitlements indexed by masterTitleId
             * @return {object} a map of owned entitlements indexed by masterTitleId
             * @method extraContentEntitlements
             */
            extraContentEntitlements: function() {
                return bestMatchExtraContentEntitlements;
            },

            /**
             * given a masterTitleId, returns all OWNED extra content entitlements
             * @param {string} masterTitleId
             * @return {string[]} a list of owned entitlements
             * @method getExtraContentEntitlementsByMasterTitleId
             */
            getExtraContentEntitlementsByMasterTitleId: function(masterTitleId) {
                return bestMatchExtraContentEntitlements[masterTitleId];
            },

            /**
             * applies suppression to all entitlements and builds the "best match" lists
             * @param {object} catalog data
             * @return {Promise} promise which resolves to an object with lists of newly suppressed/unsuppressed content
             * @method processSuppressions
             */
            processSuppressions: processSuppressions,

            /**
             * updates/adds to the existing entitlement models for base games
             * @param {object} response array of entitlements
             * @method
             */
            processEntitlementsFromResponseBaseGames: processEntitlementsFromResponseBaseGames,
            /**
             * updates/adds to the existing entitlement models for extra content
             * @param {object} response array of entitlements
             * @method
             */
            processEntitlementsFromResponseExtraContent: processEntitlementsFromResponseExtraContent,
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
            ownsEntitlement: ownsEntitlement
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesEntitlementFactorySingleton(DialogFactory, ComponentsLogFactory, SingletonRegistryFactory) {
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
