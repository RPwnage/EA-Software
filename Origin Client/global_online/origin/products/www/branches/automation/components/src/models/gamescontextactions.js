/**
 * @file game/gamescontextactions.js
 */
(function() {
    'use strict';

    function GamesContextActionsFactory(GamesDataFactory, NavigationFactory) {
        var template = [
                {'label': 'downloadnowcta', 'callback': Origin.client.games.moveToTopOfQueue},
                {'label': 'startdownloadcta', 'callback': Origin.client.games.startDownload},
                {'label': 'pausedownloadcta', 'callback': Origin.client.games.pauseDownload},
                {'label': 'resumedownloadcta', 'callback': Origin.client.games.resumeDownload},
                {'label': 'canceldownloadcta', 'callback': Origin.client.games.cancelDownload},
                {'label': 'playcta', 'callback': Origin.client.games.play},
                {'label': 'starttrialcta', 'callback': Origin.client.games.play},
                {'label': 'cancelcloudsynccta', 'callback': Origin.client.games.cancelCloudSync},
                {'label': 'pauseupdatecta', 'callback': Origin.client.games.pauseUpdate},
                {'label': 'resumeupdatecta', 'callback': Origin.client.games.resumeUpdate},
                {'label': 'cancelupdatecta', 'callback': Origin.client.games.cancelUpdate},
                {'label': 'preloadcta', 'callback': Origin.client.games.startDownload},
                {'label': 'pausepreloadcta', 'callback': Origin.client.games.pauseDownload},
                {'label': 'resumepreloadcta', 'callback': Origin.client.games.resumeDownload},
                {'label': 'cancelpreloadcta', 'callback': Origin.client.games.cancelDownload},
                {'label': 'pauseinstallcta', 'callback': Origin.client.games.pauseInstall},
                {'label': 'resumeinstallcta', 'callback': Origin.client.games.resumeInstall},
                {'label': 'cancelinstallcta', 'callback': Origin.client.games.cancelInstall},
                {'label': 'pauserepaircta', 'callback': Origin.client.games.pauseRepair},
                {'label': 'resumerepaircta', 'callback': Origin.client.games.resumeRepair},
                {'label': 'cancelrepaircta', 'callback': Origin.client.games.cancelRepair},
                {'label': 'installcta', 'callback': Origin.client.games.install},
                {'label': 'installparentcta', 'callback': Origin.client.games.installParent},
                {'label': 'showgamedetailscta', 'callback': GamesDataFactory.showGameDetailsPageForOffer},
                {'label': 'addasfavoritecta', 'callback': GamesDataFactory.addFavoriteGame},
                {'label': 'removeasfavoritecta', 'callback': GamesDataFactory.removeFavoriteGame},
                {'label': 'viewachievementscta', 'callback': NavigationFactory.goOgdAchievements},
                {'divider': true, 'visible': true},
                {'label': 'viewinstorecta', 'callback': GamesDataFactory.buyNow},
                {'divider': true, 'visible': true},
                {'label': 'updategamecta', 'callback': Origin.client.games.checkForUpdateAndInstall},
                {'label': 'checkforupdatescta', 'callback': Origin.client.games.checkForUpdateAndInstall},
                {'label': 'installupdatecta', 'callback': Origin.client.games.installUpdate},
                {'label': 'updatenowcta', 'callback': Origin.client.games.checkForUpdateAndInstall},
                {'label': 'repaircta', 'callback': Origin.client.games.repair}, //actual Repair
                {'label': 'repairnowcta', 'callback': Origin.client.games.repair},
                {'label': 'restorecta', 'callback': Origin.client.games.restore},
                {'label': 'customizeboxartcta', 'callback': Origin.client.games.customizeBoxArt},
                {'label': 'gamepropertiescta', 'callback': GamesDataFactory.openGameProperties},
                {'divider': true, 'visible': true},
                {'label': 'removefromlibrarycta', 'callback': GamesDataFactory.removeFromLibrary},
                {'label': 'hidecta', 'callback': GamesDataFactory.hideGame},
                {'label': 'unhidecta', 'callback': GamesDataFactory.unhideGame},
                {'label': 'uninstallcta', 'callback': GamesDataFactory.uninstall},
                {'label': 'openinodtcta', 'callback': Origin.client.games.openInODT}
            ],
            GAME_SETTINGS_BUTTON_CONTEXT = 'origin-game-settings-button';

        function bindOfferIdToCallbacks(offerId, context) {
            return function (item) {
                if (item.callback) {
                    var boundCallback = item.callback.bind(this, offerId);
                    item.callback = function() {
                        boundCallback();
                        
                        //we need to differentiate where the button click came from so we have to check the passed in context
                        if (context === GAME_SETTINGS_BUTTON_CONTEXT) {
                            Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_OGD_COGMENU, item.labelId, offerId);
                        } else {
                            Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_GAMELIBRARY_CONTEXTMENU, item.labelId, offerId);
                        }
                    };
                    // only disable the labels and not the dividers which have no callback
                    item.enabled = false;
                    item.visible = false;
                }
                // this is to preserve the id before translation to send to telemetry
                item.labelId = item.label;
                return item;
            };
        }

        function baseActionsList(offerId, context) {
            var bindFn = bindOfferIdToCallbacks(offerId, context);
            return _.map(_.cloneDeep(template), bindFn);
        }

        return {

            /**
             * Initializes and returns the base table of context actions
             * @param {string} offerId
             * @param {string} context
             * @return {object} a list of ordered context action objects
             * @method initialActionsList
             */
            baseActionsList: baseActionsList
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesContextActionsFactorySingleton(GamesDataFactory, NavigationFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesContextActionFactory', GamesContextActionsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesContextActionFactory

     * @description
     *
     * GamesContextActionFactory
     */
    angular.module('origin-components')
        .factory('GamesContextActionsFactory', GamesContextActionsFactorySingleton);
}());