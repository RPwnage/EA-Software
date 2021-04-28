/**
 * @file store/product.js
 */
(function() {
    'use strict';

    var ACTION_PRELOAD = 'preload',
        ACTION_PRELOADED = 'preloaded',
        ACTION_PREORDER = 'preorder',
        ACTION_SUBSCRIBE = 'subscribe',
        ACTION_BUY = 'buy',
        ACTION_DOWNLOAD = 'download',
        ACTION_PLAY = 'play',
        ACTION_FREE = 'free',
        ACTION_NONE = '';

    function StoreActionFlowFactory(ObservableFactory, DateHelperFactory, DialogFactory) {
        var past = DateHelperFactory.isInThePast;

        /**
         * Returns true if the given game is free
         * @param {Object} game game data as a plain JS object
         * @return {boolean}
         */
        function isFree(game) {
            return game.oth || game.trial || game.demo;
        }

        /**
         * Returns true if the current user can purchase the given game
         * @param {Object} game game data as a plain JS object
         * @return {boolean}
         */
        function canBuy(game) {
            return game.purchasable && !game.isOwned && !isFree(game);
        }

        /**
         * Checks if the current user can download the game
         * @param {Object} game game data as a plain JS object
         * @return {boolean}
         */
        function canDownload(game) {
            return !game.isInstalled && (game.isSubscription || game.isOwned);
        }

        /**
         * Checks if an action is an action that should show savings (50% off $strike)
         * @param {string} action the action to compare
         * @return {boolean}
         */
        function isSavingsAction(action) {
            return action === ACTION_PREORDER || action === ACTION_BUY || action === ACTION_FREE;
        }

        /**
         * Checks if an action is an action that should be disabled
         * @param {string} action the action to compare
         * @return {boolean}
         */
        function isDisabledAction(action) {
            return action === ACTION_PRELOADED;
        }

        /**
         * Returns appropriate download action for the given game
         * @param {Object} game game data as a plain JS object
         * @return {string}
         */
        function getDownloadAction(game) {
            if (past(game.releaseDate)) {
                return ACTION_DOWNLOAD;
            } else if (past(game.downloadStartDate)) {
                return ACTION_PRELOAD;
            } else {
                return ACTION_NONE;
            }
        }

        /**
         * Returns appropriate purchase action for the given game
         * @param {Object} game game data as a plain JS object
         * @return {string}
         */
        function getPurchaseAction(game) {
            if (past(game.releaseDate)) {
                return ACTION_BUY;
            } else {
                return ACTION_PREORDER;
            }
        }

        /**
         * Returns if the given game is 'preloaded'. AKA a preload that is installed. 
         * @param {Object} game game data as a plain JS object
         * @return {boolean}
         */
        function isPreloaded(game) {
            return game.isInstalled && getDownloadAction(game) === ACTION_PRELOAD;
        }

        /**
         * Returns the most relevant action available for the current user on the given game
         * @param {Object} modelObject game model object: Observable or plain JS
         * @return {string}
         */
        function getPrimaryAction(modelObject) {
            var game = ObservableFactory.extract(modelObject);

            if (isPreloaded(game)) {
                return ACTION_PRELOADED;

            } else if (game.isInstalled) {
                return ACTION_PLAY;

            } else if (canDownload(game)) {
                return getDownloadAction(game);

            } else if (isFree(game)) {
                return ACTION_FREE;

            } else if (game.subscriptionAvailable) {
                return ACTION_SUBSCRIBE;

            } else if (canBuy(game)) {
                return getPurchaseAction(game);

            } else {
                return ACTION_NONE;
            }
        }

        /**
         * Returns the secondary action available for the current user on the given game
         * @param {Object} modelObject game model object: Observable or plain JS
         * @return {string}
         */
        function getSecondaryAction(modelObject) {
            var game = ObservableFactory.extract(modelObject);

            if (canBuy(game)) {
                return getPurchaseAction(game);
            } else {
                return ACTION_NONE;
            }
        }

        // !! Mock method, to be removed in following iterations
        // @todo: replace this method with appropriate flow when the back-end services are ready
        function startActionFlow(modelObject) {
            var game = ObservableFactory.extract(modelObject);

            DialogFactory.openDirective({
                    contentDirective: 'origin-dialog-content-checkoutconfirmation',
                    id: 'checkout-confirmation',
                    name: 'origin-dialog-content-checkoutconfirmation',
                    size: 'large',
                    data: {
                        "offers": [game.offerId]
                    }
                });
        }

        return {
            getPrimaryAction: getPrimaryAction,
            getSecondaryAction: getSecondaryAction,
            startActionFlow: startActionFlow,
            isSavingsAction: isSavingsAction,
            isDisabledAction: isDisabledAction
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function StoreActionFlowFactorySingleton(ObservableFactory, DateHelperFactory, DialogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('StoreActionFlowFactory', StoreActionFlowFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.StoreActionFlowFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('StoreActionFlowFactory', StoreActionFlowFactorySingleton);
}());