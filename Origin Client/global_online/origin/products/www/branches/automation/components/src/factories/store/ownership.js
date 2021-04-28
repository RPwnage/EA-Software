/**
 * @file ownership.js
 */
(function() {
    'use strict';

    function OwnershipFactory(ObjectHelperFactory, FunctionHelperFactory, GamesDataFactory, SubscriptionFactory, GamesTrialFactory, GameRefiner, VaultRefiner, AuthFactory) {
        var pipe = FunctionHelperFactory.pipe,
            orderByField = ObjectHelperFactory.orderByField,
            takeHead = ObjectHelperFactory.takeHead,
            forEachWith = ObjectHelperFactory.forEachWith,
            filterBy = ObjectHelperFactory.filterBy,
            ENTITLEMENT_TIMEOUT_MS = 60000;

        /**
         * Determines whether user owns a particular edition
         * @param edition
         * @returns {isOwned|*}
         */
        function isOwned(edition) {
            return edition.isOwned;
        }

        /**
         * Determines whether subscription is available for a particular edition
         * @param edition
         * @returns {boolean|*}
         */
        function isSubscriptionAvailable(edition) {
            return edition.isSubscription;
        }

        /**
         * Gets the top edition user has purchased
         * @param editions to filter through
         * @returns {*}
         */
        function getTopOwnedEdition(editions) {
            return getTopEditionByFilter(isOwned)(editions);
        }



        function getTopOwnedEditions(editions) {
            return getTopEditionsByFilter(isOwned)(editions);
        }

        function getTopEditionsByFilter(filterField) {
            return pipe([filterBy(filterField), orderByField('weight')]);
        }

        /**
         * Get the top edition user has purchased
         * @param editions
         * @returns {*}
         */
        function getTopSubscriptionEdition(editions) {
            return getTopEditionByFilter(isSubscriptionAvailable)(editions);
        }

        /**
         * Filters editions by given filter and orders by weight
         * @param filterField
         * @returns {Function}
         */
        function getTopEditionByFilter(filterField) {
            return pipe([filterBy(filterField), orderByField('weight'), takeHead]);
        }

        /**
         * Has at least one subscribe-able edition
         * @param editions
         * @returns {*}
         */
        function hasSubscriptionEdition(editions) {
            return getTopSubscriptionEdition(editions);
        }

        /**
         * Whether user owns a better edition
         * @param editions
         * @param model
         * @returns {*|boolean}
         */
        function userOwnsBetterEdition(editions, model) {
            var topOwnedEdition = getTopOwnedEdition(editions);
            return (topOwnedEdition && topOwnedEdition.weight > model.weight);
        }

        /**
         *
         * @param editions
         * @param model
         * @returns {*}
         */
        function getPrimaryEdition(editions, model) {
            if (userOwnsBetterEdition(editions, model)) {
                return getTopOwnedEdition(editions);
            } else if (model.isOwned) {
                return model;
            } else if (hasSubscriptionEdition(editions)) {
                return getTopSubscriptionEdition(editions);
            } else {
                return model;
            }
        }

        /**
         * set currency and anonymous ownership values to false
         * as users cant own currency, so they can re-purchase and
         * anonymous users dont own anything
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function isRepurchasableOrAnonymous(game) {
            // TODO: replace isCurrency and isConsumable with isRepurchasable when available
            if(GameRefiner.isCurrency(game) || !AuthFactory.isAppLoggedIn()) { // || GamesDataFactory.isConsumable(_.get(game, 'offerId'))) {
                game.isOwned = false;
                game.vaultEntitlement = false;

                return Promise.resolve(game);
            }

            return Promise.reject(game);
        }

        /**
         * check a games ownership
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function isParentOwned(game) {
            if(GamesDataFactory.ownsEntitlement(game.offerId)) {
                game.isOwned = GamesDataFactory.ownsEntitlement(game.offerId);                
                game.vaultEntitlement = GamesDataFactory.isSubscriptionEntitlement(game.offerId);
                game.ogdOfferId = game.offerId;

                return GamesTrialFactory
                        .getTrialTimeByOfferId(game.offerId)
                        .then(function(data) {
                            game.trialTimeRemaining = data;

                            return game;
                        });
            }

            return Promise.reject(game);
        }

        // ocdPath or offerPath from catalog data
        function getPath(game) {
            return _.get(game, 'ocdPath', _.get(game, 'offerPath', ''));
        }

        /**
         * check if a user owns another version of a game (not another edition),
         * maybe from another locale, or preorder version
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function isAnotherVersionOfParentOwned(game) {
            var otherOwnedVersion = GamesDataFactory.getOtherOwnedVersion(getPath(game), false);

            if(otherOwnedVersion) {
                game.isOwned = true;
                game.vaultEntitlement = GamesDataFactory.getOtherOwnedVersion(game.ocdPath, true) !== undefined;
                game.ogdOfferId = otherOwnedVersion;

                return GamesTrialFactory
                        .getTrialTimeByOfferId(game.offerId)
                        .then(function(data) {
                            game.trialTimeRemaining = data;

                            return game;
                        });
            }

            return Promise.reject(game);
        }

        /**
         * set a bundles ownership based on child models ownership
         * @param {Object} game model data to augment
         * @param {Object} catalogData catalog data to check
         * @return {Promise}
         */
        function isAllChildrenOwned(game, catalogData) {
            var owned = true,
                vault = true,
                basegame,
                bundle = _.get(game, 'bundleOffers', []);

            _.forEach(bundle, function(offerId) {
                // use the catalog data to check ownership if it exists
                if(catalogData.hasOwnProperty(offerId)) {
                    var offer = _.get(catalogData, offerId, {}),
                        bundledOfferIds = _.get(offer, 'bundleOffers', []),
                        isShellBundle = _.get(offer, 'isShellBundle', false);

                    if(!GamesDataFactory.ownsEntitlement(offerId)) {
                        if(isShellBundle) {
                            _.forEach(bundledOfferIds, function(childOfferId) {
                                if(!GamesDataFactory.ownsEntitlement(childOfferId)) {
                                    owned = false;
                                }
                            });
                        } else {
                            owned = false;
                        }
                    }

                    if(!GamesDataFactory.isSubscriptionEntitlement(offerId)) {
                        if(isShellBundle) {
                            _.forEach(bundledOfferIds, function(childOfferId) {
                                if(!GamesDataFactory.isSubscriptionEntitlement(childOfferId)) {
                                    vault = false;
                                }
                            });
                        } else {
                            vault = false;
                        }
                    }

                    if(GameRefiner.isBaseGame(offer) && owned) {
                        basegame = offerId;
                    }
                // use the offer id to check ownership if the catalog data does not exist
                } else {
                    if(!GamesDataFactory.ownsEntitlement(offerId)) {
                        owned = false;
                    }

                    if(!GamesDataFactory.isSubscriptionEntitlement(offerId)) {
                        vault = false;
                    }
                }
            });

            if(owned) {
                game.isOwned = owned;
                game.vaultEntitlement = vault;
                game.ogdOfferId = basegame;

                return Promise.resolve(game);
            }

            return Promise.reject(game);
        }

        /**
         * check a shell bundles child models ownership
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function isBundleContentOwned(game) {
            if(_.size(game.bundleOffers) > 0 && GameRefiner.isShellBundle(game)) {
                return GamesDataFactory
                    .getCatalogInfo(game.bundleOffers)
                    .then(_.partial(isAllChildrenOwned, game));
            } else {
                return Promise.reject(game);
            }
        }

        /**
         * check vault game model ownership
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function getVaultGameAndCheckOwnership(game, vaultGame) {
            if(GamesDataFactory.ownsEntitlement(vaultGame.offerId)) {
                game.isOwned = GamesDataFactory.ownsEntitlement(vaultGame.offerId);                
                game.vaultEntitlement = GamesDataFactory.isSubscriptionEntitlement(vaultGame.offerId);
                game.ogdOfferId = vaultGame.offerId;

                return Promise.resolve(game);
            }

            return Promise.reject(game);
        }

        /**
         * checks a shell bundle is owned
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function isShellBundleOwned(game) {
            if(VaultRefiner.isShellBundle(game) && Origin.utils.os() === Origin.utils.OS_WINDOWS) {
                return GamesDataFactory
                        .getCatalogByPath(_.get(game, 'vaultOcdPath'), !VaultRefiner.isShellBundle(game))
                        .then(_.partial(getVaultGameAndCheckOwnership, game));
            } else {
                return Promise.reject(game);
            }
        }

        /**
         * get vault game childrens models and check ownership
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function getVaultGameAndCheckChildrenOwnership(game, vaultGame) {
            return GamesDataFactory
                .getCatalogInfo(_.get(_.first(_.values(vaultGame)), 'bundleOffers'))
                .then(_.partial(isAllChildrenOwned, game));
        }

        /**
         * checks if all content in a shell bundle is owned
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function isShellBundleContentOwned(game) {
            if(VaultRefiner.isShellBundle(game) && Origin.utils.os() === Origin.utils.OS_WINDOWS) {
                return GamesDataFactory
                        .getCatalogByPath(_.get(game, 'vaultOcdPath'), !VaultRefiner.isShellBundle(game))
                        .then(_.partial(getVaultGameAndCheckChildrenOwnership, game));
            } else {
                return Promise.reject(game);
            }
        }

        /**
         * set isOwned and vaultEntitlment to false (unowned)
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function setIsNotOwned(game) {
            game.isOwned = false;
            game.vaultEntitlement = false;

            return Promise.resolve(game);
        }

        /**
         * set isInstalled and IsSubscription
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function isInstalledOrSubscription(game) {
            game.isInstalled = GamesDataFactory.isInstalled(game.offerId);
            game.isSubscription = game.subscriptionAvailable && SubscriptionFactory.userHasSubscription();

            return Promise.resolve(game);
        }

        /**
         * handles isInstalled and IsSubscription errors
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function handleIsInstalledOrSubscriptionError(game) {
            game.isInstalled = false;
            game.isSubscription = false;

            return Promise.resolve(game);
        }

        /**
         * Adds entitlement information to the product model object
         * @param {Object} game model data to augment
         * @return {Promise}
         */
        function loadEntitlementsInfo(game) {
            return isRepurchasableOrAnonymous(game)
                    .catch(_.partial(isParentOwned, game))
                    .catch(_.partial(isAnotherVersionOfParentOwned, game))
                    .catch(_.partial(isBundleContentOwned, game))
                    .catch(_.partial(isShellBundleOwned, game))
                    .catch(_.partial(isShellBundleContentOwned, game))
                    .catch(_.partial(setIsNotOwned, game))
                    .then(isInstalledOrSubscription)
                    .catch(_.partial(handleIsInstalledOrSubscriptionError, game));
        }

        /*
         * Returns a promise loaded with game data (including entitlement info) for a list of given offerIds
         * @param {string[]} offerIds offer ID(s)
         * @return {Promise}
         */
        function getGameDataWithEntitlements(offerIds) {
            if (angular.isString(offerIds)) {
                offerIds = [offerIds];
            } else if (!angular.isArray(offerIds)) {
                return Promise.reject();
            }
            return GamesDataFactory.getCatalogInfo(offerIds)
                    .then(resolveEntitlements)
                    .then(forEachWith(loadEntitlementsInfo));
        }

        // Resolve promise once all entitlement data has been loaded. Reject in case of error or timeout.
        function resolveEntitlements(gameData){
            return new Promise(function (resolve, reject) {
                var entitlementTimeout;
                var funcs = {
                    rejectEntitlement: function(){
                        if(entitlementTimeout) {
                            clearTimeout(entitlementTimeout);
                        }
                        funcs.removeEventListeners();
                        reject("An error occured and you are not able to purchase games at this time. Please try again later. (Entitlements service timed out)");
                    },
                    resetTimeout: function (){
                        if(entitlementTimeout) {
                            clearTimeout(entitlementTimeout);
                        }
                    },
                    resolveSuccess: function() {
                        if(entitlementTimeout) {
                            funcs.resetTimeout();
                        }
                        funcs.removeEventListeners();
                        resolve(gameData);
                    },
                    removeEventListeners: function () {
                        GamesDataFactory.events.off('games:extraContentEntitlementsUpdate', resolveEntitlements);
                        GamesDataFactory.events.off('games:baseGameEntitlementsError', funcs.rejectEntitlement);
                    }
                };

                if(GamesDataFactory.initialRefreshCompleted()) {
                    funcs.resolveSuccess();
                } else{
                    GamesDataFactory.events.once('games:extraContentEntitlementsUpdate', funcs.resolveSuccess);
                    GamesDataFactory.events.once('games:baseGameEntitlementsError', funcs.rejectEntitlement);
                    entitlementTimeout = setTimeout(funcs.rejectEntitlement, ENTITLEMENT_TIMEOUT_MS);
                }
            });
        }

        /**
         * DEPRICATED This function should be fetching the entitlements first to make sure that they are correct then run the below logic
         * Takes a list of products and returns a new list of the owned products
         * @param  {Array} productList The list of product
         * @return {Array}             The items from the list that a user owns.
         */
        function getOwnedProducts(productList) {
            return _.filter(productList, function (offer) {
                return offer.isOwned;
            });
        }
        /**
         * Of the provide products fetch data on the ones that are owned.
         * @param  {Array} productList A List of products
         * @return {Promise} A promise that will fetch product data with entitlements then filter them to ones that are owned.
         */
        function getProductsOwned(productIDList) {
            return getGameDataWithEntitlements(productIDList).then(getOwnedProducts);
        }
        /**
         * Takes a list of products and returns a new list of the unowned products
         * @param  {Array} productList The list of product
         * @return {Array}             The items from the list that a user owns.
         */
        function getUnownedProducts(productIDList) {
            return getGameDataWithEntitlements(productIDList).then(function(productList) {
                return _.filter(productList, function (offer) {
                    return !offer.isOwned;
                });
            });
        }

        /**
         * Takes a list of products and returns a new list of the outright owned products
         * @param  {Array} productList The list of product
         * @return {Array}             The items from the list that a user owns outright.
         */
        function getOutrightOwnedProducts(productList) {
            return _.filter(productList, function (offer) {
                return offer.isOwned && !offer.vaultEntitlement;
            });
        }

        /**
         * Takes a list of products and returns a new list of the outright unowned products
         * @param  {Array} productList The list of product
         * @return {Array}             The items from the list that a user does not own outright.
         */
        function getOutrightUnownedProducts(productList) {
            return _.filter(productList, function (offer) {
                return !offer.isOwned || offer.vaultEntitlement;
            });
        }


        return {
            getPrimaryEdition: getPrimaryEdition,
            getTopOwnedEdition: getTopOwnedEdition,
            getTopSubscriptionEdition: getTopSubscriptionEdition,
            hasSubscriptionEdition: hasSubscriptionEdition,
            userOwnsBetterEdition: userOwnsBetterEdition,
            getGameDataWithEntitlements: getGameDataWithEntitlements,
            loadEntitlementsInfo: loadEntitlementsInfo,
            getOwnedProducts: getOwnedProducts,
            getUnownedProducts: getUnownedProducts,
            getTopOwnedEditions: getTopOwnedEditions,
            getProductsOwned: getProductsOwned,
            getOutrightOwnedProducts: getOutrightOwnedProducts,
            getOutrightUnownedProducts: getOutrightUnownedProducts
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OwnershipFactorySingleton(ObjectHelperFactory, FunctionHelperFactory, GamesDataFactory, SubscriptionFactory, GamesTrialFactory, GameRefiner, VaultRefiner, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OwnershipFactory', OwnershipFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OwnershipFactory
     * @description
     *
     * Ownership: adds dynamic filtering and scope binding to observable data
     */
    angular.module('origin-components')
        .factory('OwnershipFactory', OwnershipFactorySingleton);
}());
