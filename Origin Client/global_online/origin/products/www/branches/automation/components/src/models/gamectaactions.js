/**
 * @file models/gamectaactions.js
 */
(function() {
    'use strict';

    function GamesCTAActionsModel(GamesDataFactory, OwnedGameActionHelper) {
        var template = [
                {'label': 'primarypreordercta', 'callback': GamesDataFactory.buyNow},
                {'label': 'primarybuynowcta', 'callback': GamesDataFactory.buyNow},
                {'label': 'getitwithorigincta', 'callback': OwnedGameActionHelper.getItWithOrigin},
                {'label': 'playonorigincta', 'callback': OwnedGameActionHelper.getItWithOrigin},
                {'label': 'primaryrestorecta', 'callback': Origin.client.games.restore},
                {'label': 'playcta', 'callback': Origin.client.games.play},
                {'label': 'starttrialcta', 'callback': Origin.client.games.play},
                {'label': 'viewspecialoffercta', 'callback': GamesDataFactory.buyNow},
                {'label': 'viewinstorecta', 'callback': GamesDataFactory.buyNow},
                {'label': 'primaryupdatecta', 'callback': Origin.client.games.checkForUpdateAndInstall}, //for when mandatory update exists
                {'label': 'primarydownloadcta', 'callback': Origin.client.games.startDownload},
                {'label': 'primarydownloadnowcta', 'callback': Origin.client.games.moveToTopOfQueue},
                {'label': 'primarypreloadcta', 'callback': Origin.client.games.startDownload},
                {'label': 'primarypreloadnowcta', 'callback': Origin.client.games.moveToTopOfQueue},
                {'label': 'primaryaddtodownloadscta', 'callback': Origin.client.games.startDownload},
                {'label': 'primarypausedownloadcta', 'callback': Origin.client.games.pauseDownload},
                {'label': 'primaryresumedownloadcta', 'callback': Origin.client.games.resumeDownload},
                {'label': 'primarypausepreloadcta', 'callback': Origin.client.games.pauseDownload},
                {'label': 'primaryresumepreloadcta', 'callback': Origin.client.games.resumeDownload},
                {'label': 'primarypauseupdatecta', 'callback': Origin.client.games.pauseUpdate},
                {'label': 'primaryresumeupdatecta', 'callback': Origin.client.games.resumeUpdate},
                {'label': 'primarypauserepaircta', 'callback': Origin.client.games.pauseRepair},
                {'label': 'primaryresumerepaircta', 'callback': Origin.client.games.resumeRepair},
                {'label': 'primaryinstallcta', 'callback': Origin.client.games.install},
                {'label': 'primarypauseinstallcta', 'callback': Origin.client.games.pauseInstall},
                {'label': 'primaryresumeinstallcta', 'callback': Origin.client.games.resumeInstall},
                {'label': 'primaryuninstallcta', 'callback': Origin.client.games.uninstall},
                {'label': 'primaryphasedisplaycta', 'callback': null},
                {'label': 'detailscta', 'callback': GamesDataFactory.showGameDetailsPageForOffer},
                {'label': 'learnmorecta', 'callback': GamesDataFactory.buyNow}, //we want to take them to PDP and buyNow does just that
                {'label': 'getitwithoriginctaincompatible', 'callback': null}
            ];

        function bindOfferIdToCallbacks(offerId) {
            return function (item) {
                if (item.callback) {
                    item.callback = item.callback.bind(this, offerId);
                }
                item.enabled = false;
                return item;
            };
        }

        function primaryActionsList(offerId) {
            var bindFn = bindOfferIdToCallbacks(offerId);
            return _.map(_.cloneDeep(template), bindFn);
        }

        return {
            /**
             * initializes and returns the table of primary actions
             * @param {string} offerId
             * @method primaryActionsList
             */
            primaryActionsList: primaryActionsList,
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesCTAActionsModelSingleton(GamesDataFactory, OwnedGameActionHelper, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesCTAActionsModel', GamesCTAActionsModel, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesCTAActionsModel

     * @description
     *
     * Model that has all of the CTA actions
     */
    angular.module('origin-components')
        .factory('GamesCTAActionsModel', GamesCTAActionsModelSingleton);
}());