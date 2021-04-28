/**
 * @file game/gamesdata.js
 */
(function() {
    'use strict';

    function GamesDataFactory(SettingsFactory, DialogFactory, GamesDirtybitsFactory, SubscriptionFactory, GamesPricingFactory, GamesClientFactory, GamesCatalogFactory, GamesEntitlementFactory, GamesOcdFactory, GamesNonOriginFactory, ComponentsLogFactory, ObjectHelperFactory, AuthFactory, AppCommFactory, PdpUrlFactory) {
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
            filterLists = {};

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
                retrieveEntitlements(); //this will wait for critical catalog to load first
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

        function processSuppressions() {
            return GamesEntitlementFactory.processSuppressions(GamesCatalogFactory.getCatalog());
        }

        function sendBaseGamesUpdateEvent() {
            myEvents.fire('games:baseGameEntitlementsUpdate', GamesEntitlementFactory.baseGameEntitlements());
            return Promise.resolve();
        }

        function sendAllExtraContentUpdateEvent() {
            myEvents.fire('games:extraContentEntitlementsUpdate', GamesEntitlementFactory.extraContentEntitlements());
        }

        function sendExtraContentUpdateEvent(masterTitleId) {
            var payload = [];

            //if we pass in a master title id only return the extra content entitlements for that master title id
            if (masterTitleId) {
                payload = GamesEntitlementFactory.extraContentEntitlements()[masterTitleId];
                //payload could be undefined if you do not/no longer own any extra content for that master title so just return empty array
                if (!payload) {
                    payload = [];
                }

                myEvents.fire('games:extraContentEntitlementsUpdate:' + masterTitleId, payload);                  
            } 

        }

        function retrieveEntitlements() {
            function refreshExtraContentEntitlements() {
                return GamesEntitlementFactory.refreshExtraContentEntitlements(GamesCatalogFactory.getCatalog());
            }

            function getAllExtraContentCatalogInfo() {
                var bestBaseGamesPerMasterTitle = {},
                    catalogUpdatePromises = [],
                    baseGameEntitlementList = GamesEntitlementFactory.baseGameEntitlements(),
                    catalog = GamesCatalogFactory.getCatalog(),
                    i;

                for (i = 0; i < baseGameEntitlementList.length; i++) {
                    var ent = baseGameEntitlementList[i],
                        catalogInfo = catalog[ent.offerId];

                    if (catalogInfo &&
                        catalogInfo.extraContent.length > 0 &&
                        (!bestBaseGamesPerMasterTitle.hasOwnProperty(catalogInfo.masterTitleId) ||
                        bestBaseGamesPerMasterTitle[catalogInfo.masterTitleId].originPermissions < ent.originPermissions)) {
                        // If we already have a base game for this masterTitleId, select the base game based on originPermissions.
                        // Base games with the same masterTitleIds share DLC, so we want to retrieve the catalog data using the
                        // base game with the higher permissions in case we have to go to /private.
                        bestBaseGamesPerMasterTitle[catalogInfo.masterTitleId] = ent;
                    }
                }

                for (var masterTitleId in bestBaseGamesPerMasterTitle) {
                    catalogUpdatePromises.push(retrieveExtraContentCatalogInfo(bestBaseGamesPerMasterTitle[masterTitleId].offerId));
                }

                return Promise.all(catalogUpdatePromises);
            }

            function handleRefreshError(error) {
                ComponentsLogFactory.log('refreshEntitlements: error', error);
                myEvents.fire('games:baseGameEntitlementsError', []);
            }

            //make sure we have the critical catalog info
            waitForCriticalCatalogInfo()
                .then(GamesEntitlementFactory.refreshBaseGameEntitlements)
                .then(GamesCatalogFactory.getCatalogInfo)
                .then(processSuppressions)
                .then(GamesNonOriginFactory.refreshNogs)
                .then(sendBaseGamesUpdateEvent)
                .then(refreshExtraContentEntitlements)
                .then(getAllExtraContentCatalogInfo)
                .then(processSuppressions)
                .then(sendAllExtraContentUpdateEvent)
                .catch(handleRefreshError);

            //comment out the promise chain above, and uncomment the promise chain below if supercat doesn't contain the catalog attribute you need (see GamesCatalogFactory.setInfoFromCriticalCatalog for attributes that are retrieved)
            //disabling supercat will slow down the load so we do need to get the attribute added to supercat but for your testing purposes, just have it load each catalog entry separately
            /*
            GamesEntitlementFactory.refreshBaseGameEntitlements()
                .then(GamesCatalogFactory.getCatalogInfo)
                .then(processSuppressions)
                .then(GamesNonOriginFactory.refreshNogs)
                .then(sendBaseGamesUpdateEvent)
                .then(refreshExtraContentEntitlements)
                .then(getAllExtraContentCatalogInfo)
                .then(processSuppressions)
                .then(sendAllExtraContentUpdateEvent)
                .catch(handleRefreshError);
                */
        }

        function clearEntitlements() {
            GamesEntitlementFactory.clearEntitlements();

            //signal that we've modified the entitlements
            sendBaseGamesUpdateEvent();
            ComponentsLogFactory.log('GAMESDATAFACTORY: fired event games:baseGameEntitlementsUpdate from clearEntitlements');
            sendAllExtraContentUpdateEvent();
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
            ComponentsLogFactory.error('********* GamesDataFactory.getCatalogInfo: error ***********');
            ComponentsLogFactory.error(error);
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
            if (isNonOriginGame(parentOfferId)) {
                return Promise.resolve({});
            } else {
                return GamesCatalogFactory.retrieveExtraContentCatalogInfo(parentOfferId, GamesEntitlementFactory.baseEntitlementOfferIdList(), filter);
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

        function initialRefreshCompleted() {
            return GamesEntitlementFactory.initialRefreshCompleted() && GamesNonOriginFactory.initialRefreshCompleted();
        }

        function fireSubFactoryEvent(evt) {
            myEvents.fire('games:' + evt.signal, evt.eventObj);
        }


        function handleCriticalCatalogError(error) {
            ComponentsLogFactory.error(error.message);
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

        //********************************************************************
        //stubbed  functions
        //********************************************************************
        function buyNow(offerId) {
            PdpUrlFactory.goToPdp({
                offerId: offerId
            });
        }

        // Stub for "Get it with Origin"
        function getItWithOrigin(offerId) {
            ComponentsLogFactory.log('GAMESDATAFACTORY: stubbed getItWithOrigin - ', offerId);
            DialogFactory.openAlert({
                id: 'web-get-it-with-origin',
                title: 'Launching Origin',
                description: 'When the rest of OriginX is fully functional, this will launch the client and do something magical.',
                rejectText: 'OK'
            });
        }

        function hasPreload(offerId) {
            // TODO: using local time, not server time for stub, should be fixed with proper trusted time
            var catalogInfo = null,
                downloadStartDate, releaseDate,
                bPreload = false,
                currentDate = Date.now();

            //assume that catlog is loaded, otherwise
            if (offerId) {
                catalogInfo = GamesCatalogFactory.getExistingCatalogInfo(offerId);
                if (catalogInfo) {
                    downloadStartDate = Origin.utils.getProperty(catalogInfo, ['platforms', Origin.utils.os(), 'downloadStartDate']);
                    releaseDate = Origin.utils.getProperty(catalogInfo, ['platforms', Origin.utils.os(), 'releaseDate']);

                    if ((downloadStartDate && releaseDate) && (currentDate < releaseDate)) {
                        bPreload = true;
                    }
                } else {
                    throw new Error('[GamesDataFactory:hasPreload] no catalog info present');
                }
            } else {
                throw new Error('[GamesDataFactory:hasPreload] no offerId');
            }
            return bPreload;
        }

        function availableForPreload(offerId) {
            // TODO: using local time, not server time for stub, should be fixed with proper trusted time
            var currentEpoch = Date.now(),
                catalogInfo = null,
                downloadStartDate, releaseDate,
                bPreload = false;

            //assume that catlog is loaded, otherwise
            if (offerId && hasPreload(offerId)) {
                catalogInfo = GamesCatalogFactory.getExistingCatalogInfo(offerId);
                if (catalogInfo) {
                    downloadStartDate = Origin.utils.getProperty(catalogInfo, ['platforms', Origin.utils.os(), 'downloadStartDate']);
                    releaseDate = Origin.utils.getProperty(catalogInfo, ['platforms', Origin.utils.os(), 'releaseDate']);
                    //hasPreload will checkfor whether preload is available or not, i.e. downloadStartDate < releaseDate
                    //1. make sure we're not already released
                    //2. make sure we're past the downloadStartDate
                    if (downloadStartDate && releaseDate && (currentEpoch < releaseDate) && (currentEpoch >= downloadStartDate)) {
                        bPreload = true;
                    }
                } else {
                    throw new Error('[GamesDataFactory:isAvailableForPreload] no catalog info present');
                }
            } else if (!offerId) {
                throw new Error('[GamesDataFactory:isAvailableForPreload] no offerId');
            }
            return bPreload;
        }

        function isPreorder(offerId) {
            //get the entitlement associated with the offerId
            var ent = getEntitlement(offerId),
                bPreorder = false;

            if (ent && ent.entitlementTag && ent.entitlementTag === 'ORIGIN_PREORDER') {
                bPreorder = true;
            }
            return bPreorder;
        }

        function gameExpired(offerId) {
            return offerId ? true : false;
        }

        function trialActivated(offerId) {
            return offerId ? true : false;
        }

        function newDLCAvailable(offerId, days) {
            return offerId && days ? true : false;
        }

        function newExpansionAvailable(offerId, days) {
            return offerId && days ? true : false;
        }

        function toBeSunset(offerId, days) {
            return offerId && days ? true : false;
        }

        function sunsetted(offerId) {
            return offerId ? true : false;
        }

        function modifyGameProperties(offerId) {
            DialogFactory.openDirective({
                id: 'origin-dialog-content-gameproperties-id',
                name: 'origin-dialog-content-gameproperties',
                data: {
                    offerId: offerId,
                }
            });
        }

        //********************************************************************
        //end stubbed  functions
        //********************************************************************

        function onAuthChanged(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                retrieveEntitlements();
            } else {
                onLoggedOut();
            }
        }


        function onAuthReady(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                retrieveEntitlements();
            }
        }

        function onAuthReadyError(error) {
            ComponentsLogFactory.error('GAMESDATAFACTORY - waitForAuthReady error:', error.message);
        }

        function waitForGamesDataReady() {
            return waitForCriticalCatalogInfo();
        }

        function onCatalogUpdated(evt) {
            processSuppressions();
            fireSubFactoryEvent(evt);
            // Split catalogUpdated event into multiple per-offer events so per-offer directives can more easily parse the data
            for (var offerId in evt.eventObj) {
                myEvents.fire('games:catalogUpdated:' + offerId, evt.eventObj[offerId]);
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

        GamesEntitlementFactory.events.on('GamesEntitlementFactory:isNewUpdate', fireSubFactoryEvent);

        GamesCatalogFactory.events.on('GamesCatalogFactory:criticalCatalogInfoComplete', fireSubFactoryEvent);
        GamesCatalogFactory.events.on('GamesCatalogFactory:catalogUpdated', onCatalogUpdated);
        GamesCatalogFactory.events.on('GamesCatalogFactory:offerStateChange', fireSubFactoryEvent);

        GamesDirtybitsFactory.events.on('GamesDirtybitsFactory:baseGamesUpdateEvent', sendBaseGamesUpdateEvent);
        GamesDirtybitsFactory.events.on('GamesDirtybitsFactory:extraContentUpdateEvent', sendExtraContentUpdateEvent);


        AuthFactory.waitForAuthReady()
            .then(onAuthReady)
            .catch(onAuthReadyError);

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
             * @property {string} gameTypeFacetKey
             * @property {string} numgerofPlayersFacetKey
             * @property {string} publisherFacetKey
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
             */

            /**
             * @typedef vaultObject
             * @type {object}
             * @property {string} pdpPath
             * @property {string} offerId
             * @property {boolean} isUpgradeable
             */

            /**
             * @typedef catalogInfo
             * @type {object}
             * @property {string} offerId
             * @property {string} offerType
             * @property {string} downloadable
             * @property {boolean} trial
             * @property {boolean} limitedTrial
             * @property {boolean} oth
             * @property {boolean} demo
             * @property {string} trialLaunchDuration
             * @property {[string]} extraContent - array of extracontent offerIds
             * @property {string} masterTitleId
             * @property {[string]} alternateMasterTitleIds - array of alternate masterTitleIds
             * @property {[string]} suppressedOfferIds - array of suppressedOffers
             * @property {string} originDisplayType
             * @property {platformsObject} platforms
             * @property {string} ratingSystemIcon - url for icon
             * @property {string} gameRatingUrl
             * @property {boolean} gameRatingPendingMature
             * @property {string} gameRatingDescriptionLong
             * @property {string} gameRatingTypeValue
             * @property {string} gameRatingReason
             * @property {[string]} gameRatingDesc - array of descriptions
             * @property {string} franchiseFacetKey
             * @property {string} platformFacetKey
             * @property {string} publisherFacetKey
             * @property {string} developerFacetKey
             * @property {string} gameTypeFacetKey
             * @property {string} gameEditionFacetKey
             * @property {string} numberofPlayersFacetKey
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
             # @property {boolean} vault
             * @property {string} extraContentType
             * @property {[string]} suppressedOfferIds
             */
            /* NOTE THAT ANY PROPERTY COULD BE NULL */

            /**
             * Get the Catalog information
             * @param {string[]} a list of offerIds
             * @param {string} parentOfferId (optional) - if list is of extra content, provide parentOfferId so we can retrieve private offers if needed
             * @return {Promise} a promise for a list of catalog info
             * @method getCatalogInfo
             */
            getCatalogInfo: getCatalogInfo,

            /**
             * given a parentOfferId, retrieves all extra content associated with that offer
             * @param {string} parentOfferId the offerid to expand
             * @param {string} filter an optional filter choice of 'OWNEDONLY', 'UNOWNEDONLY'
             * @return {Promise<string>}
             * @method retrieveExtraContentCatalogInfo
             */
            retrieveExtraContentCatalogInfo: retrieveExtraContentCatalogInfo,

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
             * force retrieve the entitlements from the server
             * @method refreshEntitlements
             */
            refreshEntitlements: retrieveEntitlements,

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


            //*********************************************************************************************************************************************
            //* OCD DATA RELATED METHODS
            //*********************************************************************************************************************************************

            /*
             * retrieves the OCD record for a given offerId
             * @param {string} offerId
             * @param {string} locale - optional
             * @return {object} ocd response
             * @method getOcdByOfferId
             */
            getOcdByOfferId: function(offerId, locale) {
                return GamesOcdFactory.getOcdByOfferId(offerId, locale);
            },

            /*
             * retrieves the OCD record for a game tree path
             * @param {string} locale
             * @param {string} franchise
             * @param {string} game - optional
             * @param {string} edition - optional
             * @return {object} ocd response
             * @method getOcdByPath
             */
            getOcdByPath: function(locale, franchise, game, edition) {
                return GamesOcdFactory.getOcdByPath(locale, franchise, game, edition);
            },

            /*
             * retrieves and returns the OCD offer info for the specified directive given an offerId
             * @param {string} directive name
             * @param {string} offerId
             * @param {string} locale
             * @return {object} ocd offer info for that directive
             */
            getOcdDirectiveByOfferId: function(directiveName, offerId, locale) {
                return GamesOcdFactory.getOcdDirectiveByOfferId(directiveName, offerId, locale);
            },

            /*
             * retrieves and returns the OCD offer info for the specified directive given a path
             * @param {string} directive name
             * @param {string} locale
             * @param {string} franchise
             * @param {string} game - optional
             * @param {string} edition - optional
             * @return {object} ocd info for that directive given the path
             */
            getOcdDirectiveByPath: function(directiveName, locale, franchise, game, edition) {
                return GamesOcdFactory.getOcdDirectiveByPath(directiveName, locale, franchise, game, edition);
            },

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
             * @return {Promise} a promise for a list of pricing info
             * @method getPrice
             */
            getPrice: function(offerIdList) {
                return GamesPricingFactory.getPrice(offerIdList);
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


            /**
             * Get the price information for a bundle
             * @param {string} bundleId
             * @param {string[]} a list of offerIds in the bundle
             * @return {Promise} a promise for a list of pricing info
             * @method getBundlePrice
             */
            getBundlePrice: function(bundleId, offerIdList) {
                return GamesPricingFactory.getBundlePrice(bundleId, offerIdList);
            },

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
             * [purchaseGame description]
             * @param  {[type]} offerId [description]
             * @return {[type]}         [description]
             */
            purchaseGame: function(offerId) {
                GamesCatalogFactory.getCatalogInfo([offerId]).then(function(response) {
                    DialogFactory.openAlert({
                        id: 'web-gameaction-purchase',
                        title: 'Purchase',
                        description: 'Purchase placeholder for ' + response[offerId].i18n.displayName,
                        rejectText: 'OK'
                    });
                });
            },

            /**
             * [viewOffer description]
             * @param  {[type]} productId [description]
             * @return {[type]}           [description]
             */
            viewOffer: function(offerId) {
                GamesCatalogFactory.getCatalogInfo([offerId]).then(function(response) {
                    DialogFactory.openAlert({
                        id: 'web-gameaction-viewoffer',
                        title: 'View Offer',
                        description: 'Special offer placeholder for ' + response[offerId].i18n.displayName,
                        rejectText: 'OK'
                    });
                });
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
             * [showAchievements description]
             * @param  {[type]} productId [description]
             * @return {[type]}           [description]
             */
            showAchievements: function(offerId) {
                GamesCatalogFactory.getCatalogInfo([offerId]).then(function(response) {
                    DialogFactory.openAlert({
                        id: 'web-gameaction-achievements',
                        title: 'Achievements',
                        description: 'Achievements placeholder for ' + response[offerId].i18n.displayName,
                        rejectText: 'OK'
                    });
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
             * Opens the "game properties" dialog
             * @param {string} offerId
             * @method modifyGameProperties
             */
            modifyGameProperties: modifyGameProperties,

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
             * returns a promise which will resole when Auth initialization has been completed
             * @memberof AuthFactory
             * @method AuthFactory.waitForAuthReady
             */
            waitForGamesDataReady: waitForGamesDataReady,


            //**************************************************************
            //stubbed functions for now
            //**************************************************************
            buyNow: buyNow,
            getItWithOrigin: getItWithOrigin,

            /**
             * returns true if game is preloadable, i.e. it has a preload period
             * @param {string} offerId
             * @return {boolean}
             * @method hasPreload
             */
             hasPreload: hasPreload,

            /**
             * returns true if game so can be preloaded now
             * @param {string} offerId
             * @return {boolean}
             * @method availableForPreload
             */
            availableForPreload: availableForPreload,

            /**
             * returns true if the entitlement associated with the offer is a
             * pre-order, i.e. must own the entitlement
             * @param {string} offerId
             * @return {boolean}
             * @method isPreorder
             */
            isPreorder: isPreorder,

            /**
             * returns true if the catalog object is a base game
             * @param {object} catalogObject
             * @return {boolean}
             * @method isBaseGame
             */
            isBaseGame: GamesCatalogFactory.isBaseGame,
            /**
             * returns true if the terminationDate < currentDate
             * @param {string} offerId
             * @return {boolean}
             * @method gameExpired
             */
            gameExpired: gameExpired,

            /**
             * returns true trial, and has no termination date
             * @param {string} offerId
             * @return {boolean}
             * @method trialActivated
             */
            trialActivated: trialActivated,

            /**
             * given a base game offerId, returns a list of unowned extra content
             * that’s been released within the specified number of days
             * @param {string} offerId
             * @param {int} days       - considered new if released within “days”
             * @return [] list of new DLC?
             * @method newDLCAvailable
             */
            newDLCAvailable: newDLCAvailable,

            /**
             * given a base game offerId, returns a list of unowned expansions
             * that’s been released within X # of days
             * @param {string} offerId
             * @param {int} days      - considered new if released within “days”
             * @return [] list of new expansions?
             * @method newExpansionAvailable
             */
            newExpansionAvailable: newExpansionAvailable,

            /**
             * given an offerId, returns true/false if the game is to be
             * sunsetted in X days
             * @param {string} offerId
             * @param {int} days
             * @return {Boolean}
             * @method toBeSunset
             */
            toBeSunset: toBeSunset,

            /**
             * given a base game offerId, returns true/false if the game has
             * been sunsetted
             * @param {string} offerId
             * @return {Boolean}
             * @method sunsetted
             */
            sunsetted: sunsetted,

            /**
            * returns true if the entitlement associated with the offer is in need of repair
            * @param {string} offerId
            * @return {boolean}
            * @method repairNeeded
            */
            repairNeeded: GamesClientFactory.repairNeeded,

            /**
            * returns true if there's a download override
            * @param {string} offerId
            * @return {boolean}
            * @method downloadOverride
            */
            downloadOverride: GamesClientFactory.downloadOverride,

            /**
            * returns true if the the entitlement is “new”, owned, and downloadable
             * @param {string} offerId
            * @param {int} days
            * @return {boolean}
            * @method newDLCreadyForInstall
            */
            newDLCreadyForInstall: GamesClientFactory.newDLCreadyforInstall,

            /**
            * returns true if the entitlement is “new”, owned, and installed within days
            * @param {string} offerId
            * @param {int} days
            * @return {boolean}
            * @method newDLCinstalled
            */
            newDLCinstalled: GamesClientFactory.newDLCinstalled,

            /**
            * returns true if the game was patched
            * @param {string} offerId
            * @param {int} days
            * @return {boolean}
            * @method patched
            */
            patched: GamesClientFactory.patched

            //**************************************************************
            //end stubbed functions
            //**************************************************************
        };
    }

    function GamesDataFactorySingleton(SettingsFactory, DialogFactory, GamesDirtybitsFactory, SubscriptionFactory, GamesPricingFactory, GamesClientFactory, GamesCatalogFactory, GamesEntitlementFactory, GamesOcdFactory, GamesNonOriginFactory, ComponentsLogFactory, ObjectHelperFactory, AuthFactory, AppCommFactory, PdpUrlFactory, SingletonRegistryFactory) {
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
