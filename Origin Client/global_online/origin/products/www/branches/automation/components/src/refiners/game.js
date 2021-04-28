/**
 * GameRefiner provides various methods to return booleans (true/false)
 * values for various queries on the catalog data
 *
 * @file refiners/game.js
 */
(function () {
    'use strict';

    var SUBTYPES = {
        'Limited Trial': ['trial', 'limited trial'],
        'Weekend Trial': ['trial'],
        'Limited Weekend Trial': ['trial', 'limited trial'],
        'Free Weekend Trial': ['trial'],
        'Gameplay Timer Trial - Access': ['early access'],
        'Gameplay Timer Trial - Game Time': ['trial'],
        'On the House': ['oth'],
        'Demo': ['demo'],
        'Alpha': ['alpha'],
        'Beta': ['beta'],
        'Normal Game': ['normal']
    };

    // TODO: Ask production what the priority of the subtypes are
    var SUBTYPE_PRIORITY = {
        'Normal Game': 2000,
        'Limited Trial': 1000,
        'Weekend Trial': 1000,
        'Limited Weekend Trial': 1000,
        'Free Weekend Trial': 1000,
        'Gameplay Timer Trial - Access': 550,
        'Gameplay Timer Trial - Game Time': 500,
        'Game Giveaway': 110,
        'On The House': 100,
        'Demo': 0,
        'Alpha': 0,
        'Beta': 0
    };

    var ONLINE_SUBSCRIPTION = 'onlinesubscription';

    function GameRefiner() {
        /**
         * Determine if the needle is in the subtype
         * @param {String} subtype
         * @param {String} needle
         * @return {Boolean}
         */
        function inSubType(subtype, needle) {
            return !!SUBTYPES[subtype] && SUBTYPES[subtype].indexOf(needle) >= 0;
        }

        /**
         * Get subtype priority number
         * @param {String} subtype
         * @return {Number}
         */
        function getSubTypePriority(subtype) {
            return SUBTYPE_PRIORITY[subtype] || 0;
        }

        /**
         * Get the game distribution subtype property
         * @param {Object} catalogInfo
         * @return {string}
         */
        function getGamedistributionSubType(catalogInfo) {
            if (!catalogInfo) {
                return;
            }

            return _.get(catalogInfo, 'gameDistributionSubType');
        }

        /**
         * Determine if the game is a trial
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isTrial(catalogInfo) {
            return !isEarlyAccess(catalogInfo) &&
                (inSubType(getGamedistributionSubType(catalogInfo), 'trial') ||
                _.get(catalogInfo, 'type') === 'trial' ||
                _.get(catalogInfo, 'gameTypeFacetKey') === 'TRIAL');
        }


        /**
         * Determine if the game is a limited trial
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isLimitedTrial(catalogInfo) {
            return inSubType(getGamedistributionSubType(catalogInfo), 'limited trial');
        }

        /**
         * Determine if the game is on the house
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isOnTheHouse(catalogInfo) {
            return inSubType(getGamedistributionSubType(catalogInfo), 'oth');
        }

        /**
         * Determine if the game is normal game
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isNormalGame(catalogInfo) {
            return inSubType(getGamedistributionSubType(catalogInfo), 'normal');
        }

        /**
         * Determine if a game is a demo
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isDemo(catalogInfo) {
            return inSubType(getGamedistributionSubType(catalogInfo), 'demo');
        }

        /**
         * Determine if the game is alpha
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isAlpha(catalogInfo) {
            return inSubType(getGamedistributionSubType(catalogInfo), 'alpha');
        }

        /**
         * Determine if the game is a beta
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isBeta(catalogInfo) {
            return inSubType(getGamedistributionSubType(catalogInfo), 'beta');
        }

        function isAlphaBetaDemo(catalogInfo) {
            return isAlpha(catalogInfo) || isBeta(catalogInfo) || isDemo(catalogInfo);
        }

        /**
         * Determine if the game is Early Access
         * @param {object} catalogInfo - catalog data model
         * @return {Boolean}
         */
        function isEarlyAccess(catalogInfo) {
            return inSubType(getGamedistributionSubType(catalogInfo), 'early access');
        }

        /**
         * is a catalog item purchasable
         * @param  {Object} catalogInfo the catalog data model
         * @return {Boolean}
         */
        function isPurchasable(catalogInfo) {
            return (_.get(catalogInfo, 'isPurchasable')) ? true : false;
        }

        /**
         * is a catalog item downloadable
         * @param  {Object} catalogInfo the catalog data model
         * @return {Boolean}
         */
        function isDownloadable(catalogInfo) {
            return (_.get(catalogInfo, 'downloadable')) ? true : false;
        }

        /**
         * is catalog item a full game?
         * @param {Object} catalogInfo the catalog data model
         * @return {Boolean}
         */
        function isFullGame(catalogInfo) {
            return (_.get(catalogInfo, 'originDisplayType') === 'Full Game');
        }

        /**
         * is catalog item a full game with expansions?
         * @param {Object} catalogInfo the catalog data model
         * @return {Boolean}
         */
        function isFullGamePlusExpansion(catalogInfo) {
            return (_.get(catalogInfo, 'originDisplayType') === 'Full Game Plus Expansion');
        }

        /**
         * is the catalog data an addon?
         * @param  {object} catalogInfo the catalog data model
         * @return {Boolean}
         */
        function isAddon(catalogInfo) {
            return (_.get(catalogInfo, 'originDisplayType') === 'Addon');
        }

        /**
         * is the catalog data an expansion?
         * @param  {object} catalogInfo the catalog data model
         * @return {Boolean}
         */
        function isExpansion(catalogInfo) {
            return (_.get(catalogInfo, 'originDisplayType') === 'Expansion');
        }

        /**
         * is the catalog data Extra Content?
         * @param  {object} catalogInfo the catalog data model
         * @return {Boolean}
         */
        function isExtraContent(catalogInfo) {
            return (isFullGamePlusExpansion(catalogInfo) || isAddon(catalogInfo) || isExpansion(catalogInfo));
        }

        /**
         * Is the catalog item a base game?
         * NOTE: this fucntion reads originDisplayType, which is not always what you would expect.
         * This value is used to determine how games shold display in the games library, not what
         * type of game it is. Use isBaseGameOfferType for offer type checking
         * @param {object} catalogInfo the catalog model
         * @return {Boolean}
         */
        function isBaseGame(catalogInfo) {
            return (isFullGame(catalogInfo) || isFullGamePlusExpansion(catalogInfo));
        }

        /**
         * Is the game a hybrid game (Mac and PC universal binary)
         * @param {object} catalogInfo the catalog model
         * @return {Boolean}
         */
        function isHybridGame(catalogInfo) {
            var platformFacetKey = _.get(catalogInfo, 'platformFacetKey');

            if(platformFacetKey) {
                return (platformFacetKey === 'Mac/PC Download' || platformFacetKey === 'Mac/PC Disc');
            }

            return (
                _.get(catalogInfo, ['platforms', 'PCWIN', 'platform']) === 'PCWIN' &&
                _.get(catalogInfo, ['platforms', 'MAC', 'platform']) === 'MAC'
            );
        }

        /**
         * Is PC/Windows game supported
         * @param {object} catalogInfo the catalog model
         * @return {Boolean}
         */
        function isPcGame(catalogInfo) {
            return isHybridGame(catalogInfo) || catalogInfo.platforms.indexOf('PCWIN') >= 0;
        }

        /**
         * Is Mac game supported
         * @param {object} catalogInfo the catalog model
         * @return {Boolean}
         */
        function isMacGame(catalogInfo) {
            return isHybridGame(catalogInfo) || catalogInfo.platforms.indexOf('MAC') >= 0;
        }

        /**
         * Determines if the offer is a trial, demo, beta or alpha
         * @param {object} catalogObject
         * @return {Boolean}
         * @method isAlphaTrialDemoOrBeta
         */
        function isAlphaTrialDemoOrBeta(catalogObject) {
            return catalogObject.trial ||
            catalogObject.beta ||
            catalogObject.demo ||
            catalogObject.earlyAccess ||
            catalogObject.limitedTrial ||
            catalogObject.alpha ? true : false;
        }

        /**
         * Determines if the offer is currency
         * @param {object} catalogInfo
         * @return {Boolean}
         * @method isCurrency
         */
        function isCurrency(catalogInfo) {
            // OfferTransformer transforms 'gameTypeFacetKey' to 'type', hence the extra check.
            return (_.get(catalogInfo, 'gameTypeFacetKey') === 'currency' || _.get(catalogInfo, 'type') === 'currency');
        }

        /**
         * Return true if current product is a bundle and contains offers
         * @param {Object} catalogInfo data as JS object
         * @returns {boolean}
         */
        function isBundle(catalogInfo) {
            return (catalogInfo.type === 'bundles' || catalogInfo.itemType === 'Bundle' || catalogInfo.offerType === 'Bundle') && !_.isUndefined(catalogInfo.bundleOffers) && !_.isEmpty(catalogInfo.bundleOffers);
        }

        /**
         * Determines if the offer grants the user a subscription
         * @param {object} catalogInfo
         * @return {Boolean}
         * @method isSubscription
         */
        function isSubscription(catalogInfo) {
            return (_.get(catalogInfo, 'offerType') === 'Subscription');
        }

        /**
         * Is the catalog item a base game offer type?
         * @param {object} catalogInfo the catalog model
         * @return {Boolean}
         */
        function isBaseGameOfferType(catalogData) {
            return (_.get(catalogData, 'offerType') === 'Base Game' || isBundle(catalogData)) && _.get(catalogData, 'gameTypeFacetKey') !== 'DLC' && _.get(catalogData, 'type') !== 'dlc';
        }

        /**
         * Is the catalog item a browser game
         * @param {object} catalogInfo the catalog model
         * @return {Boolean}
         */
        function isBrowserGame(catalogData) {
            var path = _.get(catalogData, ['platforms', 'PCWIN', 'executePathOverride']);

            return (!!path && path.charAt(0) !== '[' && path.indexOf('http') !== -1 && _.get(catalogData, 'price') === 0);
        }

        /**
         * Returns whether the user has a permanent entitlement to the model (is not vault entitled).
         * @param {object} catalogData the model object
         * @returns {boolean}
         */
        function isOwnedOutright(catalogData) {
            return _.get(catalogData, 'isOwned', false) && !_.get(catalogData, 'vaultEntitlement', false);
        }

        /**
         * Returns whether an offer grants an entitlement, or is a shell bundle,
         * granting no direct entitlment, only its children grant entitlements
         * @param {object} catalogData the model object
         * @returns {boolean}
         */
        function isShellBundle(catalogData) {
            return _.get(catalogData, 'isShellBundle', false);
        }

        /**
         * Returns whether an offer is an online subscription
         * @param {object} catalogData the model object
         * @returns {boolean}
         */
        function isOnlineSubscription(catalogData) {
            return _.get(catalogData, 'gameTypeFacetKey', '') === ONLINE_SUBSCRIPTION || _.get(catalogData, 'type', '') === ONLINE_SUBSCRIPTION;
        }

        return {
            getSubTypePriority: getSubTypePriority,
            isTrial: isTrial,
            isLimitedTrial: isLimitedTrial,
            isOnTheHouse: isOnTheHouse,
            isNormalGame: isNormalGame,
            isDemo: isDemo,
            isAlpha: isAlpha,
            isBeta: isBeta,
            isAlphaBetaDemo: isAlphaBetaDemo,
            isEarlyAccess: isEarlyAccess,
            isAlphaTrialDemoOrBeta: isAlphaTrialDemoOrBeta,
            isPurchasable: isPurchasable,
            isFullGame: isFullGame,
            isFullGamePlusExpansion: isFullGamePlusExpansion,
            isDownloadable: isDownloadable,
            isAddon: isAddon,
            isExpansion: isExpansion,
            isExtraContent: isExtraContent,
            isBaseGame: isBaseGame,
            isHybridGame: isHybridGame,
            isPcGame: isPcGame,
            isMacGame: isMacGame,
            isCurrency: isCurrency,
            isBundle: isBundle,
            isSubscription: isSubscription,
            isBaseGameOfferType: isBaseGameOfferType,
            isBrowserGame: isBrowserGame,
            isOwnedOutright: isOwnedOutright,
            isShellBundle: isShellBundle,
            isOnlineSubscription: isOnlineSubscription,
            SUBTYPES: SUBTYPES
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameRefiner
     * @description Helper method to filter catalog fields
     */
    angular.module('origin-components')
        .factory('GameRefiner', GameRefiner);
}());
