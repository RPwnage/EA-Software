/**
 * @file ownedgameaction.js
 */
(function() {
    'use strict';

    var ACTION_PRELOAD = 'preload',
        ACTION_PRELOADED = 'preloaded',
        ACTION_DOWNLOAD = 'download',
        ACTION_PLAY = 'play',
        ACTION_NONE = '';

    function OwnedGameActionHelper(ProductInfoHelper, BeaconFactory, DialogFactory, OgdUrlFactory, ComponentsLogFactory, $state) {
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
            if (ProductInfoHelper.isDownloadable(game)) {
                return ACTION_DOWNLOAD;
            } else if (ProductInfoHelper.isPreloadable(game)) {
                return ACTION_PRELOAD;
            } else {
                return ACTION_NONE;
            }
        }

        /**
         * Returns if the given game is 'preloaded'. AKA a preload that is installed.
         * @param {Object} game game data as a plain JS object
         * @return {boolean}
         */
        function isPreloaded(game) {
            return ProductInfoHelper.isInstalled(game) && getDownloadAction(game) === ACTION_PRELOAD;
        }

        function getPrimaryAction (game) {
            if (isPreloaded(game)) {
                return ACTION_PRELOADED;
            } else if (ProductInfoHelper.isInstalled(game)) {
                return ACTION_PLAY;
            } else {
                return getDownloadAction(game);
            }
        }

        function startAction (action, game) {
            switch(action) {
                case ACTION_PRELOADED:
                    // do nothing
                    break;
                case ACTION_PRELOAD:
                    // preload
                    break;
                case ACTION_DOWNLOAD:
                    // download
                    break;
                case ACTION_NONE:
                    ComponentsLogFactory.error('[OwnedGameActionHelper] Tried to execute ACTION_NONE. Game Data: ' + game.toString());
                    break;
                default:
                    ComponentsLogFactory.error('[OwnedGameActionHelper] Tried to execute undefined action.');
                    break;
            }
        }

        /**
        * Launch the download dialog for the game
        * @param {String} offerId
        */
        function launchDownloadDialog(offerId) {
            DialogFactory.openDirective({
                contentDirective: 'origin-dialog-content-youneedorigin',
                id: 'need-origin-download',
                name: 'origin-dialog-content-youneedorigin',
                size: 'large',
                data: {
                    'offer-id': offerId,
                    id: 'need-origin-download'
                }
            });
        }

        /**
        * Go to the OGD if you are not already there
        * @param {String} offerId
        */
        function goToOgd(offerId) {
            if (!$state.current.data.onOwnedGameDetails) {
                OgdUrlFactory.goToOgd({'offerId':offerId});
            }
        }

        /**
        * Call the JSSDK play method if the client is installed
        * otherwise launch the download dialog
        * @param {String} offerId - the offerId to play/download/install
        */
        function playOrDownloadClient(offerId) {
            return function(isClientInstalled, isClientInstallable) {
                if (isClientInstalled) {
                    Origin.client.games.play(offerId);
                } else if (isClientInstallable) {
                    launchDownloadDialog(offerId);
                } else {
                    goToOgd(offerId);
                }
            };
        }

        /**
        * Play / download the game if the client is available
        * otherwise if you can install the client, ask the user to
        * install the client.
        * @param {String} offerId - the offerId to play/download/install
        */
        function getItWithOrigin(offerId) {
            Promise.all([
                BeaconFactory.isInstalled(),
                BeaconFactory.isInstallable()
            ])
            .then(_.spread(playOrDownloadClient(offerId)));
        }

        return {
            isDisabledAction: isDisabledAction,
            getPrimaryAction: getPrimaryAction,
            startAction: startAction,
            getItWithOrigin: getItWithOrigin
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OwnedGameActionHelper
     * @description  Determines which action to perform on owned games and implements these actions.
     *
     */
    angular.module('origin-components')
        .factory('OwnedGameActionHelper', OwnedGameActionHelper);
}());
