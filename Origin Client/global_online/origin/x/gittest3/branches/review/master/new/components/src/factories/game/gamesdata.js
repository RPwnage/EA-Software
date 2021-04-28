/**
 * @file game/gamesdata.js
 */
(function() {
    'use strict';

    function GamesDataFactory(SettingsFactory, DialogFactory, GamesClientFactory, GamesCatalogFactory, GamesEntitlementFactory, GamesOcdFactory, ComponentsLogFactory) {
        //events triggered from this service:
        //  games:baseGameEntitlementsUpdate
        //  games:criticalCatalogInfoComplete

        var myEvents = new Origin.utils.Communicator(),
            criticalCatalogRequestCompleted = false,
            filterSettingsRetrieved = false,
            filterSettingsRetrievalPromise = null,
            filterLists = {};

        //triggered on jssdk logout
        function onLoggedOut() {
            ComponentsLogFactory.log('GamesDataFactory: onLoggedOut');
            filterLists.favorites = [];
            filterLists.hidden = [];
            filterSettingsRetrieved = false;
            GamesEntitlementFactory.clearEntitlements();
        }

        function onLocaleChanged() {
            //currently in C++ client, we make the user log out when the locale changes, will that happen for web too, or at least force reload of the page?
            //if not, then we'll need to reload all the existing catalog to force an update so the user sees the language change for the game tiles
            GamesEntitlementFactory.clearEntitlements();
        }

        function onClientOnlineStateChanged(online) {
            ComponentsLogFactory.log('GamesDataFactory.onClientOnlineStateChanged:', online);
            if (online) {
                //reset the flag so that those listening will wait for the entitlements to get loaded
                GamesEntitlementFactory.setInitialRefreshFlag(false);
                GamesCatalogFactory.clearCatalog(); //and clear cached catalog info so it will repopulate when we come back online (will load cached version if lmd unchanged)
            }
            ComponentsLogFactory.log('GamesDataFactory.onClientOnlineStateChanged: initialRefreshFlag=', GamesEntitlementFactory.initialRefreshCompleted());
        }

        function waitForCriticalCatalogInfo() {
            return new Promise(function(resolve) {
                if (criticalCatalogRequestCompleted) {
                    resolve();
                } else {
                    myEvents.once('games:criticalCatalogReceived', resolve);
                }
            });
        }

        function retrieveEntitlements() {
            //make sure we have the critical catalog info
            waitForCriticalCatalogInfo().then(GamesEntitlementFactory.refreshEntitlements);
            //comment out the line above, and uncomment the line below if supercat doesn't contain the catalog attribute you need (see GamesCatalogFactory.setInfoFromCriticalCatalog for attributes that are retrieved)
            //disabling supercat will slow down the load so we do need to get the attribute added to supercat but for your testing purposes, just have it load each catalog entry separately
            //GamesEntitlementFactory.refreshEntitlements();
        }

        function entitlementByMasterTitleId(masterTitleId) {
            GamesEntitlementFactory.getEntitlementByMasterTitleId(masterTitleId, GamesCatalogFactory.getCatalog());
        }


        function fireSubFactoryEvent(evt) {
            myEvents.fire('games:' + evt.signal, evt.eventObj);
        }


        function handleCriticalCatalogError(error) {
            ComponentsLogFactory.error(error.message);
        }

        function handleCriticalCatalogResponse() {
            criticalCatalogRequestCompleted = true;
            myEvents.fire('games:criticalCatalogReceived');
        }

        function retrieveCriticalCatalog() {
            var locale = Origin.locale.locale();
            GamesCatalogFactory.retrieveCriticalCatalogInfo(locale)
                .then(handleCriticalCatalogResponse)
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

        //stub functions
        function buyNow(offerId) {
            ComponentsLogFactory.log('GAMESDATAFACTORY: stubbed buyNow - ', offerId);
        }


        //stub functions

        //if the relogin fails, then we need to clear entitlements
        Origin.events.on(Origin.events.AUTH_FAILEDCREDENTIAL, GamesEntitlementFactory.clearEntitlements);

        //on initial login or successful login of the jssdk after error, we want to call the retrieveInitialGameData
        //which makes a rest call to grab the entitlemnent information from the server
        Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, retrieveEntitlements);

        retrieveCriticalCatalog();

        if (Origin.auth.isLoggedIn()) {
            retrieveEntitlements();
        }

        //do some cleanup when we log out
        Origin.events.on(Origin.events.AUTH_LOGGEDOUT, onLoggedOut);

        //this is to signal a dirtybit
        //Origin.events.on('dirtybitsOffer', onDirtyBit);

        //need to listen for locale changes to clear cached catalog
        Origin.events.on(Origin.events.LOCALE_CHANGED, onLocaleChanged);

        //listen for connection change (for embedded)
        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);

        //hook up to listen for events from the subfactories
        GamesClientFactory.events.on('GamesClientFactory:initialClientStateReceived', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:update', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:progressupdate', fireSubFactoryEvent);
        GamesClientFactory.events.on('GamesClientFactory:downloadqueueupdate', fireSubFactoryEvent);

        GamesCatalogFactory.events.on('GamesCatalogFactory:criticalCatalogInfoComplete', fireSubFactoryEvent);

        GamesEntitlementFactory.events.on('GamesEntitlementFactory:baseGameEntitlementsUpdate', fireSubFactoryEvent);
        GamesEntitlementFactory.events.on('GamesEntitlementFactory:baseGameEntitlementsError', fireSubFactoryEvent);

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
             * @property {string} packageType
             * @property {Date} releaseDate
             * @property {Date} downloadStartDate
             * @property {Date} useEndDate
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
             * @typedef catalogInfo
             * @type {object}
             * @property {string} offerId
             * @property {string} offerType
             * @property {url} packArt
             * @property {string} displayName - localized
             * @property {boolean} downloadable
             * @property {boolean} trial
             * @property {boolean} limitedTrial
             * @property {boolean} oth
             * @property {string} trialLaunchDuration
             * @property {[string]} extraContent - array of extracontent offerIds
             * @property {string} masterTitleId
             * @property {boolean} purchasable
             * @property {string} originDisplayType
             * @property {platformsObject} platforms
             * @property {string} extraContentType
             */
            /* NOTE THAT ANY PROPERTY COULD BE NULL */

            /**
             * Get the Catalog information
             * @param {string[]} a list of offerIds
             * @param {string} parentOfferId (optional) - if list is of extra content, provide parentOfferId so we can retrieve private offers if needed
             * @return {Promise} a promise for a list of catalog info
             * @method getCatalogInfo
             */
            getCatalogInfo: function(offerId, parentOfferId) {
                return GamesCatalogFactory.getCatalogInfo(offerId, parentOfferId, GamesEntitlementFactory.baseEntitlementOfferIdList());
            },

            /**
             * given a parentOfferId, retrieves all extra content associated with that offer
             * @param {string} parentOfferId
             * @return {Promise<string>}
             * @method retrieveExtraContentCatalogInfo
             */
            retrieveExtraContentCatalogInfo: function(parentOfferId, filter) {
                return GamesCatalogFactory.retrieveExtraContentCatalogInfo(parentOfferId, GamesEntitlementFactory.baseEntitlementOfferIdList(), filter);
            },

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
             * return the cached unique base game entitlements
             * if unspecified returns full list of entitlements
             * @return {string[]}
             * @method baseGameEntitlements
             */
            baseGameEntitlements: GamesEntitlementFactory.baseGameEntitlements,

            baseEntitlementOfferIdList: GamesEntitlementFactory.baseEntitlementOfferIdList,

            /**
             * given a offerId, returns entitlement associated with that offerId, null if unowned
             * @return {object}
             * @method checkEntitlementByMasterTitleId
             */
            getEntitlement: GamesEntitlementFactory.getEntitlement,

            /**
             * given a master title id we return an owned base game offer, return null if no offer
             * @return {object}
             * @method checkEntitlementByMasterTitleId
             */
            getEntitlementByMasterTitleId: entitlementByMasterTitleId,

            /**
             * force retrieve the base game entitlements from the
             * @return {promise<string[]>}
             * @method refreshEntitlements
             */
            refreshEntitlements: GamesEntitlementFactory.refreshEntitlements,

            /**
             * force retrieve from server ALL the extra content entitlements associated with each offer in the user's entitlement list
             * assumes base game entitlements and associated catalog info have already been retrieved
             * @return {promise<string[]>}
             * @method refreshExtraContentEntitlements
             */
            refreshExtraContentEntitlements: function() {
                return GamesEntitlementFactory.refreshExtraContentEntitlements(GamesCatalogFactory.getCatalog());
            },
            /**
             * given a masterTitleId, returns all OWNED extra content entitlements
             * @param {string} masterTitleId
             * @return {string[]} a list of owned entitlements
             */
            getExtraContentEntitlementByMasterTitleId: GamesEntitlementFactory.getExtraContentEntitlementByMasterTitleId,


            /**
             * returns true if the initial base game entitlement request completed
             * @return {boolean}
             * @method initialRefreshCompleted
             */
            initialRefreshCompleted: GamesEntitlementFactory.initialRefreshCompleted,


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
                return GamesCatalogFactory.getCatalogInfo([offerId], null, GamesEntitlementFactory.baseGameEntitlements())
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
                GamesCatalogFactory.getCatalogInfo([offerId], null, GamesEntitlementFactory.baseGameEntitlements()).then(function(response) {
                    DialogFactory.openAlert({
                        id: 'web-gameaction-purchase',
                        title: 'Purchase',
                        description: 'Purchase placeholder for ' + response[offerId].displayName,
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
                GamesCatalogFactory.getCatalogInfo([offerId], null, GamesEntitlementFactory.baseGameEntitlements()).then(function(response) {
                    DialogFactory.openAlert({
                        id: 'web-gameaction-viewoffer',
                        title: 'View Offer',
                        description: 'Special offer placeholder for ' + response[offerId].displayName,
                        rejectText: 'OK'
                    });
                });
            },

            /**
             * [showGameDetails description]
             * @param  {[type]} productId [description]
             * @return {[type]}           [description]
             */
            showGameDetails: function(offerId) {
                GamesCatalogFactory.getCatalogInfo([offerId], null, GamesEntitlementFactory.baseGameEntitlements()).then(function(response) {
                    DialogFactory.openAlert({
                        id: 'web-gameaction-gamedetails',
                        title: 'Game Details',
                        description: 'Games detail page placeholder for ' + response[offerId].displayName,
                        rejectText: 'OK'
                    });
                });
            },

            /**
             * [showAchievements description]
             * @param  {[type]} productId [description]
             * @return {[type]}           [description]
             */
            showAchievements: function(offerId) {
                GamesCatalogFactory.getCatalogInfo([offerId], null, GamesEntitlementFactory.baseGameEntitlements()).then(function(response) {
                    DialogFactory.openAlert({
                        id: 'web-gameaction-achievements',
                        title: 'Achievements',
                        description: 'Achievements placeholder for ' + response[offerId].displayName,
                        rejectText: 'OK'
                    });
                });
            },

            /**
             * returns true if an offer is a non-Origin game
             * @param {string} offerId
             * @return {boolean}
             * @method nonOriginGame
             */
            nonOriginGame: GamesEntitlementFactory.nonOriginGame,

            /**
             * returns true if an entitlement exists for the offer
             * @param {string} offerId
             * @return {boolean}
             * @method ownsEntitlement
             */
            ownsEntitlement: GamesEntitlementFactory.ownsEntitlement,

            //stub functions for now
            buyNow: buyNow

        };
    }

    function GamesDataFactorySingleton(SettingsFactory, DialogFactory, GamesClientFactory, GamesCatalogFactory, GamesEntitlementFactory, GamesOcdFactory, ComponentsLogFactory, SingletonRegistryFactory) {
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
