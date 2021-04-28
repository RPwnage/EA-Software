/**
 * @file game/gamesdata.js
 */
(function() {
    'use strict';

    function GamesDirtybitsFactory(GamesCatalogFactory, GamesEntitlementFactory, ComponentsLogFactory, ObjectHelperFactory) {


        var myEvents = new Origin.utils.Communicator();

        function handleDirtyBitsUpdateError(error) {
            ComponentsLogFactory.error('[GAMES DIRTYBITS UPDATE]:', error);
        }

        function determineNeededRefreshes(offerIdsArray) {
            return function(catalogInfoObjects) {
                var refreshesNeeded = {
                        baseGameRefresh: false,
                        masterTitleIdsRefresh: {}
                    },
                    i = 0,
                    len = offerIdsArray.length;

                //loop through our offers see if its a basegame and mark approriately
                for (i = 0; i < len; i++) {
                    var catalogObject = catalogInfoObjects[offerIdsArray[i]],
                        masterTitleId = catalogObject.masterTitleId;
                    if (GamesCatalogFactory.isBaseGame(catalogObject)) {
                        refreshesNeeded.baseGameRefresh = true;

                    } else {
                        //we are just keeping track of what masterTitleIds we've encountered and assigning it to null
                        //so we can track it in the object
                        if (catalogObject.masterTitleId) {
                            refreshesNeeded.masterTitleIdsRefresh[masterTitleId] = null;
                        }
                    }

                    return refreshesNeeded;
                }
            };
        }

        function calculateRefreshes(offerId) {
            return GamesCatalogFactory.getCatalogInfo([offerId])
                .then(determineNeededRefreshes([offerId]));
        }

        function assignEntitlementsToCategories(contentUpdateEntitlements) {
            return function(catalogInfoObjects) {
                var categorizedEntitlements = {
                        baseGame: [],
                        masterTitleIds: {}
                    },
                    i = 0,
                    len = contentUpdateEntitlements.length;

                //loop through the content update entitlements to filter it between base games and extra content
                for (i = 0; i < len; i++) {
                    var catalogObject = catalogInfoObjects[contentUpdateEntitlements[i].offerId];
                    if (GamesCatalogFactory.isBaseGame(catalogObject)) {
                        categorizedEntitlements.baseGame.push(contentUpdateEntitlements[i]);
                    } else {
                        if (catalogObject.masterTitleId) {
                            if (!categorizedEntitlements.masterTitleIds[catalogObject.masterTitleId]) {
                                categorizedEntitlements.masterTitleIds[catalogObject.masterTitleId] = [];
                            }
                            categorizedEntitlements.masterTitleIds[catalogObject.masterTitleId].push(contentUpdateEntitlements[i]);
                        }
                    }
                }

                return categorizedEntitlements;
            };
        }

        function categorizeEntitlements(entitlements) {
            var offerIdsArray = entitlements.map(ObjectHelperFactory.getProperty('offerId'));

            return GamesCatalogFactory.getCatalogInfo(offerIdsArray)
                .then(assignEntitlementsToCategories(entitlements));
        }

        function processSupressions() {
            GamesEntitlementFactory.processSuppressions(GamesCatalogFactory.getCatalog());
        }
        function fireDirtyBitsBaseGamesUpdateEvent() {
            myEvents.fire('GamesDirtybitsFactory:baseGamesUpdateEvent');
        }

        function fireExtraContentUpdateEvent(masterTitleId) {
            myEvents.fire('GamesDirtybitsFactory:extraContentUpdateEvent', masterTitleId);
        }

        function processUpdatedEntitlements(categorizedEntitlements) {
            var masterTitleIds = categorizedEntitlements.masterTitleIds;
            if (categorizedEntitlements.baseGame.length) {
                //updates the model for base games with the new data
                GamesEntitlementFactory.processEntitlementsFromResponseBaseGames(categorizedEntitlements.baseGame);
                fireDirtyBitsBaseGamesUpdateEvent();
            }

            for (var m in masterTitleIds) {
                if (masterTitleIds.hasOwnProperty(m)) {
                    if (masterTitleIds[m].length) {
                        //updates the model for extra content with the new data
                        GamesEntitlementFactory.processEntitlementsFromResponseExtraContent(masterTitleIds[m]);
                        fireExtraContentUpdateEvent(m);
                    }
                }
            }

        }

        function deleteEntitlementIfNecessary(performDelete, entitlementId) {
            //if we get a delete from dirty bits lets remove the entitlement first
            return function(offerId) {
                if (performDelete) {
                    GamesEntitlementFactory.removeEntitlement(offerId, entitlementId);
                }
                return offerId;
            };
        }

        function sendNotifications(masterTitleIds) {
            return function() {
                for (var m in masterTitleIds) {
                    if (masterTitleIds.hasOwnProperty(m)) {
                        fireExtraContentUpdateEvent(m);
                    }
                }
            };
        }

        function refreshBaseGameOrExtraContent(categorizedEntitlements) {
            var masterTitleIds = categorizedEntitlements.masterTitleIdsRefresh;

            //if basegame changed we'll refresh the base game entitlements
            if (categorizedEntitlements.baseGameRefresh) {
                return GamesEntitlementFactory.refreshBaseGameEntitlements()
                    .then(GamesCatalogFactory.getCatalogInfo)
                    .then(processSupressions)
                    .then(fireDirtyBitsBaseGamesUpdateEvent);
            }

            //see which master title ids have extra content that has changed
            if (Object.getOwnPropertyNames(masterTitleIds).length !== 0) {
                var extraContentPromises = [];
                for (var m in masterTitleIds) {
                    if (masterTitleIds.hasOwnProperty(m)) {
                        extraContentPromises.push(GamesEntitlementFactory.refreshExtraContentEntitlementForMasterTitleId(m).then(GamesCatalogFactory.getCatalogInfo));
                    }
                }
                return Promise.all(extraContentPromises).then(processSupressions).then(sendNotifications(masterTitleIds));
            }

            return Promise.resolve();

        }


        function needsFullRefresh(data) {
            //if status has changed or if a game has added, it wil have a status field returned in dirtybits data. In those cases we need 
            //a full refresh
            return data.status;
        }

        function findOfferIdFromEntitlement(entId) {
            return function(entitlements) {
                for (var i = 0; i < entitlements.length; i++) {
                    if (entId === entitlements[i].entitlementId) {
                        return entitlements[i].offerId;
                    }
                }
                return Promise.reject(new Error('[DIRTYBITS]Cannot find offerid in entitlements'));

            };
        }

        function determineOfferId(incomingId, entitlementId) {
            var offerId = incomingId;
            //if we weren't passed in a valid offer id, lets try to look one up in our current entitlements
            if (!offerId) {
                var ent = GamesEntitlementFactory.entitlementByEntitlementId(entitlementId);
                if (ent) {
                    offerId = ent.offerId;
                    return Promise.resolve(offerId);
                }
            }

            //we couldn't find one (in the case of going from a deleted to active status), lets try to look one up using contentUpdates
            //pending a talk with ML, we will always have a valid offerId passed with the dirtybits notification, so we can remove a lot of this
            //logic when that happens
            if (!offerId) {
                return Origin.games.contentUpdates()
                    .then(findOfferIdFromEntitlement(entitlementId));

            } else {
                return Promise.resolve(offerId);
            }
        }


        function notifyCatalogUpdate(offers) {
            myEvents.fire('games:catalogUpdated', offers);
            for (var p in offers) {
                if (offers.hasOwnProperty(p)) {
                    myEvents.fire('games:catalogUpdated:' + p, offers[p]);
                }
            }
        }

        function onDirtyBitsEntitlementUpdate(data) {
            //certain property changes like "status" require full refresh which means refresh base games and any relevant master title
            if (needsFullRefresh(data)) {
                determineOfferId(data.offerId, data.entId)
                    .then(deleteEntitlementIfNecessary(data.status === 'DELETED', data.entId))
                    .then(calculateRefreshes)
                    .then(refreshBaseGameOrExtraContent)
                    .catch(handleDirtyBitsUpdateError);
            } else {
                //other wise we jsut get the new entitlement info and update the model records
                Origin.games.contentUpdates()
                    .then(categorizeEntitlements)
                    .then(processUpdatedEntitlements)
                    .catch(handleDirtyBitsUpdateError);
            }
        }


        function onDirtyBitsCatalogUpdate(data) {
            if (data.baseGameOfferIds && data.baseGameOfferIds.length) {
                GamesCatalogFactory.getCatalogInfo(data.baseGameOfferIds, null, true)
                    .then(notifyCatalogUpdate)
                    .catch(handleDirtyBitsUpdateError);
            }

            if (data.masterTitles) {
                var masterTitles = data.masterTitles;

                for (var p in masterTitles) {
                    if (masterTitles.hasOwnProperty(p)) {
                        GamesCatalogFactory.getCatalogInfo(masterTitles[p], GamesEntitlementFactory.getBaseGameEntitlementByMasterTitleId(p, GamesCatalogFactory.getCatalog()), true)
                            .then(notifyCatalogUpdate)
                            .catch(handleDirtyBitsUpdateError);
                    }
                }
            }
        }
        //
        // DIRTY BITS END
        // 

        //listen for dirty bit updates
        Origin.events.on(Origin.events.DIRTYBITS_ENTITLEMENT, onDirtyBitsEntitlementUpdate);
        Origin.events.on(Origin.events.DIRTYBITS_CATALOG, onDirtyBitsCatalogUpdate);

        return {
            events: myEvents
        };
    }

    function GamesDirtybitsFactorySingleton(GamesCatalogFactory, GamesEntitlementFactory, ComponentsLogFactory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesDirtybitsFactory', GamesDirtybitsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesDirtybitsFactory
     
     * @description
     *
     * GamesDirtybitsFactory
     */
    angular.module('origin-components')
        .factory('GamesDirtybitsFactory', GamesDirtybitsFactorySingleton);
}());