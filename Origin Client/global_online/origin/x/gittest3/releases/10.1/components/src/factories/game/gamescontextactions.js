/**
 * @file game/gamescontextactions.js
 */
(function() {
    'use strict';

    function GamesContextActionsFactory(GamesDataFactory) {
        var actions = [],
            ctaPrimaryActions = [];



        function addAction(actionList, label, callback) {
            var action = {
                'label': label,
                'callback': callback,
                'visible': false,
                'enabled': false
            };

            actionList.push(action);
        }

        function addDivider() {
            var action = {
                divider: true,
                visible: true
            };

            actions.push(action);
        }

        function initActions(offerId) {
            actions = [];

            //order is important because this is how it would be shown in the context menu and based on whether it's enabled or not
            //section 1 Main Action, Important Views and Actions
            addAction(actions, 'startdownloadcta', Origin.client.games.startDownload.bind(this, offerId));
            addAction(actions, 'pausedownloadcta', Origin.client.games.pauseDownload.bind(this, offerId));
            addAction(actions, 'resumedownloadcta', Origin.client.games.resumeDownload.bind(this, offerId));
            addAction(actions, 'canceldownloadcta', Origin.client.games.cancelDownload.bind(this, offerId));
            addAction(actions, 'downloadnowcta', Origin.client.games.moveToTopOfQueue.bind(this, offerId));

            addAction(actions, 'playcta', Origin.client.games.play.bind(this, offerId));
            addAction(actions, 'starttrialcta', Origin.client.games.play.bind(this, offerId));
            addAction(actions, 'cancelcloudsynccta', Origin.client.games.cancelCloudSync.bind(this, offerId));

            addAction(actions, 'pauseupdatecta', Origin.client.games.pauseUpdate.bind(this, offerId));
            addAction(actions, 'resumeupdatecta', Origin.client.games.resumeUpdate.bind(this, offerId));
            addAction(actions, 'cancelupdatecta', Origin.client.games.cancelUpdate.bind(this, offerId));

            addAction(actions, 'pauseinstallcta', Origin.client.games.pauseInstall.bind(this, offerId));
            addAction(actions, 'resumeinstallcta', Origin.client.games.resumeInstall.bind(this, offerId));
            addAction(actions, 'cancelinstallcta', Origin.client.games.cancelInstall.bind(this, offerId));

            addAction(actions, 'pauserepaircta', Origin.client.games.pauseRepair.bind(this, offerId));
            addAction(actions, 'resumerepaircta', Origin.client.games.resumeRepair.bind(this, offerId));
            addAction(actions, 'cancelrepaircta', Origin.client.games.cancelRepair.bind(this, offerId));

            addAction(actions, 'installcta', Origin.client.games.install.bind(this, offerId));
            addAction(actions, 'installparentcta', Origin.client.games.installParent.bind(this, offerId));
            addAction(actions, 'viewspecialoffercta', GamesDataFactory.viewOffer.bind(this, offerId));
            addAction(actions, 'viewinstorecta', GamesDataFactory.viewOffer.bind(this, offerId));
            addAction(actions, 'showgamedetailscta', GamesDataFactory.showGameDetails.bind(this, offerId));
            addAction(actions, 'showachievementscta', GamesDataFactory.showAchievements.bind(this, offerId));
            addAction(actions, 'addasfavoritecta', GamesDataFactory.addFavoriteGame.bind(this, offerId));
            addAction(actions, 'removeasfavoritecta', GamesDataFactory.removeFavoriteGame.bind(this, offerId));
            addDivider();

            // Section 2: Game state and fixing
            addAction(actions, 'updategamecta', Origin.client.games.checkForUpdateAndInstall.bind(this, offerId));
            addAction(actions, 'checkforupdatescta', Origin.client.games.checkForUpdateAndInstall.bind(this, offerId));
            addAction(actions, 'installupdatecta', Origin.client.games.installUpdate.bind(this, offerId));
            addAction(actions, 'updatenowcta', Origin.client.games.checkForUpdateAndInstall.bind(this, offerId));
            addAction(actions, 'repaircta', Origin.client.games.repair.bind(this, offerId)); //actual Repair
            addAction(actions, 'repairnowcta', Origin.client.games.repair.bind(this, offerId));
            addAction(actions, 'restorecta', Origin.client.games.restore.bind(this, offerId));
            addAction(actions, 'customizeboxartcta', Origin.client.games.customizeBoxArt.bind(this, offerId));
            addAction(actions, 'gamepropertiescta', Origin.client.games.modifyGameProperties.bind(this, offerId));
            addDivider();

            // Bottom separator
            //   bottomSeparator = addSeparator();
            //Section 3 - removal
            addAction(actions, 'removefromlibrarycta', Origin.client.games.removeFromLibrary.bind(this, offerId));
            addAction(actions, 'hidecta', GamesDataFactory.hideGame.bind(this, offerId));
            addAction(actions, 'unhidecta', GamesDataFactory.unhideGame.bind(this, offerId));
            addAction(actions, 'uninstallcta', Origin.client.games.uninstall.bind(this, offerId));

            //debugSeparator = addSeparator();
        }

        function initPrimaryActions(offerId) {
            ctaPrimaryActions = [];

            addAction(ctaPrimaryActions, 'primarybuynowcta', GamesDataFactory.buyNow.bind(this, offerId));

            addAction(ctaPrimaryActions, 'primaryrestorecta', Origin.client.games.restore.bind(this, offerId));

            addAction(ctaPrimaryActions, 'playcta', Origin.client.games.play.bind(this, offerId));
            addAction(ctaPrimaryActions, 'starttrialcta', Origin.client.games.play.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primaryupdatecta', Origin.client.games.checkForUpdateAndInstall.bind(this, offerId)); //for when mandatory update exists

            addAction(ctaPrimaryActions, 'primarydownloadcta', Origin.client.games.startDownload.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primarydownloadnowcta', Origin.client.games.moveToTopOfQueue.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primarypreloadcta', Origin.client.games.startDownload.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primarypreloadnowcta', Origin.client.games.moveToTopOfQueue.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primaryaddtodownloadscta', Origin.client.games.startDownload.bind(this, offerId));

            addAction(ctaPrimaryActions, 'primarypausedownloadcta', Origin.client.games.pauseDownload.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primaryresumedownloadcta', Origin.client.games.resumeDownload.bind(this, offerId));

            addAction(ctaPrimaryActions, 'primarypauseupdatecta', Origin.client.games.pauseUpdate.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primaryresumeupdatecta', Origin.client.games.resumeUpdate.bind(this, offerId));

            addAction(ctaPrimaryActions, 'primarypauserepaircta', Origin.client.games.pauseRepair.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primaryresumerepaircta', Origin.client.games.resumeRepair.bind(this, offerId));

            addAction(ctaPrimaryActions, 'primaryinstallcta', Origin.client.games.install.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primarypauseinstallcta', Origin.client.games.pauseInstall.bind(this, offerId));
            addAction(ctaPrimaryActions, 'primaryresumeinstallcta', Origin.client.games.resumeInstall.bind(this, offerId));

            addAction(ctaPrimaryActions, 'primaryphasedisplaycta', null);
        }

        function findAction(actionsList, label) {
            for (var i = 0; i < actionsList.length; i++) {
                if (actionsList[i].label === label) {
                    return i;
                }
            }
            return -1;
        }

        function makeVisible(label, enabled) {
            var ndx = findAction(actions, label);
            if (ndx !== -1) {
                actions[ndx].visible = true;
                actions[ndx].enabled = enabled;
            }
        }

        function getAction(label) {
            var ndx = findAction(actions, label);
            if (ndx === -1) { //doesn't exist
                return {};
            } else {
                return actions[ndx];
            }
        }

        function getPrimaryAction(label) {
            var ndx = findAction(ctaPrimaryActions, label);
            if (ndx === -1) { //doesn't exist
                return {};
            } else {
                return ctaPrimaryActions[ndx];
            }
        }

        return {

            /**
             * Returns the table of context actions
             * @param {string} offerId
             * @return {object} a list of ordered context action objects
             * @method actionsList
             */
            actionsList: function() {
                return actions;
            },

            /**
             * initializes the table of context actions
             * @param {string} offerId
             * @method initActions
             */
            initActions: initActions,

            /**
             * mark a context obj as visible
             * @param {string} label - context action
             * @param {boolean} enabled - whether the action is enabled or not
             * @method makeVisible
             */
            makeVisible: makeVisible,

            /**
             * returns context object associated with the label
             * @param {string} label - requested context action object
             * @return {Object} context action object
             * @method makeVisible
             */
            getAction: getAction,

            /**
             * initializes the table of primary actions
             * @param {string} offerId
             * @method initPrimaryActions
             */
            initPrimaryActions: initPrimaryActions,

            /**
             * returns the primary context object associated with the label
             * @param {string} offerId
             * @return {Object} primary Action object
             * @method initPrimaryActions
             */
            getPrimaryAction: getPrimaryAction


        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesContextActionsFactorySingleton(GamesDataFactory, SingletonRegistryFactory) {
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