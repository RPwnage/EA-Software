/**
 * @file game/gamesdata.js
 */
(function() {
    'use strict';

   function GamesDataFactory(SettingsFactory, DialogFactory, GamesDirtybitsFactory, SubscriptionFactory, GamesPricingFactory, GamesClientFactory, GamesCatalogFactory, GamesEntitlementFactory, GamesOcdFactory, GamesNonOriginFactory, ComponentsLogFactory, ObjectHelperFactory, AuthFactory, AppCommFactory, PdpUrlFactory, GamePropertiesFactory, GamesCommerceFactory, EntitlementStateRefiner, GameRefiner, GamesTrialFactory, FeatureIntroFactory) {
        //events triggered from this service:
        //  games:baseGameEntitlementsUpdate
        //  games:baseGameEntitlementsError
        //  games:extraContentEnitlementsUpdate
        //  games:criticalCatalogInfoComplete
        //  games:catalogUpdated
        //  games:catalogUpdated:<offerId>

        var myEvents = new Origin.utils.Communicator(),
            filterSettingsRetrieved = false,
            filterSettingsRetrievalPromise = null,
            filterLists = {},
            ADD_ON_PREVIEW_OFFERID = 'OFB-EAST:46353';  //if user owns this offer then they are able to see "hidden" extra content

        //triggered on jssdk logout
        function onLoggedOut() {
            ComponentsLogFactory.log('GamesDataFactory: onLoggedOut');
            filterLists.favorites = [];
            filterLists.hidden = [];
            filterSettingsRetrieved = false;
            clearEntitlements();
        }

        function onLocaleChanged() {
            //currently in C++ client, we make the user log out when the locale changes, will that happen for web too, or at least force reload of the page?
            //if not, then we'll need to reload all the existing catalog to force an update so the user sees the language change for the game tiles
            clearEntitlements();
        }

        function onClientOnlineStateChanged(onlineObj) {
            ComponentsLogFactory.log('GamesDataFactory.onClientOnlineStateChanged:', onlineObj);
            if (onlineObj && onlineObj.isOnline) {
                //reset the flag so that those listening will wait for the entitlements to get loaded
                GamesEntitlementFactory.setInitialRefreshFlag(false);
                GamesCatalogFactory.clearCatalog(); //and clear cached catalog info so it will repopulate when we come back online (will load cached version if lmd unchanged)
                GamesCatalogFactory.clearCriticalCatalogRetrieved();
                //now go out and get supercat and entitlements
                retrieveCriticalCatalog();
                refreshEntitlements(true); //this will wait for critical catalog to load first
            }
            ComponentsLogFactory.log('GamesDataFactory.onClientOnlineStateChanged: initialRefreshFlag=', GamesEntitlementFactory.initialRefreshCompleted());
        }


        function waitForCriticalCatalogInfo() {
            return new Promise(function(resolve) {
                if (GamesCatalogFactory.isCriticalCatalogRetrieved()) {
                    resolve();
                } else {
                    myEvents.once('games:criticalCatalogInfoComplete', resolve);
                }
            });

        }

        function waitForInitialRefreshCompleted() {
            return new Promise(function(resolve) {
                if (initialRefreshCompleted()) {
                    resolve();
                } else {
                    myEvents.once('games:baseGameEntitlementsUpdate', resolve);
                }
            });
        }

        function doPostRefreshTasks() {
            return GamesEntitlementFactory.doPostRefreshTasks(GamesCatalogFactory.getCatalog());
        }

        function sendEntitlementUpdateEvents() {
            myEvents.fire('games:baseGameEntitlementsUpdate', GamesEntitlementFactory.baseGameEntitlements());
            myEvents.fire('games:extraContentEntitlementsUpdate', GamesEntitlementFactory.extraContentEntitlements());
        }

        function refreshEntitlements(forceRetrieve) {
            function handleRefreshError(error) {
                ComponentsLogFactory.log('refreshEntitlements: error', error);
                myEvents.fire('games:baseGameEntitlementsError', []);

                //don't know if we want to send perf telemetry but we do want to mark the end
                Origin.performance.endTime('COMPONENTS:gamesdata');
            }

            function sendPerformanceTelemetry() {
                Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTS:gamesdata');
            }

            function retrieveEntitlements() {
                return GamesEntitlementFactory.retrieveEntitlements(forceRetrieve);
            }

            Origin.performance.beginTime('COMPONENTS:gamesdata');

            //make sure we have the critical catalog info
            return waitForCriticalCatalogInfo()
                .then(retrieveEntitlements)
                .then(GamesCatalogFactory.getCatalogInfo)
                .then(doPostRefreshTasks)
                .then(GamesNonOriginFactory.refreshNogs)
                .then(sendEntitlementUpdateEvents)
                .then(sendPerformanceTelemetry)
                .catch(handleRefreshError);

            //comment out the promise chain above, and uncomment the promise chain below if supercat doesn't contain the catalog attribute you need (see GamesCatalogFactory.setInfoFromCriticalCatalog for attributes that are retrieved)
            //disabling supercat will slow down the load so we do need to get the attribute added to supercat but for your testing purposes, just have it load each catalog entry separately
            /*
            return retrieveEntitlements()
                .then(GamesCatalogFactory.getCatalogInfo)
                .then(doPostRefreshTasks)
                .then(GamesNonOriginFactory.refreshNogs)
                .then(sendEntitlementUpdateEvents)
                .then(sendPerformanceTelemetry)
                .catch(handleRefreshError);
                */
        }

        function refreshGiftEntitlements() {
            function handleRefreshError(error) {
                ComponentsLogFactory.log('refreshGiftEntitlements: error', error);
                myEvents.fire('games:baseGameEntitlementsError', []);
            }

            return GamesEntitlementFactory.updateGiftEntitlements()
                .then(GamesCatalogFactory.getCatalogInfo)
                .then(doPostRefreshTasks)
                .then(sendEntitlementUpdateEvents)
                .catch(handleRefreshError);
        }

        function clearEntitlements() {
            GamesEntitlementFactory.clearEntitlements();

            //signal that we've modified the entitlements
            sendEntitlementUpdateEvents();
            ComponentsLogFactory.log('GAMESDATAFACTORY: fired event games:baseGameEntitlementsUpdate from clearEntitlements');
            ComponentsLogFactory.log('GAMESDATAFACTORY: fired event games:extraContentEntitlementsUpdate from clearEntitlements');
        }

        /**
         * Indicates whether or not an offer ID is a non-Origin game offer.
         *
         * @param {string} offerId - The offer ID to check.
         * @returns {boolean} true if the offer is a non-Origin game, otherwise false.
         */
        function isNonOriginGame(offerId) {
            return GamesNonOriginFactory.isNonOriginGame(offerId);
        }

        function handleCatalogError(error) {
            ComponentsLogFactory.error('********* GamesDataFactory.getCatalogInfo: error ***********', error);
        }

        /**
         * Get catalog info for the specified offer IDs.
         * @param {string[]} offerIds - The list of offer IDs.
         * @param {string} parentOfferId - The parent offer ID.
         * @returns {Promise.<Object<string, catalogInfo>>} A promise of catalog data for the specified offerIds.
         */
        function getCatalogInfo(offerIds, parentOfferId) {

            // Query the NOG factory for any NOGs.
            var nogData = GamesNonOriginFactory.getCatalogInfo(offerIds);

            // Determine which offerIds are non-NOGs.
            var isNogOfferId = _.partial(_.includes, _.keys(nogData));
            var nonNogOffers = _.reject(offerIds, isNogOfferId);

            // Asynchronously query the catalog factory for all non-NOG offers.
            var nonNogData = GamesCatalogFactory.getCatalogInfo(nonNogOffers, parentOfferId);

            // Merge all results.
            return nonNogData.then(_.partial(_.merge, nogData)).catch(handleCatalogError);
        }

        function retrieveExtraContentCatalogInfo(parentOfferId, filter) {
            var addOnPreviewAllowed = false,
                is110X = false,
                privateEnts = [];

            if (isNonOriginGame(parentOfferId)) {
                return Promise.resolve({});
            } else {
                //check and see if we own the parent offer and if so, we need to pass in the originPermissions
                privateEnts = GamesEntitlementFactory.getPrivateEntitlements();
                is110X = (GamesEntitlementFactory.ownsEntitlement(parentOfferId) && (privateEnts.indexOf(parentOfferId) >= 0));
                addOnPreviewAllowed = (is110X && GamesEntitlementFactory.ownsEntitlement(ADD_ON_PREVIEW_OFFERID));
                return GamesCatalogFactory.retrieveExtraContentCatalogInfo(parentOfferId, GamesEntitlementFactory.getExtraContentEntitlements(), GamesEntitlementFactory.allSuppressedOffers(), addOnPreviewAllowed, filter);
            }
        }

        function baseGameEntitlements() {
            var nogs = GamesNonOriginFactory.baseGameEntitlements();
            var games = GamesEntitlementFactory.baseGameEntitlements();

            return games.concat(nogs);
        }

        function baseEntitlementOfferIdList() {
            var nogs = GamesNonOriginFactory.baseEntitlementOfferIdList();
            var games = GamesEntitlementFactory.baseEntitlementOfferIdList();

            return games.concat(nogs);
        }

        function getEntitlement(offerId) {
            var factory = (isNonOriginGame(offerId)) ? GamesNonOriginFactory : GamesEntitlementFactory;
            return factory.getEntitlement(offerId);
        }

        function getBaseGameEntitlementByMasterTitleId(masterTitleId) {
            return GamesEntitlementFactory.getBaseGameEntitlementByMasterTitleId(masterTitleId, GamesCatalogFactory.getCatalog());
        }

        function getBaseGameEntitlementByMultiplayerId(multiplayerId) {
            return GamesEntitlementFactory.getBaseGameEntitlementByMultiplayerId(multiplayerId, GamesCatalogFactory.getCatalog());
        }

        function getBaseGameEntitlementByAchievementSetId(achievementSetId) {
            return GamesEntitlementFactory.getBaseGameEntitlementByAchievementSetId(achievementSetId, GamesCatalogFactory.getCatalog());
        }

        function initialRefreshCompleted() {
            return GamesEntitlementFactory.initialRefreshCompleted() && GamesNonOriginFactory.initialRefreshCompleted();
        }

        function fireSubFactoryEvent(evt) {
            if(!_.has(evt, 'signal')) {
                return;
            }

            myEvents.fire(['games', _.get(evt, 'signal')].join(':'), _.get(evt, 'eventObj', {}));
        }

        function handleCriticalCatalogError(error) {
            ComponentsLogFactory.error('[GAMESDATAFACTORY:handleCriticalCatalogError]', error);
        }

        function retrieveCriticalCatalog() {
            var locale = Origin.locale.locale();
            GamesCatalogFactory.retrieveCriticalCatalogInfo(locale)
                .catch(handleCriticalCatalogError);
        }

        function retrieveFavoritesAndHidden() {
            //if we already retrieving filter settings lets just return the promise we have outstanding
            if (!filterSettingsRetrievalPromise) {
                filterSettingsRetrievalPromise = Promise.all([
                    SettingsFactory.getFavoriteGames(),
                    SettingsFactory.getHiddenGames()
                ]).then(function(response) {
                    filterSettingsRetrievalPromise = null;
                    return response;
                }).catch(function(error) {
                    filterSettingsRetrievalPromise = null;
                    Promise.reject(error.message);
                });
            }
            return filterSettingsRetrievalPromise;
        }

        function waitForFiltersReady() {
            if (filterSettingsRetrieved) {
                return Promise.resolve();
            } else {
                return retrieveFavoritesAndHidden()
                    .then(function(results) {
                        filterLists.favorites = results[0];
                        filterLists.hidden = results[1];
                        filterSettingsRetrieved = true;
                    });
            }
        }

        function isInFilteredList(listName, offerId) {
            var list = filterLists[listName];
            if (list) {
                return (list.indexOf(offerId) > -1);
            } else {
                return false;
            }
        }

        function addToFilteredList(listName, settingsFn, offerId) {
            waitForFiltersReady().then(function() {
                var list = filterLists[listName];

                if (list.indexOf(offerId) < 0) {
                    list.push(offerId);
                    settingsFn(list);
                    myEvents.fire('filterChanged');

                    if (listName === 'hidden') {
                        FeatureIntroFactory.updateHiddenGameSessionInfo();
                        myEvents.fire('game:hidden:' + offerId);
                    }
                }
            });

        }

        function removeFromFilteredList(listName, settingsFn, offerId) {
            waitForFiltersReady().then(function() {
                var list = filterLists[listName],
                    index = list.indexOf(offerId);

                if (index > -1) {
                    list.splice(index, 1);
                    settingsFn(list);
                    myEvents.fire('filterChanged');

                    if (listName === 'hidden') {
                        myEvents.fire('game:unhidden:' + offerId);
                    }
                }
            });

        }

        function createGameUsageObj(offerId) {
            return function(response) {
                var gameUsageObj = {
                    offerId: offerId
                };

                if (typeof response.lastSessionEndTimeStamp !== 'undefined') {
                    gameUsageObj.lastPlayedTime = Number(response.lastSessionEndTimeStamp);
                } else {
                    ComponentsLogFactory.warn('[GamesData:getGameUsageInfo] - missing lastSessionEndTimeStamp, defaulting to 0');
                    gameUsageObj.lastPlayedTime = 0;
                }

                if (typeof response.total !== 'undefined') {
                    gameUsageObj.totalTimePlayedSeconds = Number(response.total);
                } else {
                    ComponentsLogFactory.warn('[GamesData:getGameUsageInfo] - missing total time played, defaulting to 0');
                    gameUsageObj.totalTimePlayedSeconds = 0;
                }

                return gameUsageObj;
            };
        }

        function requestGameUsageInfo(offerId, platform) {
            return function(catalogInfoList) {
                var catalogInfo = catalogInfoList[offerId];
                if (catalogInfo && (typeof catalogInfo.masterTitleId !== 'undefined')) {
                    if (!platform) {
                        platform = 'PCWIN';
                    }
                    var multiPlayerId = Origin.utils.getProperty(catalogInfo, ['platforms', platform, 'multiPlayerId']);
                    return Origin.atom.atomGameUsage(catalogInfo.masterTitleId, multiPlayerId);
                } else {
                    return Promise.reject(new Error('[GAMESDATAFACTORY:requestGameUsageInfo] no catalog info for:' + catalogInfo.productId));
                }
            };
        }

        function isUserVaultGame(offerId) {
            var cat;

            //first check and see if the user is a subscriber
            if (!SubscriptionFactory.userHasSubscription()) {
                return false;
            }

            //assume offer is loaded, don't go out and retrieve
            cat = GamesCatalogFactory.getCatalog();
            if (!cat.hasOwnProperty(offerId)) {
                return false;
            }

            if (!cat[offerId].vault) {
                return false;
            }

            //here if the catalog item is a vault item, now check and see if it's in user's vault
            return SubscriptionFactory.inUsersVault(offerId);
        }

        function isTrial(offerId) {
            var cat = GamesCatalogFactory.getCatalog();
            if (!cat && !cat.hasOwnProperty(offerId)) {
                return false;
            }

            return GameRefiner.isTrial(cat[offerId]);
        }

        function isAlphaBetaDemo(offerId) {
            var cat = GamesCatalogFactory.getCatalog();
            if (!cat && !cat.hasOwnProperty(offerId)) {
                return false;
            }
            return GameRefiner.isAlphaBetaDemo(cat[offerId]);
        }

        function isEarlyAccess(offerId) {
            var cat = GamesCatalogFactory.getCatalog();
            if (!cat && !cat.hasOwnProperty(offerId)) {
                return false;
            }

            return GameRefiner.isEarlyAccess(cat[offerId]);
        }

        function isConsumable(offerId) {
            var entitlementInfo = GamesEntitlementFactory.getEntitlement(offerId);
            return _.get(entitlementInfo, 'isConsumable', false);
        }

        function isMissingBasegameVaultEditionOrDLC(offerId) {
            if (SubscriptionFactory.userHasSubscription()) {
                return Promise.resolve(checkMissing(GamesCatalogFactory.getCatalog(), offerId));
            }
            else {
                return Promise.resolve(false);
            }
        }

        function checkMissing(catalogInfos, offerId) {
            var missing = false,
                masterTitleId,
                rank,
                vaultGame,
                catalogInfo = catalogInfos[offerId];

            if (catalogInfo) {
                masterTitleId = catalogInfo.masterTitleId;
                rank = catalogInfo.gameEditionTypeFacetKeyRankDesc;
                vaultGame = SubscriptionFactory.getVaultGameFromMasterTitleId(masterTitleId);
                missing = checkMissingFromVault(catalogInfos, rank, vaultGame);
            }

            return missing;

        }

        function checkOwnedEntitlementsForItemId(catalogInfos, itemId) {
            var hasItemId = false,
                entitlements = GamesEntitlementFactory.getAllEntitlements();

            for (var offerId in entitlements) {
                var catalogInfo = catalogInfos[offerId];
                if (catalogInfo && catalogInfo.itemId === itemId) {
                    hasItemId = true;
                    break;
                }
            }

            return hasItemId;
        }

        function checkMissingFromVault(catalogInfos, rank, vaultGame) {
            var i,
                baseGamesArray,
                extraContentArray,
                itemId,
                found,
                missingBaseGame = false,
                missingDLC = false;

            if(rank && vaultGame) {
                // Check whether the user owns the subscription base game
                // If the game we want to upgrade to is of lower rank than the game we have --> don't upgrade.
                if (rank < vaultGame.gameEditionTypeFacetKeyRankDesc) {
                    baseGamesArray = Origin.utils.getProperty(vaultGame, ['basegames', 'basegame']);
                    if(baseGamesArray) {
                        for (i = 0; i < baseGamesArray.length; i++) {
                            found = false;
                            if (GamesEntitlementFactory.ownsEntitlement(baseGamesArray[i])) {
                                found = true;
                            }

                            if (!found) {
                                // Check whether there are entitlements for the same itemId
                                itemId = SubscriptionFactory.getItemIdFromOfferId(baseGamesArray[i]);
                                found = itemId ? checkOwnedEntitlementsForItemId(catalogInfos, itemId) : false;
                            }

                            if (!found) {
                                missingBaseGame = true;
                                break;
                            }
                        }
                    }
                }

                if (!missingBaseGame) {
                    // Check whether the user has all the DLC/ULC.
                    extraContentArray = Origin.utils.getProperty(vaultGame, ['extracontents', 'extracontent']);
                    if (extraContentArray) {
                        for (i = 0; i < extraContentArray.length; i++) {
                            found = false;
                            if (GamesEntitlementFactory.ownsEntitlement(extraContentArray[i])) {
                                found = true;
                            }

                            if (!found) {
                                // Check whether there are entitlements for the same itemId
                                itemId = SubscriptionFactory.getItemIdFromOfferId(extraContentArray[i]);
                                found = itemId ? checkOwnedEntitlementsForItemId(catalogInfos, itemId) : false;
                            }

                            if (!found) {
                                missingDLC = true;
                                break;
                            }
                        }
                    }
                }
            }

            return missingBaseGame || missingDLC;
        }

        function isAvailableFromSubscription(offerId) {
            return getCatalogInfo([offerId])
                .then(_.partial(checkAvailable, _, offerId))
                .catch(Promise.resolve(false));
        }

        function checkAvailable(catalogInfos, offerId) {
            var available = false,
                masterTitleId,
                itemId,
                vaultGame,
                catalogInfo = catalogInfos[offerId];

            if (catalogInfo) {
                masterTitleId = catalogInfo.masterTitleId;
                itemId = catalogInfo.itemId;
                vaultGame = SubscriptionFactory.getVaultGameFromMasterTitleId(masterTitleId);
                available = checkAvailableFromVault(offerId, itemId, vaultGame);
            }

            return available;

        }

        function checkAvailableFromVault(offerId, itemId, vaultGame) {
            var i,
                available = false,
                baseGamesArray,
                extraContentArray,
                itemIds;

            // Check whether the offer is in the subscription.
            if (vaultGame) {
                // Check base games
                baseGamesArray = Origin.utils.getProperty(vaultGame, ['basegames', 'basegame']);
                if (baseGamesArray) {
                    for (i = 0; i < baseGamesArray.length; i++) {
                        if (baseGamesArray[i] === offerId) {
                            available = true;
                            break;
                        }
                    }
                }

                // Check DLCs
                if (!available) {
                    extraContentArray = Origin.utils.getProperty(vaultGame, ['extracontents', 'extracontent']);
                    if (extraContentArray) {
                        for (i = 0; i < extraContentArray.length; i++) {
                            if (extraContentArray[i] === offerId) {
                                available = true;
                                break;
                            }
                        }
                    }
                }
            }

            // If the offerId is not in the vault, maybe there is a similar offer in the vault that gives the same stuff.
            if (!available) {
                if (itemId) {
                    itemIds = SubscriptionFactory.getItemIds();
                    for (i = 0; i < itemIds.length; i++) {
                        if (itemIds[i] === itemId) {
                            available = true;
                            break;
                        }
                    }
                }
            }

            return available;
        }

        /**
         * Directly entitle offer to user
         * @param  {string} offerId Offer ID
         * @return {Promise}
         */
        function directEntitle(offerId) {
            return GamesEntitlementFactory.directEntitle(offerId);
        }

        /**
         * Directly entitle vault offer to user
         * @param  {string} offerId Offer ID
         * @return {Promise}
         */
        function vaultEntitle(offerId) {
            return SubscriptionFactory.vaultEntitle(offerId);
        }

        /**
         * Remove vault offer entitlement from user
         * @param  {string} offerId Offer ID
         * @return {Promise}
         */
        function vaultRemove(offerId) {
            var model = GamesCatalogFactory.getExistingCatalogInfo(offerId),
                vaultGame = SubscriptionFactory.getVaultGameFromMasterTitleId(model.masterTitleId),
                subscriptionId = _.get(GamesEntitlementFactory.getEntitlement(offerId), ['externalId']);

            return SubscriptionFactory.vaultRemove(subscriptionId, vaultGame.offerId);
        }

        function getPath(offerId) {
            return function(catalogdata) {
                return catalogdata[offerId].offerPath;
            };
        }

        function retrieveOcdByPath(locale) {
            return function(path) {
                return GamesOcdFactory.getOcdByPath(path, locale);
            };
        }

        function handleOcdByOfferIdError(error){
            ComponentsLogFactory.log('handleOcdByOfferIdError: error', error);
        }

        // convert offerId to path and request ocd by path
        function getOcdByOfferId(offerId, locale) {
            return getCatalogInfo([offerId])
                .then(getPath(offerId))
                .then(retrieveOcdByPath(locale))
                .catch(handleOcdByOfferIdError);
        }


        function getContentId(offerId) {
            return getCatalogInfo([offerId])
                .then(function(data){
                    var offer = data[offerId] || {};
                    return offer.contentId || null;
                });
        }

        function buyNow(offerId) {
            getCatalogInfo([offerId])
                .then(function(data) {
                    PdpUrlFactory.goToPdp(data[offerId]);
                });
        }

        function getOdcProfile(offerId) {
            return GamesCommerceFactory.getOdcProfile(offerId);
        }

        function onAuthChanged(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                refreshEntitlements();
            } else {
                onLoggedOut();
            }
        }

        function onAuthReady(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                refreshEntitlements();
            }
        }

        function onAuthReadyError(error) {
            ComponentsLogFactory.error('GAMESDATAFACTORY - waitForAuthReady error:', error);
        }

        function waitForGamesDataReady() {
            return waitForCriticalCatalogInfo();
        }

        function onCatalogUpdated(evt) {
            doPostRefreshTasks();
            fireSubFactoryEvent(evt);
            // Split catalogUpdated event into multiple per-offer events so per-offer directives can more easily parse the data
            for (var offerId in evt.eventObj) {
                myEvents.fire('games:catalogUpdated:' + offerId, evt.eventObj[offerId]);
            }
        }

        function gameExpired(offerId) {
            if (!offerId) {
                throw new Error('[GamesDataFactory.gameExpired] Invalid OfferId');
            }
            var entitlementInfo = GamesEntitlementFactory.getEntitlement(offerId);
            if (!entitlementInfo) {
                throw new Error('[GamesDataFactory.gameExpired] No entitlement for Offer');
            }

            return EntitlementStateRefiner.isGameExpired(entitlementInfo);
        }

        function lookUpOfferIdIfNeeded(offerPath, offerId) {
            //if there is no offerId or path just resolve with an undefined
            if(!offerPath && !offerId) {
                return Promise.resolve();
            }
            return (!offerId && offerPath) ? GamesCatalogFactory.getOfferIdByPath(offerPath) : Promise.resolve(offerId);
        }

          /**
           * see if project ids match between recommended item and the offer information
           * @param  {object recoItem the recommendation object
           * @param  {object} offer a catalog offer object
           * @return {boolean} true if theres a match
           */
          function isSameProject(projectId, offer) {

              return offer.projectNumber === projectId;
          }

          /**
           * given an object of objects return first item
           * @param  {object} item object of objects
           * @return {object} return first object
           */
          function getFirstItem(item) {
              return _.first(_.values(item));
          }

          /**
           * given an offer tell if its unowned and purchasable
           * @param  {object} offer the offer object
           * @return {boolean}       true if it meets requirements, false otherwise
           */
          function filterUnownedAndPurchasable(offer) {
              return !GamesEntitlementFactory.ownsEntitlement(offer.offerId) && GameRefiner.isPurchasable(offer);
          }


        /**
         * given a list of possible offers find the best purchasable one
         * @param {string} projectId the project id we want to match against
         * @param  {object[]} possibleOffers array of offers under a single project id
         * @return {promise} promise that resolves with the best offer
         */
        function getPurchasableOfferByProjectId(projectId, possibleOffers) {
            var possibleOffersForProject = _.filter(possibleOffers, _.partial(isSameProject, projectId)),
                firstOffer = _.first(_.filter(possibleOffersForProject, filterUnownedAndPurchasable));
            //since these are filtered out by project id, if one in the list is a basegame all of them are.
            //we will check the first item to see if its a base game and if we own any normal variation of it
            if (GameRefiner.isBaseGame(firstOffer)) {
                var masterTitleId = _.get(firstOffer, 'masterTitleId', ''),
                    ownedEntitlement = getBaseGameEntitlementByMasterTitleId(masterTitleId),
                    ownedOffer = _.get(ownedEntitlement, 'offerId', null);

                //if we don't own the offer or the offer is not a normal game lets suggest the best purchasable
                if (!ownedOffer) { // if ownedOffer is valid, it is also a 'normal game'
                    //ask for best purchasable
                    return GamesCatalogFactory.getPurchasableOfferIdByMasterTitleId(masterTitleId, true)
                        .then(ObjectHelperFactory.toArray)
                        .then(getCatalogInfo)
                        .then(getFirstItem);
                } else {
                    return Promise.reject(new Error('getPurchasableOfferByProjectId: cannot find base game by projectid'));
                }

            } else {
                //if its dlc lets just take the first one thats unowned and purchasable
                if (firstOffer) {
                    return Promise.resolve(firstOffer);
                } else {
                    return Promise.reject(new Error('getPurchasableOfferByProjectId: cannot find dlc game by projectid'));
                }
            }
        }

        //on initial login or successful login of the jssdk after error, we want to call the retrieveInitialGameData
        //which makes a rest call to grab the entitlemnent information from the server
        AuthFactory.events.on('myauth:change', onAuthChanged);

        retrieveCriticalCatalog();

        //need to listen for locale changes to clear cached catalog
        Origin.events.on(Origin.events.LOCALE_CHANGED, onLocaleChanged);

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechanged', onClientOnlineStateChanged);

        //hook up to listen for events from the subfactories
        GamesClientFactory.events.on('GamesClientFactory:initialClientStateReceived', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:update', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:progressupdate', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:downloadqueueupdate', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:operationfailedupdate', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:finishedPlaying', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:startedPlaying', fireSubFactoryEvent);
        GamesEntitlementFactory.events.on('GamesEntitlementFactory:isNewUpdate', fireSubFactoryEvent);

        GamesCatalogFactory.events.on('GamesCatalogFactory:criticalCatalogInfoComplete', fireSubFactoryEvent);
        GamesCatalogFactory.events.on('GamesCatalogFactory:catalogUpdated', onCatalogUpdated);
        GamesCatalogFactory.events.on('GamesCatalogFactory:offerStateChange', fireSubFactoryEvent);

        GamesDirtybitsFactory.events.on('GamesDirtybitsFactory:entitlementUpdateEvent', sendEntitlementUpdateEvents);
        GamesDirtybitsFactory.events.on('GamesDirtybitsFactory:catalogUpdated', onCatalogUpdated);


        AuthFactory.waitForAuthReady()
            .then(onAuthReady)
            .catch(onAuthReadyError);

        // check if a user owns another version of a game (not another edition)
        function getOtherOwnedVersion(path, vault) {
            var otherVersions = GamesCatalogFactory.getPath2OfferIdMap(path),
                ownsOtherVersion;

            if(_.size(otherVersions) > 1) {
                _.forEach(otherVersions, function(offerId) {
                    if(GamesEntitlementFactory.ownsEntitlement(offerId) && !vault) {
                        ownsOtherVersion = offerId;
                        return false;
                    }

                    if(GamesEntitlementFactory.isSubscriptionEntitlement(offerId) && vault) {
                        ownsOtherVersion = offerId;
                        return false;
                    }
                });
            }

            return ownsOtherVersion;
        }

        return {

            events: myEvents,

            //*********************************************************************************************************************************************
            //* CATALOG GAME DATA RELATED METHODS
            //*********************************************************************************************************************************************
            //catalog format
            /**
             * @typedef platformObject
             * @type {object}
             * @property {string} platform
             * @property {string} multiPlayerId
             * @property {string} packageType
             * @property {Date} releaseDate
             * @property {Date} downloadStartDate
             * @property {Date} useEndDate
             * @property {string} executePathOverride - null unless it's a url (web game)
             * @property {string} achievementSet
             */

            /**
             * @typedef platformsObject
             * @type {object}
             * not sure how to represent this, but basically, the object looks like this:
             * {
             *   PCWIN: {platformObject},
             *   MAC: {platformObject}
             * }
             * but each one is optional, i.e. the object could be empty, or it could just have PCWIN or just MAC or it could have both
             */

            /**
             * @typedef countriesObject
             * @type {object}
             * @property {booelan} isPurchasable
             * @property {string} inStock
             * @property {string} catalogPrice
             * @property {string} countryCurrency
             * @property {[string]} catalogPriceA - array of prices
             * @property {[string]} countryCurrencyA - array of currencies
             * @property {boolean} hasSubscriberDiscount
             */

            /**
             * @typedef i18nObject
             * @type {object}
             * @property {string} platformFacetKey
             * @property {string} franchiseFacetKey
             * @property {string} publisherFacetKey
             * @property {string} developerFacetKey
             * @property {string} gameTypeFacetKey
             * @property {string} numgerofPlayersFacetKey
             * @property {string} publisherFacetKey
             * @property {string} genreFacetKey
             * @property {string} systemRequirements
             * @property {string} longDescription
             * @property {string} shortDescription
             * @property {string} officialSiteURL
             * @property {string} eulaURL
             * @property {string} gameForumURL
             * @property {string} franchisePageLink
             * @property {string} brand
             * @property {string} displayName
             * @property {string} preAnnouncementDisplayDate
             * @property {string} ratingSystemIcon - url for icon image
             * @property {string} packArtSmall - url for packArt
             * @property {string} packArtMedium - url for packArt
             * @property {string} packArtLarge - url for packArt
             * @property {string} gameManualURL
             * @property {string} extraContentDisplayGroupDisplayName
             */

            /**
             * @typedef vaultObject
             * @type {object}
             * @property {string} offerId
             * @property {boolean} isUpgradeable
             */

            /**
             * @typedef catalogInfo
             * @type {object}
             * @property {string} offerId
             * @property {string} offerType
             * @property {string} offerPath
             * @property {string} contentId
             * @property {string} downloadable
             * @property {string} gameDistributionSubType
             * @property {boolean} trial
             * @property {boolean} limitedTrial
             * @property {boolean} oth
             * @property {boolean} demo
             * @property {boolean} alpha
             * @property {boolean} beta
             * @property {boolean} earlyAccess
             * @property {string} trialLaunchDuration
             * @property {[string]} extraContent - array of extracontent offerIds
             * @property {string} masterTitleId
             * @property {[string]} alternateMasterTitleIds - array of alternate masterTitleIds
             * @property {[string]} suppressedOfferIds - array of suppressedOffers
             * @property {[string]} includeOffers
             * @property {string} originDisplayType
             * @property {platformsObject} platforms
             * @property {string} ratingSystemIcon - url for icon
             * @property {string} gameRatingUrl
             * @property {boolean} gameRatingPendingMature
             * @property {string} gameRatingDescriptionLong
             * @property {string} gameRatingType
             * @property {string} gameRatingTypeValue
             * @property {string} gameRatingReason
             * @property {[string]} gameRatingDesc - array of descriptions
             * @property {string} franchiseFacetKey
             * @property {string} gameNameFacetKey
             * @property {string} platformFacetKey
             * @property {string} publisherFacetKey
             * @property {string} developerFacetKey
             * @property {string} gameTypeFacetKey
             * @property {string} gameEditionTypeFacetKey
             * @property {string} gameEditionType
             * @property {string} numberofPlayersFacetKey
             * @property {string} genreFacetKey
             * @property {string} platformFacetKey
             * @property {string} mdmItemType
             * @property {string} revenueModel
             * @property {string} defaultLocale
             * @property {[string]} softwareLocales - array of locales
             * @property {string} imageServer - domain portion of packArt Url
             * @property {string} masterTitle
             * @property {string} itemName
             * @property {string} itemType
             * @property {string} itemId
             * @property {string} rbuCode
             * @property {string} brand
             * @property {string} storeGroupId
             * @property {boolean} dynamicPricing
             * @property {countriesObject} countries
             * @property {i18nObject} i18n
             * @property {vaultObject} vaultInfo
             * @property {boolean} vault
             * @property {[string]} suppressedOfferIds
             * @property {string} gameEditionTypeFacetKeyRankDesc
             * @property {string} extracontentDisplayGroup
             * @property {string} extraContentDisplayGroupSortAsc
             */
            /* NOTE THAT ANY PROPERTY COULD BE NULL */

            /**
             * Directly entitle offer to user
             * @param  {string} offerId Offer ID
             * @return {Promise}
             * @method directEntitle
             */
            directEntitle: directEntitle,

            /**
             * Directly entitle vault offer to user
             * @param  {string} offerId Offer ID
             * @return {Promise}
             * @method vaultEntitle
             */
            vaultEntitle: vaultEntitle,

            /**
             * Remove vault offer entitlement from user
             * @param  {string} offerId Offer ID
             * @return {Promise}
             * @method vaultRemove
             */
            vaultRemove: vaultRemove,


            /**
             * Get the Catalog information
             * @param {string[]} a list of offerIds
             * @param {string} parentOfferId (optional) - if list is of extra content, provide parentOfferId so we can retrieve private offers if needed
             * @return {Promise} a promise for a list of catalog info
             * @method getCatalogInfo
             */
            getCatalogInfo: getCatalogInfo,

            /**
             * Given a path of franchise/game/edition, get the corresponding catalog information
             * @param {string} offer path (franchise/game/edition)
             * @param {boolean} purchasable - set to false if want to return any offer that matches path, default is true
             * @return {Promise} a promise for the catalog info
             * @method getCatalogByPath
             */
            getCatalogByPath: GamesCatalogFactory.getCatalogByPath,

            /**
             * Given a path of franchise/game/edition, get the corresponding offerId
             * @param {string} offer path (franchise/game/edition)
             * @param {boolean} purchasable - set to false if want any offerId that matches path, default is true
             * @return {Promise} a promise for the offerId
             * @method getOfferIdByPath
             */
            getOfferIdByPath: GamesCatalogFactory.getOfferIdByPath,

            /**
             * Given a masterTitleId, returns a promise for the path to a purchasable offer associated with the masterTitleId
             * @param {string} masterTitleId
             * @param {boolean} baseGameOnly set to true if you only want the path for a base game, default is false
             * @return {Promise} a promise for the path, null if none found
             * @method getPurchasablePathByMasterTitleId
             */
            getPurchasablePathByMasterTitleId: GamesCatalogFactory.getPurchasablePathByMasterTitleId,


            /**
             * Given a masterTitleId, returns a promise for the offer Id to a purchasable offer associated with the masterTitleId
             * @param {string} masterTitleId
             * @param {boolean} baseGameOnly set to true if you only want the path for a base game, default is false
             * @return {Promise} a promise for the offer Id, null if none found
             * @method getPurchasablePathByMasterTitleId
             */
            getPurchasableOfferIdByMasterTitleId: GamesCatalogFactory.getPurchasableOfferIdByMasterTitleId,

            /**
             * given a list of possible offers find the best purchasable one
             * @param {string} projectId the project id we want to match against
             * @param  {object[]} possibleOffers array of offers under a single project id
             * @return {promise} promise that resolves with the best offer
             * @method getPurchasableOfferByProjectId
             */
            getPurchasableOfferByProjectId: getPurchasableOfferByProjectId,
            /**
             * Given a masterTitleId, returns all the base game entitlements associated with the masterTitleId in a sorted order based
             * on distribution subtype and facet
             * @param {string} masterTitleId
             * @return {object[]} an array of entitlements
             * @method getAllBaseGameEntitlementsByMasterTitleId
             */
            getAllBaseGameEntitlementsByMasterTitleId: GamesEntitlementFactory.getAllBaseGameEntitlementsByMasterTitleId,

            /**
             * given a parentOfferId, retrieves all extra content associated with that offer
             * @param {string} parentOfferId the offerid to expand
             * @param {string} filter an optional filter choice of 'OWNEDONLY', 'UNOWNEDONLY'
             * @return {Promise<Object>} an object of extra content catalog objects indexed by offerId
             * @method retrieveExtraContentCatalogInfo
             */
            retrieveExtraContentCatalogInfo: retrieveExtraContentCatalogInfo,

            /**
             * returns the url for the default box art
             * return {string} default box art url
             * @method defaultBoxArtUrl
             */
            defaultBoxArtUrl: GamesCatalogFactory.defaultBoxArtUrl,


            //***********************************************
            //* CLIENT GAME DATA RELATED METHODS
            // ***********************************************

            /**
             * return whether the initial gamestates have been retrieved from the client
             * @return {boolean}
             * @method initialClientStatesReceived
             */
            initialClientStatesReceived: GamesClientFactory.initialStateReceived,

            /**
             * return the client game data object
             * @return {Object}
             * @method getClientGame
             */
            getClientGame: GamesClientFactory.getGame,


            /**
             * Get the client game object
             * @return {Promise}
             * @method getClientGamePromise
             */
            getClientGamePromise: GamesClientFactory.getGamePromise,


            /**
             * return the index for the client game object that is ITO
             * @return {Number}
             * @method getClientGame
             */
            getClientITOGameIndex: GamesClientFactory.getITOGame,


            //*********************************************************************************************************************************************
            //* ENTITLEMENT DATA RELATED METHODS
            //*********************************************************************************************************************************************

            /**
             * returns whether or not this entitlement is private; i.e., whether an 1102/1103 code is applicable.
             * @return {boolean}
             * @method isPrivate
             */
            isPrivate: GamesEntitlementFactory.isPrivate,

            /**
             * return the cached unique base game entitlements
             * if unspecified returns full list of entitlements
             * @return {string[]}
             * @method baseGameEntitlements
             */
            baseGameEntitlements: baseGameEntitlements,

            baseEntitlementOfferIdList: baseEntitlementOfferIdList,

            /**
             * given a offerId, returns entitlement associated with that offerId, null if unowned
             * @return {object}
             * @method getEntitlement
             */
            getEntitlement: getEntitlement,

            /**
             * given a master title id we return an owned base game offer, return null if no offer
             * @return {object}
             * @method getBaseGameEntitlementByMasterTitleId
             */
            getBaseGameEntitlementByMasterTitleId: getBaseGameEntitlementByMasterTitleId,

            /**
             * given a multiplayer id we return an owned base game offer, return null if no offer
             * @return {object}
             * @method getBaseGameEntitlementByMultiplayerId
             */
            getBaseGameEntitlementByMultiplayerId: getBaseGameEntitlementByMultiplayerId,

            /**
             * given an achievement set id we return an owned base game offer, return null if no offer
             * @return {object}
             * @method getBaseGameEntitlementByAchievementSetId
             */
            getBaseGameEntitlementByAchievementSetId: getBaseGameEntitlementByAchievementSetId,

            /**
             * retrieve the entitlements and catalog data from the server
             * @param {boolean} forceRetrieve if true, entitlements will be fetched from server even if cached data exists
             * @method refreshEntitlements
             * @return {Promise} A Promise that will resolve when the entitlements have been returned.
             */
            refreshEntitlements: refreshEntitlements,

            /**
             * refresh entitlements with locally cached and updated gift data
             * @return {Promise} A Promise that will resolve when the entitlements have been returned.
             */
            refreshGiftEntitlements: refreshGiftEntitlements,

            /**
             * given a masterTitleId, returns all OWNED extra content entitlements
             * @param {string} masterTitleId
             * @return {string[]} a list of owned entitlements
             */
            getExtraContentEntitlementsByMasterTitleId: GamesEntitlementFactory.getExtraContentEntitlementsByMasterTitleId,


            /**
             * returns true if the initial base game entitlement request completed
             * @return {boolean}
             * @method initialRefreshCompleted
             */
            initialRefreshCompleted: initialRefreshCompleted,

            /**
             * returns a Promise that resolves when initial refresh has completed
             * @return {Promise}
             * @method waitForInitialRefreshCompleted
             */
            waitForInitialRefreshCompleted: waitForInitialRefreshCompleted,


            //*********************************************************************************************************************************************
            //* OCD DATA RELATED METHODS
            //*********************************************************************************************************************************************

            /**
             * retrieves the raw OCD record for a game tree path
             * @param {string} offerId the EADP offerId to lookup (OFB-EAST:12345)
             * @param {string} locale the locale in ocd format eg. en-gb.gbr
             * @return {Promise.<Object, Error>}
             */
            getOcdByOfferId: getOcdByOfferId,

            /**
             * retrieves the raw OCD record for a game tree path
             * @param {OcdPath} ocdPath the ocdPath attribute eg. /content/web/app/games/battlefield/battlefield-4/standard-edition
             * @param {string} locale the locale in ocd format eg. en-gb.gbr
             * @return {Promise.<Object, Error>}
             */
            getOcdByPath: GamesOcdFactory.getOcdByPath,

            //*********************************************************************************************************************************************
            //* TRIAL RELATED METHODS
            //*********************************************************************************************************************************************

            /**
             * Check if this offer is a trial awaiting activation
             * @param {string} offerId the offerID of product
             * @return {boolean} if the trial is awaiting activation
             * @method isTrialAwaitingActivation
             */
            isTrialAwaitingActivation: GamesTrialFactory.isTrialAwaitingActivation,
            /**
             * Check if this offer is a trial that has started but not completed
             * @param {string} offerId the offerID of product
             * @return {boolean} if the trial is in progress
             * @method isTrialInProgress
             */
            isTrialInProgress: GamesTrialFactory.isTrialInProgress,
            /**
             * Check if this offer is a trial that has expired (no longer available)
             * @param {string} offerId the offerID of product
             * @return {boolean} if the trial is expired
             * @method isTrialExpired
             */
            isTrialExpired: GamesTrialFactory.isTrialExpired,
            /**
             * Check the amount of trial time remaining,
             * returns 0 if there is none or if the trial is not time-based
             * @param {string} offerId the offerID of product
             * @return {int} the trial time remaining (in seconds), or 0 if trial is not time-based
             * @method getTrialTimeRemaining
             */
            getTrialTimeRemaining: GamesTrialFactory.getTrialTimeRemaining,
            /**
             * Check when this trial should end
             * @param {string} offerId the offerID of product
             * @return {Date} The end date, or undefined if no end date
             * @method getTrialEndDate
             */
            getTrialEndDate: GamesTrialFactory.getTrialEndDate,
            /**
             * Check how many seconds the trial should last (in seconds)
             * @param {string} offerId the offerID of product
             * @return {int} the total trial time (in seconds), or 0 if not time-based
             * @method getTrialTotalTime
             */
            getTrialTotalTime: GamesTrialFactory.getTrialTotalTime,

            //*********************************************************************************************************************************************
            //* PRICING RELATEDMETHODS
            //*********************************************************************************************************************************************

            //this format may change once we get the real server response
            /**
             * @typedef ratingObject
             * @type {object}
             * @param {float} finalTotalPrice
             * @param {float} originalTotalPrice
             * @param {float} originalTotalUnitPrice
             * @param {promotionsObject} promotions - could be empty
             * @param {integer} quantity
             * @param {recommendedPromotionsObject} recommendedPromotions - could be empty
             * @param {float} totalDiscountAmount
             * @param {float} totalDiscountRate
             */

            /**
             * @typedef priceObj
             * @type {object}
             * @param {string} offerId
             * @param {string} offerType
             * @param {ratingObject} rating associative array indexed by currency country e.g. rating['USD']
             */

            /**
             * Get the price information
             * @param {string[]} a list of offerIds
             * @param {string} currency the real or virtual currrency code, e.g. 'USD', 'CAD', '_BW' (optional)
             * @return {Promise} a promise for a list of pricing info
             * @method getPrice
             */
            getPrice: function(offerIdList, currency) {
                return GamesPricingFactory.getPrice(offerIdList, currency);
            },

            /**
             * @typedef bundlePromotionsObj
             * @type {object}
             * @param {string} promotionRuleId
             * @param {float} discountAmount
             * @param {float} discountRate
             * @param {string} promotionRuleType
             */

            /**
             * @typedef bundleOfferPriceObj
             * @type {object}
             * @type {string} bundleType
             * equivalent to priceObj with additional fields specified under ratings[].promotionsObj
             */

            /**
             * @typedef bundlePriceObj
             * @type {object}
             * @param {boolean} isComplete
             * @param {[bundleOfferPriceObj]} array of bundleOfferPriceObj
             */

            //*********************************************************************************************************************************************
            //* GENERAL
            //*********************************************************************************************************************************************

            /**
             * returns a promise that resolves when the filters are ready
             * @return {promise}
             * @method waitForFiltersReady
             */
            waitForFiltersReady: waitForFiltersReady,

            /**
             * given an offerId, returns a promise for the associated contentId
             * @param {string} offerId
             * @return {promise}
             * @method getCotentId
             */
            getContentId: getContentId,

            /**
             * Game Played Time Info
             * @typedef gamePlayedTimeInfoObject
             * @type {object}
             * @property {String} gameId masterTitleId
             * @property {String} total
             * @property {String} lastSessions
             * @property {String} lastSessionEndTimeStamp epoch time
             */

            /**
             * Given offerId, retrieve information on usage
             * @return {Promise<string>}
             * @method getGameUsageInfo
             */
            getGameUsageInfo: function(offerId) {
                return getCatalogInfo([offerId], null)
                    .then(requestGameUsageInfo(offerId))
                    .then(createGameUsageObj(offerId));
            },

            /**
             * Given offerId, return if the game is currently downloand
             * @param {string} offerId offerId for the game
             * @return {boolean} true if the game is downloading, false otherwise
             */
            isDownloading: function(offerId) {
                return GamesClientFactory.getGame(offerId).downloading;
            },

            /**
             * Given offerId, return if the game is a favorite or not
             * @param {string} offerId offerId for the game
             * @return {boolean} true if the game is a favorite, false otherwise
             */
            isFavorite: function(offerId) {
                return isInFilteredList('favorites', offerId);
            },
            /**
             * Add a game to favorites list
             * @param {string} offerId offerId for the game
             */
            addFavoriteGame: function(offerId) {
                return addToFilteredList('favorites', SettingsFactory.updateFavoriteGames, offerId);
            },
            /**
             * Remove a game from the favorites list
             * @param {string} offerId offerId for the game
             */
            removeFavoriteGame: function(offerId) {
                return removeFromFilteredList('favorites', SettingsFactory.updateFavoriteGames, offerId);
            },
            /**
             * Given offerId, return if the game is hidden or not
             * @param {string} offerId offerId for the game
             * @return {boolean} true if the game is hidden, false otherwise
             */
            isHidden: function(offerId) {
                return isInFilteredList('hidden', offerId);
            },
            /**
             * Add a game to hidden list
             * @param {string} offerId offerId for the game
             */
            hideGame: function(offerId) {
                return addToFilteredList('hidden', SettingsFactory.updateHiddenGames, offerId);
            },
            /**
             * Remove a game from hidden list
             * @param {string} offerId offerId for the game
             */
            unhideGame: function(offerId) {
                return removeFromFilteredList('hidden', SettingsFactory.updateHiddenGames, offerId);
            },

            /**
             * Given offerId, return if the game is installed
             * @param {string} offerId offerId for the game
             * @return {boolean} true if the game is installed, false otherwise
             */
            isInstalled: function(offerId) {
                return GamesClientFactory.getGame(offerId).installed;
            },

            /**
             * Given offerId, return if the game is installable
             * @param {string} offerId offerId for the game
             * @return {boolean} true if the game is installable, false otherwise
             */
            isInstallable: function (offerId) {
                return GamesClientFactory.getGame(offerId).installable;
            },

            /**
             * Given offerId, ask the Origin client to install the game
             * @param {string} offerId offerId for the game
             */
            install: function(offerId) {
                Origin.client.games.install(offerId);
            },

            /**
             * Given offerId, ask the Origin client to install the parent game
             * @param {string} offerId offerId for the game
             */
            installParent: function(offerId) {
                Origin.client.games.installParent(offerId);
            },

            /**
             * Given offerId, ask the Origin client to uninstall the game
             * @param {string} offerId offerId for the game
             */
            uninstall: function(offerId) {
                if (GamesEntitlementFactory.isSubscriptionEntitlement(offerId)) {

                    DialogFactory.openDirective({
                        id: 'origin-dialog-content-access-uninstall-id',
                        name: 'origin-dialog-content-access-uninstall',
                        size: 'large',
                        data: {
                            offerid: offerId,
                            id: 'origin-dialog-content-access-uninstall-id'
                        }
                    });
                } else {
                    Origin.client.games.uninstall(offerId);
                }
            },

            removeFromLibrary: function(offerId) {
                if (GamesEntitlementFactory.isSubscriptionEntitlement(offerId)) {
                    DialogFactory.openDirective({
                        id: 'origin-dialog-content-access-remove-id',
                        name: 'origin-dialog-content-access-remove',
                        size: 'large',
                        data: {
                            offerid: offerId,
                            id: 'origin-dialog-content-access-remove-id'
                        }
                    });
                }
                else if (isNonOriginGame(offerId)) {
                     Origin.client.games.removeFromLibrary(offerId);
                }
            },

            /**
             * Given offerId, ask the Origin client play the game
             * @param {string} offerId offerId for the game
             */
            play: function(offerId) {
                if (GamesEntitlementFactory.isSubscriptionEntitlement(offerId) && gameExpired(offerId)) {
                    DialogFactory.openDirective({
                        id: 'origin-dialog-content-access-expired-id',
                        name: 'origin-dialog-content-access-expired',
                        size: 'large',
                        data: {
                            offerid: offerId,
                            id: 'origin-dialog-content-access-expired-id'
                        }
                    });
                } else {
                    Origin.client.games.play(offerId);
                }
            },

            /**
             * Given offerId, return if the game is playable
             * @param {string} offerId offerId for the game
             * @return {boolean} true if the game is playable, false otherwise
             */
            isPlayable: function (offerId) {
                return GamesClientFactory.getGame(offerId).playable;
            },

            /**
             * Navigate the SPA to the Game Library / Owned Game Details page.
             * @param  {string} offerId The Offer ID of the game to view.
             */
            showGameDetailsPageForOffer: function(offerId) {
                AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd', {
                    offerid: offerId
                });
            },

            /**
             * returns true if an offer is owned and was entitled via subscriptions
             * @param {string} offerId
             * @return {boolean}
             * @method isSubscriptionEntitlement
             */
            isSubscriptionEntitlement: GamesEntitlementFactory.isSubscriptionEntitlement,

            /**
             * returns true if an offer is a non-Origin game
             * @param {string} offerId
             * @return {boolean}
             * @method nonOriginGame
             */
            isNonOriginGame: isNonOriginGame,

            /**
             * returns true if an entitlement exists for the offer
             * @param {string} offerId
             * @return {boolean}
             * @method ownsEntitlement
             */
            ownsEntitlement: GamesEntitlementFactory.ownsEntitlement,

            /**
             * returns true if an entitlement exists for the offer path
             * @param {string} path
             * @return {boolean}
             * @method ownsEntitlementByPath
             */
            ownsEntitlementByPath: GamesEntitlementFactory.ownsEntitlementByPath,

            /**
             * returns true if an entitlement exists for the offer path
             * @param {string} path
             * @return {boolean}
             * @method ownsEntitlementByPath
             */
            ownsOTHEntitlementByPath: function(ocdPath) {
                //THIS IS TEMP CODE WHILE WE WAIT FOR THE SERVER RESPONSE TO GET UPDATED
                //WHEN ITS UPDATED EVERYTHING WILL RUN THROUGH THE GamesEntitlementFactory.ownsEntitlementByPath
                //CURRENTLY TO UNBLOCK GEO
                var allEntitlements = GamesEntitlementFactory.getAllEntitlements();
                for (var offerId in allEntitlements) {
                    var ent = allEntitlements[offerId];
                    //if the ocd path matches and if the checkOTHEntitlement flag is set see if entitlement was granted via OTH
                    if (ent && ent.offerPath === ocdPath) {

                        //simulates data we'll get back from updated entitlement
                        var obj = {
                            gameDistributionSubType: _.get(GamesCatalogFactory.getExistingCatalogInfo(offerId), 'gameDistributionSubType'),
                            entitlementSource: _.get(ent, 'entitlementSource')
                        };

                        if (EntitlementStateRefiner.isOTHGranted(obj)) {
                            return true;
                        }

                    }
                }
                return false;

            },

            /**
             * is there a download like operation happening
             * @param {string} offerid the offer
             * @return {Boolean} true if a download update or repair is happening, false other wise
             */
            isOperationActive: GamesClientFactory.isOperationActive,

            /**
             * is this game part of user's vault -- not necessarily entitled yet, but is it part of his subscription set
             * @param {string} offerid the offer
             * @return {Boolean} true if game is part of user's vault
             */
            isUserVaultGame: isUserVaultGame,

            /**
             * is this game a trial
             * @param {string} offerid the offer
             * @return {Boolean} true if game is trial
             */
            isTrial: isTrial,

            /**
             * is this game a Alpha Beta or Demo
             * @param {string} offerid the offer
             * @return {Boolean} true if game is alpha beta or demo.
             */
            isAlphaBetaDemo: isAlphaBetaDemo,

            /**
             * is this game a early access
             * @param {string} offerid the offer
             * @return {Boolean} true if game is early access
             */
            isEarlyAccess: isEarlyAccess,

            /**
             * is the entitlement for this offer consumable
             * @param {string} offerid the offer
             * @return {Boolean} true if the user owns the offer and the entitlement is consumable
             */
            isConsumable: isConsumable,

            /**
             * returns a promise which will resolve when critical catalog is is loaded
             * @method waitForGamesDataReady
             */
            waitForGamesDataReady: waitForGamesDataReady,

            /**
            * opens the Game Properties dialog
            * @param {string} offerId
            * @method openGameProperties
            */
            openGameProperties: GamePropertiesFactory.openGameProperties,


            /**
             * Alias for go to pdp function
             * @param  {string} offerId the offer id to resolve
             * @method buyNow
             */
            buyNow: buyNow,

            /**
             * if we don't have an offer id but have ocd path, do the look up, otherwise assume there's an offerid
             * @param  {string} offerPath the offer path for the offer
             * @param  {string} offerId   the default offer id
             * @return {string}           if we pass in an offer id we just return that out other wise call getOfferIdByPath to get an offerid
             */
            lookUpOfferIdIfNeeded: lookUpOfferIdIfNeeded,

            /**
             * Retrieves ODC profile data for the given offer.
             *
             * @param {string} offerId The offer for which to retrieve ODC profile data
             * @return {Promise<odcProfileObject>} Promise that resolves into a ODC profile data model object
             */
            getOdcProfile: getOdcProfile,

            /**
             * given a game model, retrieves the lowest ranked basegames offerId associated with the masterId
             * @param {object} game
             * @return {string} lowest ranked basegames offerId
             */
            getLowestRankedPurchsableBasegame: GamesCatalogFactory.getLowestRankedPurchsableBasegame,

            /**
             * Given an offerId, returns whether the vault edition of the base game or any DLCs are missing.
             * WARNING: This function assumes that the catalog information for entitlements associated to the masterTitleId of the specified offer
             *          (e.g., associated DLCs and other editions) has already been cached.
             *          For example, in components\src\directives\game\scritps\tile.js:enableDelayLoaded(), the catalog info
             *          for extra content associated with the offer’s masterTitleId is retrieved before coming into here via the violator code.
             *
             * @param {string} offerId offerId to check
             * @return {Promise<boolean>}
             * @method isMissingBasegameVaultEditionOrDLC
             */
            isMissingBasegameVaultEditionOrDLC: isMissingBasegameVaultEditionOrDLC,

            /**
             * Given an offerId, returns whether the vault edition of offer is available.
             * @param {string} offerId offerId to check
             * @return {Promise<boolean>}
             * @method isAvailableFromSubscription
             */
            isAvailableFromSubscription: isAvailableFromSubscription,

            /**
             * Given an path, returns offer id for other owned versions
             * @param {string} path ocd path to check
             * @param {boolean} vault check vault ownership
             * @return {mixed}
             * @method getOtherOwnedVersion
             */
            getOtherOwnedVersion: getOtherOwnedVersion
        };
    }

    function GamesDataFactorySingleton(SettingsFactory, DialogFactory, GamesDirtybitsFactory, SubscriptionFactory, GamesPricingFactory, GamesClientFactory, GamesCatalogFactory, GamesEntitlementFactory, GamesOcdFactory, GamesNonOriginFactory, ComponentsLogFactory, ObjectHelperFactory, AuthFactory, AppCommFactory, PdpUrlFactory, GamePropertiesFactory, GamesCommerceFactory, EntitlementStateRefiner, GameRefiner, GamesTrialFactory, FeatureIntroFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesDataFactory', GamesDataFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesDataFactory

     * @description
     *
     * GamesDataFactory
     */
    angular.module('origin-components')
        .factory('GamesDataFactory', GamesDataFactorySingleton);
}());
