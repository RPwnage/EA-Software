/**
 * @file store/actionflow.js
 */
(function() {
    'use strict';

    function StoreActionFlowFactory(AuthFactory, OwnedGameActionHelper, UnownedGameActionHelper, GameRefiner, ProductInfoHelper) {
        /**
         * Checks if an action is an action that should show savings (50% off $strike)
         * @param {string} action the action to compare
         * @return {boolean}
         */
        function isSavingsAction(action) {
            return UnownedGameActionHelper.isSavingsAction(action);
        }

        /**
         * Checks if an action is an action that should be disabled
         * @param {string} action the action to compare
         * @return {boolean}
         */
        function isDisabledAction(action) {
            return OwnedGameActionHelper.isDisabledAction(action);
        }

        /**
         *
         * @param game
         * @returns {boolean}
         */
        function isPlayAction(game) {
            return game && game.isOwned &&
                !GameRefiner.isBundle(game) &&
                (!game.vaultEntitlement || game.oth) &&
                !GameRefiner.isBrowserGame(game) && ProductInfoHelper.isReleased(game);
        }

        /**
         *
         * @param game
         * @returns {*|boolean}
         */
        function isPurchaseAction(game) {
            return game && !GameRefiner.isOwnedOutright(game) && !ProductInfoHelper.isFree(game) && !GameRefiner.isBrowserGame(game);
        }

        /**
         *
         * @param game
         * @returns {*|boolean}
         */
        function isEntitleAction(game) {
            return game && !GameRefiner.isOwnedOutright(game) && ProductInfoHelper.isFree(game) && !GameRefiner.isBrowserGame(game);
        }

        /**
         * Checks which action helper to use base on ownership
         * @param {Object} game game data object
         * @return {Object} action helper
         */
        function getFactory(game){
            return GameRefiner.isOwnedOutright(game) ? OwnedGameActionHelper : UnownedGameActionHelper;
        }

        /**
         * Returns the most relevant action available for the current user on the given game
         * @param {Object} game game data object
         * @return {string}
         */
        function getPrimaryAction(game) {
            return getFactory(game).getPrimaryAction(game);
        }

        /**
         * Helper method. Directs flow to the appropriate action handler
         * @param {Object} game The game data of the game that the action is going to be performed on.
         * @param {string} action The action to be performed
         */
        function handleAction(game, action) {
            getFactory(game).startAction(action, game);
        }

        function startActionFlow(game, action) {
            // log in then handle specific action cases
            AuthFactory.promiseLogin()
                .then(function () {
                    handleAction(game, action);
                });
        }

        return {
            getPrimaryAction: getPrimaryAction,
            startActionFlow: startActionFlow,
            isPurchaseAction: isPurchaseAction,
            isEntitleAction: isEntitleAction,
            isPlayAction: isPlayAction,
            isSavingsAction: isSavingsAction,
            isDisabledAction: isDisabledAction
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.StoreActionFlowFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('StoreActionFlowFactory', StoreActionFlowFactory);
})();
