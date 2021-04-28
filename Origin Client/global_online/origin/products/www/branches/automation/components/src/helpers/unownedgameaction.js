/**
 * @file unownedgameaction.js
 */
(function() {
    'use strict';

    var ACTION_PREORDER = 'preorder',
        ACTION_SUBSCRIBE = 'subscribe',
        ACTION_VAULT = 'vault',
        ACTION_BUY = 'buy',
        ACTION_BUY_ADDON = 'buy-addon',
        ACTION_FREE = 'free',
        ACTION_NONE = '';

    function UnownedGameActionHelper(PurchaseFactory, ComponentsLogFactory, ProductInfoHelper, GameRefiner) {
        /**
         * Checks if an action is an action that should show savings (50% off $strike)
         * @param {string} action the action to compare
         * @return {boolean}
         */
        function isSavingsAction(action) {
            return action === ACTION_PREORDER || action === ACTION_BUY || action === ACTION_BUY_ADDON || action === ACTION_FREE;
        }

        /**
         * Returns appropriate purchase action for the given game
         * @param {Object} game game data as a plain JS object
         * @return {string}
         */
        function getPurchaseAction(game) {
            if (ProductInfoHelper.isPreorderable(game)) {
                return ACTION_PREORDER;
            } else if (GameRefiner.isAddon(game)) {
                return ACTION_BUY_ADDON;
            } else {
                return ACTION_BUY;
            }
        }

        function getPrimaryAction (game) {
            if (ProductInfoHelper.isFree(game)) {
                return ACTION_FREE;
            } else if (ProductInfoHelper.isPurchasable(game)) {
                return getPurchaseAction(game);
            } else {
                return ACTION_NONE;
            }
        }

        function startAction (action, game) {
            switch(action) {
                case ACTION_PREORDER:
                case ACTION_BUY:
                    PurchaseFactory.buy(game.offerId);
                    break;
                case ACTION_BUY_ADDON:
                    PurchaseFactory.buyAddons([game.offerId]);
                    break;
                case ACTION_SUBSCRIBE:
                    //subscribe
                    break;
                case ACTION_VAULT:
                    PurchaseFactory.entitleVaultGame(game.offerId);
                    break;
                case ACTION_FREE:
                    PurchaseFactory.entitleFreeGame(game.offerId);
                    break;
                case ACTION_NONE:
                    ComponentsLogFactory.error('[UnownedGameActionHelper] Tried to execute ACTION_NONE.');
                    break;
                default:
                    ComponentsLogFactory.error('[UnownedGameActionHelper] Tried to execute undefined action.');
                    break;
            }
        }

        return {
            isSavingsAction: isSavingsAction,
            getPrimaryAction: getPrimaryAction,
            startAction: startAction
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.UnownedGameActionHelper
     * @description  Determines which action to perform on unowned games.
     */
    angular.module('origin-components')
        .factory('UnownedGameActionHelper', UnownedGameActionHelper);
}());
