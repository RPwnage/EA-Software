/**
 *  * @file offertransformer.js
 */
(function () {
    'use strict';

    var PC_PLATFORM = 'PCWIN',
        MAC_PLATFORM = 'MAC',
        CATALOG_DATA_MAP = {
            offerId: 'offerId',
            contentId: 'contentId',
            displayName: ['i18n', 'displayName'],
            shortDescription: ['i18n', 'shortDescription'],
            mediumDescription: ['i18n', 'mediumDescription'],
            longDescription: ['i18n', 'longDescription'],
            packArt: ['i18n', 'packArtLarge'],
            purchasable: ['countries', 'isPurchasable'],
            isPurchasable: 'isPurchasable',
            hasSubsDiscount: ['countries', 'hasSubscriberDiscount'],
            downloadable: 'downloadable',
            oth: 'oth',
            trial: 'trial',
            demo: 'demo',
            beta: 'beta',
            eulaLink: ['i18n', 'eulaURL'],
            gameRatingDesc: 'gameRatingDesc',
            gameRatingDescriptionLong: 'gameRatingDescriptionLong',
            gameRatingPendingMature: 'gameRatingPendingMature',
            gameRatingReason: 'gameRatingReason',
            gameRatingType: 'gameRatingType',
            gameRatingTypeValue: 'gameRatingTypeValue',
            gameRatingUrl: 'gameRatingUrl',
            gameRatingSystemIcon: 'ratingSystemIcon',
            onlineDisclaimer: ['i18n', 'onlineDisclaimer'],
            systemRequirements: ['i18n', 'systemRequirements'],
            subscriptionAvailable: 'vault',
            isUpgradeable: 'isUpgradeable',
            vaultOcdPath: 'vaultOcdPath',
            softwareLocales: 'softwareLocales',
            publisher: 'publisherFacetKey',
            developer: 'developerFacetKey',
            genre: ['genreFacetKey'],
            platforms: 'platforms',
            vaultEntitlement: 'vaultEntitlement',
            earlyAccess: 'earlyAccess',
            type: 'gameTypeFacetKey',
            franchiseKey: 'franchiseFacetKey',
            gameKey: 'gameNameFacetKey',
            officialSite: ['i18n', 'officialSiteURL'],
            forumSite: ['i18n', 'gameForumURL'],
            ocdPath: 'offerPath',
            editionName: 'gameEditionTypeFacetKey',
            editionKey: 'gameEditionType',
            weight: 'gameEditionTypeFacetKeyRankDesc',
            freeBaseGame: 'freeBaseGame',
            masterTitleId: 'masterTitleId',
            trialLaunchDuration: 'trialLaunchDuration',
            originDisplayType: 'originDisplayType',
            bundleOffers: 'bundleOffers',
            mdmItemType: 'mdmItemType',
            itemType: 'itemType',
            giftable: 'giftable',
            i18n: 'i18n',
            gameDistributionSubType: 'gameDistributionSubType',
            alternateMasterTitleIds: 'alternateMasterTitleIds',
            offerType: 'offerType',
            ogdOfferId: 'ogdOfferId',
            durationUnit: 'durationUnit',
            isThirdPartyTitle: 'isThirdPartyTitle',
            isShellBundle: 'isShellBundle'
        };

    function OfferTransformer(ComponentsLogFactory, CurrencyHelper, GameRefiner, GamesDataFactory, ObjectHelperFactory, OwnershipFactory, RatingSystemFactory) {
        var transformWith = ObjectHelperFactory.transformWith,
            mapWith = ObjectHelperFactory.mapWith,
            wrapInArray = ObjectHelperFactory.wrapInArray,
            forEachWith = ObjectHelperFactory.forEachWith,
            forEach = ObjectHelperFactory.forEach;

        function formatPriceValue(value, model, destination) {
            if(angular.isDefined(model)) {
                if(angular.isDefined(value)) {
                    CurrencyHelper.formatCurrency(value)
                        .then(function(result){
                            model[destination] = result;
                        });
                } else {
                    model[destination] = '--';
                }
            }
        }

        /**
         * Some rating systems (DJCTQ) have their rating reasons listed as a comma-delimited string
         * in gameRatingDesc property so we'll put them where they should be.
         * @param {Object} games collection of models to augment
         * @return {Object}
         */
        function normalizeGameRatingReasons(games) {
            _.forEach(games, function(offerData) {
                if (offerData.gameRatingReason && _.isEmpty(offerData.gameRatingDesc)) {
                    offerData.gameRatingDesc = _.compact(offerData.gameRatingReason.split(','));
                }
            });
            return games;
        }

        /**
         * Adds prices and discounts to the product model objects
         * @param {Object} games collection of models to augment
         * @return {Function}
         */
        function addPriceInfo(games) {
            return function (dataItem) {

                if (dataItem && dataItem.offerId && games.hasOwnProperty(dataItem.offerId)) {
                    var game = games[dataItem.offerId],
                        prices = dataItem.rating && dataItem.rating[0] ? dataItem.rating[0] : undefined;

                    if (prices) {
                        // reset oth based on discount rate
                        game.oth = (GameRefiner.isOnTheHouse(game) && prices.totalDiscountRate === 1) ? true : false;

                        game.price = prices.finalTotalAmount;
                        game.strikePrice = prices.originalConfiguredPrice || prices.originalTotalPrice || 0;
                        game.savings = prices.totalDiscountRate ? (100 * prices.totalDiscountRate).toFixed(0) : 0;

                        game.formattedPrice = "";
                        game.formattedStrikePrice = "";
                        game.formattedIncluded = "";
                        
                        formatPriceValue(game.price, game, 'formattedPrice');
                        formatPriceValue(game.strikePrice, game, 'formattedStrikePrice');
                        formatPriceValue(game.savings, game, 'formattedIncluded');
                        game.currency = prices.currency || '';
                    } else {
                        ComponentsLogFactory.log('[ProductFactory]: Cannot find model for the price data being loaded');
                    }
                }
            };
        }

        /**
         * Loads prices and discounts information from the Ratings service
         * @param {Object|Array} games collection of models to augment
         * @return {Promise}
         */
        function loadPrices(games) {
            var offerIds = Object.keys(games),
                promises = [];

            forEach(function(offerId) {
                promises.push(GamesDataFactory.getPrice(offerId, games[offerId].fictionalCurrencyCode)
                    .then(wrapInArray)
                    .then(forEachWith(addPriceInfo(games))));
            }, offerIds);

            return Promise.all(promises)
                .then(function () {
                    return games;
                });
        }

        /**
         * Adds ODC profile data to the product model objects
         * @param {Object} games collection of models to augment
         * @return {Function}
         * @throws {Error} If ODC profile data could not be found
         * @throws {Error} If catalog/entitlement info for the given offer could not be found
         */
        function addOdcProfileInfo(games, offerId) {
            return function (profileData) {
                if (!profileData || !games.hasOwnProperty(offerId)) {
                    throw new Error('[ProductFactory]: Cannot find model for the ODC profile data being loaded');
                }

                var game = games[offerId];
                game.odcProfile = profileData.odcProfile || 'oig-real';
                game.fictionalCurrencyCode = profileData.fictionalCurrencyCode || '';
                game.categoryId = profileData.categoryId || '';
                game.invoiceSource = profileData.invoiceSource || '';
            };
        }

        /**
         * Loads ODC profile data from CMS
         * @param {Object|Array} games collection of models to augment
         * @return {Promise}
         */
        function loadOdcProfiles(games) {
            var offerIds = Object.keys(games),
                promises = [];

            forEach(function(offerId) {
                // ODC profiles only apply to DLC. Note that we check !isBaseGame instead of isExtraContent
                // so that we catch 'Game Only' DLC such as FIFA ultimate team packs. We are also operating
                // under the assumption that offers that grant virtual currency will not cost virtual currency.
                if (!GameRefiner.isBaseGame(games[offerId]) && !GameRefiner.isCurrency(games[offerId])) {
                    promises.push(GamesDataFactory.getOdcProfile(offerId)
                        .then(addOdcProfileInfo(games, offerId)));
                }
            }, offerIds);

            return Promise.all(promises)
                .then(function () {
                    return games;
                });
        }

        function setReleaseDate(model) {
            var os = Origin.utils.os();

            var pcReleaseDate = _.get(model, ['platforms', PC_PLATFORM, 'releaseDate']),
                macReleaseDate = _.get(model, ['platforms', MAC_PLATFORM, 'releaseDate']),
                pcDownloadStartDate = _.get(model, ['platforms', PC_PLATFORM, 'downloadStartDate']),
                pcUseEndDate =  _.get(model, ['platforms', PC_PLATFORM, 'useEndDate']),
                macDownloadStartDate =_.get(model, ['platforms', MAC_PLATFORM, 'downloadStartDate']),
                macUseEndDate = _.get(model, ['platforms', MAC_PLATFORM, 'useEndDate']);

            if (os === PC_PLATFORM) {
                model.releaseDate = pcReleaseDate || macReleaseDate;
                model.downloadStartDate = pcDownloadStartDate || macDownloadStartDate;
                model.useEndDate = pcUseEndDate || macUseEndDate;
            } else {
                model.releaseDate = macReleaseDate || pcReleaseDate;
                model.downloadStartDate = macDownloadStartDate || pcDownloadStartDate;
                model.useEndDate = macUseEndDate || pcUseEndDate;
            }

            return model;
        }

        function transformOffer(promise) {
            return Promise.resolve(promise)
                .then(mapWith(transformWith(CATALOG_DATA_MAP)))
                .then(forEachWith(setReleaseDate))
                .then(forEachWith(OwnershipFactory.loadEntitlementsInfo))
                .then(loadOdcProfiles)
                .then(loadPrices)
                .then(normalizeGameRatingReasons)
                .then(RatingSystemFactory.updateGamesWithLocalizedRatings);
        }

        return {
            transformOffer: transformOffer,
            loadPrices: loadPrices
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OfferTransformerSingleton(
        ComponentsLogFactory,
        CurrencyHelper,
        GameRefiner,
        GamesDataFactory,
        ObjectHelperFactory,
        OwnershipFactory,
        RatingSystemFactory,
        SingletonRegistryFactory
    ) {
        return SingletonRegistryFactory.get('OfferTransformer', OfferTransformer, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OfferTransformer
     * @description Transforms (remaps) offer fields
     *
     * OfferTransformer
     */
    angular.module('origin-components')
        .factory('OfferTransformer', OfferTransformerSingleton);
}());

