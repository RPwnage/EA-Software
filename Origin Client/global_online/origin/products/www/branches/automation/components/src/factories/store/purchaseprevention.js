/**
 * @file purchase.js
 */
(function() {
    'use strict';


    function PurchasePreventionFactory(GamesDataFactory, GameRefiner, ProductInfoHelper, PdpUrlFactory, GiftingFactory, OwnershipFactory, PurchasePreventionConstant, OcdPathFactory, PdpFactory, OfferTransformer, PageThinkingFactory) {
        /**
         * Build the html directive that will go inside the modal (not compiled)
         * @return {html} The directive to add to the modal.
         */
        function getDLCPurchasePreventionModal() {
            return '<origin-dialog-content-bundledlcpurchaseprevention dialogid="' + PurchasePreventionConstant.BUNDLE_DLC_DIALOG_ID + '"></origin-dialog-content-bundledlcpurchaseprevention>';
        }

        /**
         * Build the html directive that will go inside the modal (not compiled)
         *
         * @param {Array<offerIds>} bundle
         * @param {Boolean} isOfferBundle is this bundle an offer bundle rather cart bundle
         * @return {html} The directive to add to the modal.
         */

        function getBundlePurchasePreventionDirectiveXML(bundle, isOfferBundle) {
            isOfferBundle = isOfferBundle ? isOfferBundle : '';
            return '<origin-dialog-purchaseprevention class="origin-dialog-content" is-offer-bundle="'+ isOfferBundle + '" bundle=' +  JSON.stringify(bundle) + ' modal-id=' + PurchasePreventionConstant.BUNDLE_DIALOG_ID +'></origin-dialog-bundlepurchaseprevention>';
        }

        // filter entitlements down to trial entitlements
        function filterTrials(ent) {
            return GameRefiner.isTrial(ent) || GameRefiner.isEarlyAccess(ent);
        }

        // get entitlements using master title id and alternate master title ids
        function getTrialEntitlements(product) {
            var ents = GamesDataFactory.getAllBaseGameEntitlementsByMasterTitleId(_.get(product, 'masterTitleId', '')),
                alternateEnts,
                alternateMasterTitleIds = _.get(product, 'alternateMasterTitleIds', []);

            if(_.size(alternateMasterTitleIds) > 0) {
                _.forEach(alternateMasterTitleIds, function(id) {
                    alternateEnts = GamesDataFactory.getAllBaseGameEntitlementsByMasterTitleId(id);

                    if(_.size(alternateEnts) > 0) {
                        ents.concat(alternateEnts);
                    }
                });
            }

            return ents;
        }

        /**
         * Get all basegame entitlements and filter out all non trial entitlements,
         * if any remain, they dont own a full basegame, but they do own a trial.
         * We know this because we already checked if they own a non trial/alpha/demo/beta
         * basegame, which they didnt. now we check if they own any trial basegames.
         * @param  {Object} product A catalog data object
         * @return {Boolean} Does the user own only a trial basegame
         */
        function isTrialOnlyOwnedBasegame(product) {
            var ents = getTrialEntitlements(product),
                ownedTrials = [];

            if(_.size(ents) > 0) {
                ownedTrials = _.filter(ents, filterTrials);
            }

            return _.size(ownedTrials) > 0;
        }

        function getDLCPurchasePreventionDirective(product, baseGame, isFree, isCurrency) {
            var pdpUrl = baseGame ? PdpUrlFactory.getPdpUrl(baseGame) : '';
            var ownsTrialOnly = isTrialOnlyOwnedBasegame(product);

            return '<origin-dialog-dlcpurchaseprevention owns-trial-only="' + ownsTrialOnly + '" third-party="' + product.isThirdPartyTitle  + '" dialogid="' + PurchasePreventionConstant.DLC_DIALOG_ID + '" is-currency="' + isCurrency + '" is-free="' + isFree + '" dlcgame="' + _.get(product, ['i18n', 'displayName'], '') + '" basegame="' + _.get(baseGame, ['i18n', 'displayName'], '') + '" basehref="' + pdpUrl + '"></origin-dialog-dlcpurchaseprevention>';
        }

        /**
         * Build a data struct that will hold the path for a base game and the product that require it
         * @param  {Array} products A list of products that require a base game
         * @return {Object}          {'OCD Path to base Game': [product, product]}
         */
        function buildRequiredBaseGames(products) {
            return Promise.all(_.map(products, function(product) {
                return GamesDataFactory.getLowestRankedPurchsableBasegame(product).then(function(offerId) {
                    return {offerId: offerId, product: product};
                });
            })).then(function(baseOfferToDLC) {
                var requiredGames = {};
                _.forEach(baseOfferToDLC, function(obj) {
                    requiredGames[obj.offerId] = requiredGames[obj.offerId] || [];
                    requiredGames[obj.offerId].push(obj.product);
                });
                return requiredGames;
            });
        }

        /**
         * Check to make sure that the use owns all the required base games for provided products
         * @param  {Array} productArray An Array of Products
         * @return {Promise}  returns a promise which results in an array of unownedProducts
         */
        function getProductsWithUnownedBaseGames(productArray) {
            return _.filter(productArray, function (product) {
                return needsToBuyBaseGame(product);
            });
        }

        function needsToBuyBaseGame(product) {
            var hasBaseGameEntitlement = false;
            var isDLC = !GameRefiner.isBaseGame(product) && ProductInfoHelper.isDLC(product) || GameRefiner.isCurrency(product);
            if (isDLC) {
                // Check masterTitleId and ignore online subscriptions
                if (GamesDataFactory.getBaseGameEntitlementByMasterTitleId(product.masterTitleId) || GameRefiner.isOnlineSubscription(product)) {
                    hasBaseGameEntitlement = true;
                } else {
                    // Check alternateMasterTitleIds
                    _.forEach(product.alternateMasterTitleIds, function(id) {
                        if (GamesDataFactory.getBaseGameEntitlementByMasterTitleId(id)) {
                            hasBaseGameEntitlement = true;
                        }
                    });
                }
            }

            return (isDLC && !hasBaseGameEntitlement);
        }

        /**
         * return higher weighted owned games than what is included in the bundle
         * @param {Catalog[]} product the bundled products catalog data
         * @param {Catalog[]} editions the bundled products siblings catalog data
         * @returns {Array} array of all owned editions offer ids that have a higher gameEditionTypeFacetKeyRankDesc than the bundle
         */
        function isHigherEditionOwned(product, editions) {
            var unWrappedEditions = PdpFactory.unwrapEditions(editions);
            var ownedGreaterSiblings = _.filter(unWrappedEditions, function(edition) {
                    return _.get(edition, 'isOwned') && !_.get(edition, 'vaultEntitlement') && _.get(edition, 'weight') > _.get(product, 'gameEditionTypeFacetKeyRankDesc');
                });

            if (!_.isEmpty(ownedGreaterSiblings)) {
                return _.map(ownedGreaterSiblings, function(offer) {
                    return _.get(offer, 'offerId', '');
                });
            } else {
                return false;
            }
        }

        /**
         * get siblings catalog data for all offers in a bundle and included ownership values
         * @param {Array} ocdData bundled offers OCD data
         * @returns {Promise} catalog data of bundled offers siblings with ownership values
         */
        function getBundleSiblings(ocdData) {
            var promises = [];

            _.forEach(_.get(ocdData, ['gamehub', 'siblings']), function(siblingOcdPath) {
                promises.push(GamesDataFactory
                    .getCatalogByPath(siblingOcdPath)
                    .then(OfferTransformer.transformOffer));
            });

            return Promise.all(promises);
        }

        /**
         * builds the return object for bundle purchsae prevention
         * @param {Array} data collection of owned offer Ids or false values for each offer in the bundle and its siblings
         * @returns {Array} list of all owned offer ids
         */
        function buildResponseObject(data) {
            var returnObj = [];

            _.forEach(data, function(results) {
                if(_.isArray(results)) {
                    returnObj = returnObj.concat(results);
                }
            });

            return returnObj;
        }

        /**
         * Verifies user entitlement for included offers
         * @param {Catalog[]} product
         * @returns {Array} Array of offer Ids the user owns included in the bundle or greater editions
         */
        function verifyBundleOwnership(product){
            var offerIds = product.bundleOffers || [];
            if (offerIds.length > 0) {
                var promises = [
                    OwnershipFactory
                        .getGameDataWithEntitlements(offerIds)
                        .then(OwnershipFactory.getOutrightOwnedProducts)
                        .then(function(ownedProducts) {
                            if (ownedProducts && ownedProducts.length > 0) {
                                return _.map(ownedProducts, function(offer) {
                                    return _.get(offer, 'offerId', '');
                                });
                            } else {
                                return false;
                            }
                        })
                    ];

                _.forEach(offerIds, function(offerId) {
                    promises.push(GamesDataFactory
                        .getOcdByOfferId(offerId)
                        .then(getBundleSiblings)
                        .then(_.partial(isHigherEditionOwned, product)));
                });

                return Promise.all(promises).then(buildResponseObject);
            } else {
                return Promise.resolve(false);
            }
        }

        /**
         * Checks if current offer is an unopened gift
         * If not, checks other editions as well
         * @param {object} game
         * @returns {Promise}
         */
        function checkUnopenedGift(game){
            return new Promise(function(resolve, reject){
                var hasUnopenedGift;
                var giftOfferId;

                //If Current game if gifted, reject
                if (GiftingFactory.isUnopenedGift(game.offerId)){
                    reject({reason: PurchasePreventionConstant.UNOPENED_GIFT, offerId: game.offerId});
                } else {
                    //Check other editions
                    OcdPathFactory.promiseGet(game.offerPath).then(function (ocdData) {
                        var model = ocdData[game.offerPath];
                        PdpFactory.getEditions(model).then(function (editions) {
                            _.forEach(editions, function (edition) {
                                if (GiftingFactory.isUnopenedGift(edition.offerId)) {
                                    hasUnopenedGift = true;
                                    giftOfferId = edition.offerId;
                                    return false;
                                }
                            });
                            if (hasUnopenedGift) {
                                reject({reason: PurchasePreventionConstant.UNOPENED_GIFT, offerId: giftOfferId});
                            }else{
                                resolve();
                            }
                        });
                    });
                }
            });
        }

        /**
         * Validate add on purchases. Resolves a promise with a status object
         * @param {Catalog[]} products list of catalog objects
         */
        function checkProducts(products){
            return Promise.all(_.map(products, function(product){
                return checkProduct(product);
            }));
        }

        function checkProduct(product) {
            return new Promise(function(resolve) {
                if(!_.isObject(product)) {
                    resolve({
                        success: false,
                        reason: PurchasePreventionConstant.INCORRECT_INPUT
                    });
                } else if(GameRefiner.isOwnedOutright(product) && !GamesDataFactory.isConsumable(product.offerId)) {
                    resolve({
                        success: false,
                        reason: PurchasePreventionConstant.OWNED_OFFER,
                        product: product
                    });
                } else if(needsToBuyBaseGame(product)) {
                    resolve({
                        success: false,
                        reason: PurchasePreventionConstant.BASE_GAME_REQUIRED,
                        product: product
                    });
                } else if (GameRefiner.isBundle(product)){
                    PageThinkingFactory
                        .blockUIPromise(product)
                        .then(verifyBundleOwnership)
                        .then(PageThinkingFactory.unblockUIPromise)
                        .then(function(ownedOfferIds) {
                            if (_.size(ownedOfferIds) > 0) {
                                ownedOfferIds = ownedOfferIds.concat(_.get(product, 'bundleOffers'));

                                resolve({
                                    success: false,
                                    reason: PurchasePreventionConstant.OWNED_SOME_GAME_IN_OFFER,
                                    bundle: _.uniq(ownedOfferIds)
                                });
                            } else {
                                resolve({
                                    success: true,
                                    reason: PurchasePreventionConstant.AVAILABLE_FOR_PURCHASE
                                });
                            }
                        })
                        .catch(function() {
                            PageThinkingFactory.unblockUI();

                            resolve({
                                success: true,
                                reason: PurchasePreventionConstant.AVAILABLE_FOR_PURCHASE,
                                product: product
                            });
                        });
                } else {
                    resolve({
                        success: true,
                        reason: PurchasePreventionConstant.AVAILABLE_FOR_PURCHASE,
                        product: product
                    });
                }
            });
        }

        /**
         * Validate a user either owns or is buying a basegame for all DLC in a bundle
         * @param {Catalog[]} bundleProducts list of catalog objects
         *
         * @return {Array} all DLC/Addons in the bundle the user does not own or is not buying a base game for
         */
        function getProductsWithBaseGameNotInCart(bundleProducts) {
            var productWithoutBaseGames = getProductsWithUnownedBaseGames(bundleProducts);

            _.forEach(bundleProducts, function(offer) {
                if(GameRefiner.isBaseGameOfferType(offer)) {
                    productWithoutBaseGames = _.reject(productWithoutBaseGames, function(dlc) {
                        return (_.get(dlc, 'alternateMasterTitleIds', []).indexOf(_.get(offer, 'masterTitleId', '')) !== -1 || _.get(dlc, 'masterTitleId', '') === _.get(offer, 'masterTitleId', ''));
                    });
                }
            });

            return productWithoutBaseGames;
        }

        return {

            getDLCPurchasePreventionModal: getDLCPurchasePreventionModal,
            getBundlePurchasePreventionDirectiveXML: getBundlePurchasePreventionDirectiveXML,
            getDLCPurchasePreventionDirective: getDLCPurchasePreventionDirective,
            getProductsWithUnownedBaseGames: getProductsWithUnownedBaseGames,
            buildRequiredBaseGames: buildRequiredBaseGames,
            checkProduct: checkProduct,
            checkProducts: checkProducts,
            checkUnopenedGift: checkUnopenedGift,
            getProductsWithBaseGameNotInCart: getProductsWithBaseGameNotInCart
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function PurchasePreventionFactorySingleton(GamesDataFactory, GameRefiner, ProductInfoHelper, PdpUrlFactory, GiftingFactory, OwnershipFactory, PurchasePreventionConstant, OcdPathFactory, PdpFactory, OfferTransformer, PageThinkingFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('PurchasePreventionFactory', PurchasePreventionFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PurchasePreventionFactory
     * @description  Implements purchase prevention related actions
     */
    angular.module('origin-components')
        .constant('PurchasePreventionConstant', {
            BUNDLE_DLC_DIALOG_ID : 'web-bundle-dlc-purchase-prevention-flow',
            BUNDLE_DIALOG_ID : 'web-bundle-purchase-prevention-flow',
            DIALOG_ID : 'web-purchase-prevention-flow',
            OWNED_SOME_GAME_IN_OFFER : 'web-purchase-owned-some-offer-in-bundle',
            DLC_DIALOG_ID : 'web-dlc-purchase-prevention-flow',
            INCORRECT_INPUT : 'incorrect-input',
            UNOPENED_GIFT : 'unopened-gift',
            OWNED_OFFER : 'owned-offer',
            BASE_GAME_REQUIRED : 'base-game-required',
            AVAILABLE_FOR_PURCHASE : 'all-good'
        })
        .factory('PurchasePreventionFactory', PurchasePreventionFactorySingleton);
}());
