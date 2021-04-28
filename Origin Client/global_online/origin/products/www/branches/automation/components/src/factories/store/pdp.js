/**
 * @file store/pdp.js
 */
(function () {
    'use strict';
    var NO_AVAILABLE_SIBLING = 'NO_AVAILABLE_SIBLING';

    function PdpFactory(OcdPathFactory, ObjectHelperFactory, GameRefiner, GamesDataFactory, GamesCatalogFactory, GamesEntitlementFactory, OfferTransformer, OcdPathFactoryConstant) {
        /**
         * Filters base on if the model is valid.
         * @private
         * @param model
         * @returns {boolean}
         */
        function filterValidModels(model) {
            return !_.isEmpty(model) && _.get(model, 'error') !== OcdPathFactoryConstant.offerNotFoundError && _.get(model, 'isPurchasable', false);
        }

        /**
         * Get first available sibling model with valid data given an invalid game model, either private or can not be found
         * @param {Object} model catalog information that has siblings information
         * @returns {Promise}
         */
        function getFirstAvailableSibling(model) {
            var promise = new Promise(function (resolve, reject) {
                var siblingPaths = _.values(model.siblings);
                if (siblingPaths.length > 0) {
                    OcdPathFactory.promiseGet(siblingPaths).then(function (models) {
                        var newPdpModel = _.first(_.filter(models, filterValidModels));
                        if (newPdpModel) {
                            resolve(newPdpModel);
                        } else {
                            reject(NO_AVAILABLE_SIBLING);
                        }
                    }, reject);
                } else {
                    reject(NO_AVAILABLE_SIBLING);
                }
            });
            return promise;
        }

        function hasBasegameOrBundleEditions(editions, model) {
            return ObjectHelperFactory.length(editions) > 0 && GameRefiner.isBaseGameOfferType(model);
        }

        function isOwnedAndHasWeightGameKeyFranchiseKey(attributeName) {
            return attributeName.isOwned && attributeName.weight && attributeName.gameKey && attributeName.franchiseKey;
        }

        function ownsLesserEdition(editions, model) {
            var ownsLesser = false;

            if (hasBasegameOrBundleEditions(editions, model)) {
                ObjectHelperFactory.forEach(function (attributeName) {
                    if (isOwnedAndHasWeightGameKeyFranchiseKey(attributeName)) {
                        if (attributeName.isOwned === true && attributeName.weight < model.weight &&
                            attributeName.gameKey === model.gameKey &&
                            attributeName.franchiseKey === model.franchiseKey) {
                            ownsLesser = true;
                        }
                    }
                }, editions);
            }

            return ownsLesser;
        }

        function ownsGreaterEdition(editions, model) {
            var ownsGreater = false;

            if (hasBasegameOrBundleEditions(editions, model)) {
                ObjectHelperFactory.forEach(function (attributeName) {
                    if (isOwnedAndHasWeightGameKeyFranchiseKey(attributeName)) {
                        if (attributeName.isOwned === true && attributeName.weight > model.weight &&
                            attributeName.gameKey === model.gameKey &&
                            attributeName.franchiseKey === model.franchiseKey) {
                            ownsGreater = true;
                        }
                    }
                }, editions);
            }

            return ownsGreater;
        }

        function ownsGreaterEditionNotVault(editions, model) {
            var ownsGreater = false;
            if (hasBasegameOrBundleEditions(editions, model)) {
                ObjectHelperFactory.forEach(function (attributeName) {
                    if (isOwnedAndHasWeightGameKeyFranchiseKey(attributeName)) {
                        if (attributeName.vaultEntitlement === false && attributeName.isOwned === true &&
                            attributeName.weight > model.weight && attributeName.gameKey === model.gameKey &&
                            attributeName.franchiseKey === model.franchiseKey) {
                            ownsGreater = true;
                        }
                    }
                }, editions);
            }
            return ownsGreater;
        }

        /**
         * Owns greater or equal editions of the game excluding vault games
         * @param editions
         * @param model
         * @returns {*}
         */
        function ownsGreaterOrEqualEditionNotVault(editions, model) {
            return ownsGreaterEditionNotVault(editions, model) || (model.isOwned === true && model.vaultEntitlement === false);
        }

        // unwraps promise.all and removes empty responses
        function unwrapEditions(editions) {
            return _.compact(_.map(editions, function (offer) {
                return _.first(_.values(offer));
            }));
        }

        function mergeOwnedNotPurchsableOffer(editions, catalogInfo) {
            editions.push(_.first(_.values(catalogInfo)));

            return editions;
        }

        /**
         * validates editions againts entitlements to check that the highest ranked
         * owned basegame is in the list of editions. highest ranked owned games need
         * to be added if it's not purchasable anymore
         *
         * @param {object} editions list of purchaseble siblings
         * @param {object} model game
         * @return {Promise} complete list of editions
         */
        function checkForOwnedNotPurchasableOffers(model, editions) {
            editions.push(model);

            if (GameRefiner.isBaseGameOfferType(model)) {
                var ownedOfferId = GamesEntitlementFactory.getBaseGameEntitlementOfferIdByMasterTitleId(model.masterTitleId);

                if (ownedOfferId) {
                    var ownedEdition = _.filter(editions, {offerId: ownedOfferId});

                    if (_.isEmpty(ownedEdition)) {
                        return GamesCatalogFactory
                            .getCatalogInfo([ownedOfferId])
                            .then(OfferTransformer.transformOffer)
                            .then(_.partial(mergeOwnedNotPurchsableOffer, editions));
                    } else {
                        return Promise.resolve(editions);
                    }
                } else {
                    return Promise.resolve(editions);
                }
            } else {
                return Promise.resolve(editions);
            }
        }

        function handleEditionsError() {
            return Promise.resolve({});
        }

        function getEditions(model) {
            var siblings = _.get(model, 'siblings'),
                promises = _.map(siblings, function (path) {
                return GamesDataFactory
                    .getCatalogByPath(path)
                    .then(OfferTransformer.transformOffer);
            });

            return Promise
                .all(promises)
                .then(unwrapEditions)
                .then(_.partial(checkForOwnedNotPurchasableOffers, model))
                .catch(handleEditionsError);
        }

        /**
         * Get the list of lower edition base game offer IDs
         *
         * @param {object[]} catalogs
         * @returns {*}
         */
        function getLowerEditionBasegameOfferIds(offerId, catalogs){
            var model = catalogs[offerId];
            //If it's not a base game, don't do anything
            if (GameRefiner.isBaseGame(model)){
                return GamesDataFactory.getOcdByOfferId(offerId).then(function(ocd){
                    var siblings = ocd.gamehub.siblings || [];
                    var ocdPaths = _.values(siblings);
                    return OcdPathFactory.promiseGet(ocdPaths).then(function(ocdData){
                        //Filters out non base base and higher editions
                        var filteredModels = _.filter(ocdData, function(siblingModel){
                            return GameRefiner.isBaseGame(siblingModel) && (siblingModel.weight < model.gameEditionTypeFacetKeyRankDesc);
                        });
                        //Check if they are in wishlist
                        return _.pluck(filteredModels, 'offerId');
                    });
                });
            }else{
                return Promise.resolve([]);
            }
        }


        /**
         * Get lower edition offer IDs by an offer ID
         * @param {string} offerId
         * @returns {Promise<string[]>}
         */
        function getLowerEditionBaseGameOfferIdsByOffer(offerId){
            return GamesDataFactory.getCatalogInfo([offerId])
                    .then(_.partial(getLowerEditionBasegameOfferIds, offerId));
        }

        return {
            getEditions: getEditions,
            ownsLesserEdition: ownsLesserEdition,
            ownsGreaterEdition: ownsGreaterEdition,
            ownsGreaterOrEqualEditionNotVault: ownsGreaterOrEqualEditionNotVault,
            ownsGreaterEditionNotVault: ownsGreaterEditionNotVault,
            getFirstAvailableSibling: getFirstAvailableSibling,
            unwrapEditions: unwrapEditions,
            getLowerEditionBaseGameOfferIdsByOffer: getLowerEditionBaseGameOfferIdsByOffer
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.PdpFactory

     * @description
     *
     * Store PDP factory
     */
    angular.module('origin-components')
        .factory('PdpFactory', PdpFactory);
}());
