/**
 * @file game/gamescontext.js
 */
(function() {
    'use strict';

    function GamesContextFactory(GamesDataFactory, GamesContextActionsFactory, ComponentsLogFactory) {
        var ctxMenu = [],
            makeVisible = GamesContextActionsFactory.makeVisible;


        function isBrowserGame(currentCatalog) {
            var packageType = Origin.utils.getProperty(currentCatalog, ['platforms', Origin.utils.os(), 'packageType']);
            return (packageType === 'ExternalURL');
        }

        function cloudSyncInProgress(clientActions) {
            if (clientActions) {
                return clientActions.cloudSaves && clientActions.cloudSaves.syncOperation;
            } else {
                return false;
            }
        }

        function isBaseGame(currentCatalog) {
            return (currentCatalog.originDisplayType === 'Full Game' || currentCatalog.originDisplayType === 'Full Game Plus Expansion');
        }

        //MY: TODO - for now always return true
        function hasAchievements() {
            return true;
        }

        function makeOperationVisible(clientActions, operationLabel, nowAction) {

            var cancellable = true,
                pauseAction = 'pause' + operationLabel + 'cta',
                resumeAction = 'resume' + operationLabel + 'cta',
                cancelAction = 'cancel' + operationLabel + 'cta',
                indexInQueue = clientActions.queueIndex,
                phase = '';

            if (clientActions.progressInfo) {
                phase = clientActions.progressInfo.phase;

                // Only show resume/pause if it's the head of the download queue.
                if (indexInQueue === 0) {
                    if ((phase === 'PAUSED') || (phase === 'PAUSING')) {
                        makeVisible(resumeAction, clientActions.resumable);
                    } else {
                        makeVisible(pauseAction, clientActions.pausable);
                    }
                }
                cancellable = clientActions.cancellable;
            }

            // index in queue is valid (not -1) and isn't head (0)
            if (nowAction && indexInQueue > 0) {
                makeVisible(nowAction, true, clientActions.queueSkippingEnabled);
            }

            makeVisible(cancelAction, cancellable);
        }

        function handleInstalledState(clientActions, currentCatalog) {
            if (clientActions.updateSupported) {
                var enableMenuOption = !clientActions.playing && !cloudSyncInProgress(clientActions);
                if (clientActions.availableUpdateVersion !== null) {
                    makeVisible('updategamecta', enableMenuOption);

                } else {
                    makeVisible('checkforupdatecta', enableMenuOption);
                }
            }

            if (clientActions.repairSupported) {
                makeVisible('repaircta', !clientActions.playing && !cloudSyncInProgress(clientActions));
            }

            if (!isBrowserGame(currentCatalog)) {
                makeVisible('gamepropertiescta', !clientActions.playing);
            }
        }


        function checkOperationsState(clientActions, currentCatalog) {
            if (clientActions.updating) {
                makeOperationVisible(clientActions, 'update', 'updatecta');
                makeVisible('uninstall', false);

            } else if (clientActions.downloading) {
                makeOperationVisible(clientActions, 'download', 'downloadcta');

            } else if (clientActions.installing) {
                makeOperationVisible(clientActions, 'install');

                if (clientActions.repairSupported) {
                    makeVisible('repaircta', false);
                }

            } else if (clientActions.repairing) {
                makeOperationVisible(clientActions, 'repair', 'repairnowcta');
                makeVisible('uninstallcta', false);

            } else if (clientActions.downloadable) {
                makeVisible('startdownloadcta', clientActions.downloadable);

            } else if (clientActions.installed) {
                handleInstalledState(clientActions, currentCatalog);

            } else if (clientActions.installable) {
                makeVisible('installcta');

                if (clientActions.repairSupported) {
                    makeVisible('repaircta', true);
                }
            }
        }

        function checkPlayableState(clientActions, currentCatalog) {
            if (clientActions.inTrash) {
                makeVisible('restorecta', true);

            } else if (clientActions.playable && !cloudSyncInProgress(clientActions)) {
                makeVisible(playVerb(currentCatalog), !clientActions.playing);

            } else if (isBaseGame(currentCatalog) && clientActions.installed && !cloudSyncInProgress(clientActions)) {
                // It's normal for PDLC to not be playable once installed
                // For everything else show a greyed out play
                makeVisible(playVerb(currentCatalog), false);
            }
        }

        function checkShowGameDetailsState(currentCatalog, ownsGame) {
            var showGameDetails = false;

            //TODO filter out for Origin games
            //if (!nonOriginGame) {
            if (ownsGame && isBaseGame(currentCatalog)) {
                // Only get a game details option for owned games or games+expansions
                showGameDetails = true;
            }
            //} //!nonOriginGame

            if (showGameDetails) {
                makeVisible('showgamedetailscta', true);
            }
        }

        function checkBaseGameState(offerId, clientActions, ownsGame) {

            if (GamesDataFactory.isFavorite(offerId)) {
                makeVisible('removeasfavoritecta', Origin.client.isOnline());
            } else {
                makeVisible('addasfavoritecta', Origin.client.isOnline());
            }
            if (GamesDataFactory.nonOriginGame(offerId) || ownsGame) {
                makeVisible('customizeboxartcta', Origin.bridgeAvailable());
            }

            if (!clientActions.playing && GamesDataFactory.nonOriginGame(offerId)) {
                makeVisible('removefromlibrarycta', true);
            }

            if (GamesDataFactory.isHidden(offerId)) {
                makeVisible('unhidecta', true);
            } else {
                makeVisible('hidecta', true);
            }
        }


        /**
         * @typedef gameInfoObject
         * @type {object}
         * @property {string} offerId
         * @property {object} catalogInfo
         * @property {object} clientActions
         * @property {boolean} ownsGame
         */

        /**
         * given a gameInfoObject, it determines what actions should be visible in the context menu, updates the actions table in GamesContextActionsFactory
         * @param {gameInfoObject} gameInfo
         */

        function updateForGame(gameInfo) {

            var clientActions = gameInfo.clientActions,
                currentCatalog = gameInfo.catalogInfo,
                ownsGame = gameInfo.ownsGame;

            if (clientActions) {
                if (currentCatalog.trial) {
                    makeVisible('viewspecialoffercta', Origin.client.isOnline());

                } else if (!ownsGame && currentCatalog.purchasable) {
                    makeVisible('viewinstorecta', Origin.client.isOnline());
                }

                checkPlayableState(clientActions, currentCatalog);

                if (cloudSyncInProgress(clientActions)) {
                    makeVisible('cancelcloudsynccta', clientActions.clientActions.cloudSaves.syncOperation.cancellable);
                }

                checkOperationsState(clientActions, currentCatalog);
            }

            checkShowGameDetailsState(currentCatalog, ownsGame);

            if (hasAchievements(gameInfo.offerId)) {
                makeVisible('showachievementscta', Origin.client.isOnline());
            }

            if (isBaseGame(currentCatalog)) {
                checkBaseGameState(gameInfo.offerId, clientActions, ownsGame);
            }

            if (clientActions.uninstallable) {
                makeVisible('uninstallcta', true);
            }
        }

        function populateMenu() {

            var actionList = GamesContextActionsFactory.actionsList();

            ctxMenu = [];

            for (var i = 0; i < actionList.length; i++) {
                var action = actionList[i];
                if (action.visible) {
                    ctxMenu.push(action);
                }
            }
            return ctxMenu;
        }

        function setContextForGame(gameInfo) {
            GamesContextActionsFactory.initActions(gameInfo.offerId);
            updateForGame(gameInfo);
        }

        function downloadVerb(clientActions) {

            var index = clientActions.queueIndex,
                isPreload = clientActions.releaseStatus === 'PRELOAD',
                label = '';

            if (clientActions.queueSize === 0) {
                label = isPreload ? 'primarypreloadcta' : 'primarydownloadcta';
            } else {
                switch (index) {
                    case -1:
                        label = 'primaryaddtodownloadscta';
                        break;

                    default:
                        label = isPreload ? 'primarypreloadnowcta' : 'primarydownloadnowcta';
                        break;
                }
            }
            return label;
        }

        function playVerb(currentCatalog) {
            var terminationDateValid = false;
            var ent = GamesDataFactory.getEntitlement(currentCatalog.offerId);
            if (ent && ent.terminationDate) {
                terminationDateValid = true;
            }

            // Only show "Start Trial" CTA if trial has not already been started
            if (currentCatalog.limitedTrial && !terminationDateValid) {
                return ('starttrialcta');
            }

            return ('playcta');
        }

        function getOperationLabel(clientActions) {
            var opLabel = '';

            //order is important
            if (clientActions.updating) {
                opLabel = 'update';
            } else if (clientActions.downloading) {
                opLabel = 'download';
            } else if (clientActions.installing) {
                opLabel = 'install';
            } else if (clientActions.repairing) {
                opLabel = 'repair';
            }
            return opLabel;
        }

        function shouldShowPhaseDisplay(clientActions) {
            var progressState = '';

            if (clientActions && clientActions.progressInfo) {
                progressState = clientActions.progressInfo.progressState;

                if (progressState === 'State-Indeterminate' || (progressState === 'State-Paused' && clientActions.progressInfo.phase === 'PAUSING')) {
                    return true;
                }
            }
            return false;
        }


        function getCurrentOperationLabel(clientActions) {
            var label = '',
                opLabel = '',
                progressState = '',
                labelObj = {},
                enabled = true;

            if (clientActions.progressInfo) {
                //get the current progress state to determine whether we should show the main action, e.g. "Pause Update"
                //or if we should show intermediary state like "Verifying files..."
                //to show "Pausing...", state is not Indeterminate but Paused, so we need to also check against phase because
                //we want to fall into the else if state = PAUSED but phase = PAUSED
                progressState = clientActions.progressInfo.progressState;

                if (shouldShowPhaseDisplay(clientActions)) {
                    label = 'primaryphasedisplaycta';
                } else {
                    opLabel = getOperationLabel(clientActions);

                    // Only show resume/pause if it's the head of the download queue.
                    if (clientActions.queueIndex === 0) {
                        if (clientActions.pausable) {
                            label = 'primarypause' + opLabel + 'cta';
                        } else if (clientActions.resumable) {
                            label = 'primaryresume' + opLabel + 'cta';
                        } else {
                            label = 'primaryphasedisplaycta';
                        }
                    } else if (clientActions.queueIndex > 0) {
                        label = 'primary' + opLabel + 'nowcta';
                        enabled = clientActions.queueSkippingEnabled;
                    }
                }
            }
            labelObj.label = label;
            labelObj.enabled = enabled;
            return labelObj;
        }

        function getPhaseDisplay(clientActions) {
            var label = 'NO PHASE';
            if (clientActions.progressInfo) {
                label = clientActions.progressInfo.phaseDisplay;
            }
            return label;
        }

        function canShowPlayable(clientActions) {
            var setPlayable = clientActions.playable;

            //ok, so it's playable, but check and see if it's updating, repairing, installing
            if (setPlayable) {
                //we need to check against progressInfo state too because it may not have officially started updating/repairing/installing
                //but we DO want to show things like "verifying files..."
                if (shouldShowPhaseDisplay(clientActions)) {
                    setPlayable = false;

                    //we omit downloading because if it's playable & downloading, that would mean that it's progressive install, so we want to show "Play"
                } else if ((clientActions.updating && clientActions.updateIsMandatory) || clientActions.repairing || clientActions.installing) {
                    setPlayable = false;
                }
            }

            return setPlayable;
        }

        function getPrimaryContext(gameInfo) {
            var contextObj = {},
                clientActions = gameInfo.clientActions,
                currentCatalog = gameInfo.catalogInfo,
                ownsGame = gameInfo.ownsGame,
                primaryLabel = '',
                primaryLabelObj = {},
                enabled = true;

            GamesContextActionsFactory.initPrimaryActions(gameInfo.offerId);

            //we may not have received it yet
            if (clientActions) {
                if (!ownsGame && currentCatalog.purchasable) {
                    primaryLabel = 'primarybuynowcta';

                    // MY: TODO
                    /* need implementation for testing for trial & expiration
                } else if trial and expired {

*/
                } else if (clientActions.inTrash) {
                    primaryLabel = 'primaryrestorecta';
                    //if mandatory update, then show primary action for the updating process, e.g. pause or resume
                    //if update is optional, then show "Play" as the primary action
                } else if (canShowPlayable(clientActions)) {
                    //if there is a mandatory update, then show "Update", but not if we're progressive-downloading
                    if (clientActions.updateAvailable && !clientActions.uninstalling && !clientActions.downloading && clientActions.updateIsMandatory) {
                        primaryLabel = 'primaryupdatecta';
                    } else {
                        primaryLabel = playVerb(currentCatalog);
                        enabled = !clientActions.uninstalling;
                    }
                } else if (clientActions.progressInfo && clientActions.progressInfo.active) {
                    primaryLabelObj = getCurrentOperationLabel(clientActions);
                    primaryLabel = primaryLabelObj.label;
                    enabled = primaryLabelObj.enabled;
                } else if (clientActions.installed) {
                    primaryLabel = playVerb(currentCatalog);
                    enabled = false;
                } else if (clientActions.downloadable) {
                    primaryLabel = downloadVerb(clientActions);
                } else if (clientActions.installable) {
                    primaryLabel = 'primaryinstallcta';

                    //MY: TODO - need to deal with extra content
                    /*
                } else if extracontent {

*/
                }

                if (primaryLabel.length > 0) {
                    contextObj = GamesContextActionsFactory.getPrimaryAction(primaryLabel);
                    if (contextObj.label === 'primaryphasedisplaycta') {
                        contextObj.phaseDisplay = getPhaseDisplay(clientActions);
                    }
                    contextObj.enabled = enabled;
                }
            }

            return contextObj;
        }

        // return an object with the offer related info to pass down to subsequent functions
        function getClientActions(offerId) {
            return function(response) {
                return {
                    offerId: offerId,
                    catalogInfo: response[offerId],
                    clientActions: GamesDataFactory.getClientGame(offerId),
                    ownsGame: GamesDataFactory.ownsEntitlement(offerId),
                };
            };
        }

        function getCatalogInfo(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId]);
        }

        function contextMenu(offerId) {
            return getCatalogInfo(offerId)
                .then(getClientActions(offerId))
                .then(setContextForGame)
                .then(populateMenu)
                .catch(function(error) {

                    ComponentsLogFactory.log('GAMESCONTEXTFACTORY: unable to retrieve cataloginfo for:', offerId, ' error:', error);
                    populateMenu();
                });
        }

        function primaryAction(offerId) {
            return getCatalogInfo(offerId)
                .then(getClientActions(offerId))
                .then(getPrimaryContext)
                .catch(function(error) {
                    ComponentsLogFactory.log('GAMESCONTEXTFACTORY: unable to retrieve cataloginfo for:', offerId, ' error:', error);
                    return {};
                });
        }

        return {
            /**
             * @typedef contextMenuObject
             * @type {object}
             * @property {string} label - context action label
             * @property {function} callback - callback function
             * @property {boolean} visible
             * @property {boolean} enabled
             */

            /**
             * given an offerid, returns the contextmenu object associated with that offer
             * @param {string} offerId
             * @return {promise} <contextMenuObject>[]
             */
            contextMenu: contextMenu,

            /**
             * given an offerid, returns the primary action for that offer
             * @param {string} offerId
             * @return {promise} contextMenuObject
             */

            primaryAction: primaryAction

        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesContextFactorySingleton(GamesDataFactory, GamesContextActionsFactory, ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesContextFactory', GamesContextFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesContextFactory

     * @description
     *
     * GamesContextFactory
     */
    angular.module('origin-components')
        .factory('GamesContextFactory', GamesContextFactorySingleton);
}());