/**
 * @file game/gamescontext.js
 */
(function() {
    'use strict';

    function GamesContextFactory(GamesDataFactory, GamesContextActionsFactory, GamesCTAActionsModel, BeaconFactory, ComponentsLogFactory, GamesTrialFactory, GameRefiner, AchievementFactory, SubscriptionFactory, OwnershipFactory) {
        var ctxMenu = [],
            personaId = Origin.user.personaId(),
            installed, // true if the Origin client is installed
            installableOnPlatform, // true if Origin is installable on this platform
            VIEW_IN_STORE_CTA = 'viewinstorecta';

        /**
        * Set the installed and installableOnPlatform to the passed value
        * @param {Mixed} response
        * @return {Function}
        */
        function setInstallInfo(response) {
            /**
             * Set local state variables from service response
             * @param  {Boolean} isInstalled true if Origin is installed
             * @param  {Boolean} isInstallable true if Origin is installable on the user's platform
             * @return {Mixed} pass through the original response to the next step in the pipeline
             */
            return function(isInstalled, isInstallable) {
                installed = isInstalled;
                installableOnPlatform = isInstallable;
                return response;
            };
        }

        /**
        * Add isAwaitingTrialActivation to the gameInfo
        * @param {object} gameInfo
        * @return {Function}
        */
        function setIsAwaitingTrialActivation(gameInfo) {
            /**
             * @param  {Boolean} awaitingActivation - true if context game is a trial awaiting activation
             * @return {object} gameInfo - updated gameInfo gets sent to the next step in the pipeline
             */
            return function(awaitingActivation) {
                gameInfo.isAwaitingTrialActivation = awaitingActivation;
                return gameInfo;
            };
        }

        /**
        * Add isTrialExpired to the gameInfo
        * @param {object} gameInfo
        * @return {Function}
        */
        function setIsTrialExpired(gameInfo) {
            /**
             * @param  {Boolean} expired - true if context game is an expired trial
             * @return {object} gameInfo - updated gameInfo gets sent to the next step in the pipeline
             */
            return function(expired) {
                gameInfo.isTrialExpired = expired;
                return gameInfo;
            };
        }

        /**
        * Determine if the client is installable on the current platform
        * @param {object} response - the response to chain to the next item in the promise chain
        * @return {Promise}
        */
        function getAndSetBeacon(response) {
            return Promise.all([
                BeaconFactory.isInstalled(),
                BeaconFactory.isInstallable()
            ])
            .then(_.spread(setInstallInfo(response)));
        }

        /**
        * Determine if the selected game is awaiting activation
        * @param {object} gameInfo
        * @return {Promise}
        */
        function getIsAwaitingTrialActivation(gameInfo) {
            return GamesTrialFactory.isTrialAwaitingActivation(gameInfo.offerId)
                .then(setIsAwaitingTrialActivation(gameInfo));
        }

        /**
        * Determine if the selected game is an expired trial
        * @param {object} gameInfo
        * @return {Promise}
        */
        function getIsTrialExpired(gameInfo) {
            return GamesTrialFactory.isTrialExpired(gameInfo.offerId)
                .then(setIsTrialExpired(gameInfo));
        }

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

        /**
         * Mark a context obj as visible
         * @param {array} menu - array of context menu action objects that represents the context menu
         * @param {string} label - context action name
         * @param {boolean} enabled - whether the action should be enabled or not
         */
        function makeVisible(menu, label, enabled) {
            var action = _.find(menu, {'label': label}) || {};
            if (!_.isEmpty(action)) {
                action.visible = true;
                action.enabled = enabled;
            }
        }

        /**
         * Add (make visible) a context menu action to the menu
         * @param {array} menu - array of context menu action objects that represents the context menu
         * @param {object} clientActions - object from gamesDataFactory describing the game's state and available operations
         * @param {string} operationLabel - name of the context menu option to add (make visible) to the context menu
         * @param {string} nowAction
         */
        function makeOperationVisible(menu, clientActions, operationLabel, nowAction) {

            var cancellable = true,
                pauseAction = 'pause' + operationLabel + 'cta',
                resumeAction = 'resume' + operationLabel + 'cta',
                cancelAction = 'cancel' + operationLabel + 'cta',
                moveToTopOfQueueAction = operationLabel + 'nowcta',
                indexInQueue = clientActions.queueIndex,
                phase = '';

            if (clientActions.progressInfo) {
                phase = clientActions.progressInfo.phase;

                // Only show resume/pause if it's the head of the download queue.
                if (indexInQueue === 0) {
                    if ((phase === 'PAUSED') || (phase === 'PAUSING')) {
                        makeVisible(menu, resumeAction, clientActions.resumable);
                    } else {
                        makeVisible(menu, pauseAction, clientActions.pausable);
                    }
                }
                cancellable = clientActions.cancellable;
            }

            // index in queue is valid (not -1) and isn't head (0)
            if (nowAction && indexInQueue > 0) {
                makeVisible(menu, moveToTopOfQueueAction, clientActions.queueSkippingEnabled);
            }

            makeVisible(menu, cancelAction, cancellable);
        }

        /**
         * Handle menu options related to game update/repair
         * @param {array} menu - array of context menu action objects that represents the context menu
         * @param {object} clientActions - object from gamesDataFactory describing the game's state and available operations
         */
        function handleInstalledState(menu, clientActions) {
            if (clientActions.updateSupported) {
                var enableMenuOption = !clientActions.playing && !cloudSyncInProgress(clientActions);
                if (clientActions.availableUpdateVersion !== null) {
                    makeVisible(menu, 'updategamecta', enableMenuOption);

                } else {
                    makeVisible(menu, 'checkforupdatecta', enableMenuOption);
                }
            }

            if (clientActions.repairSupported) {
                makeVisible(menu, 'repaircta', !clientActions.playing && !cloudSyncInProgress(clientActions));
            }
        }

        /**
         * Handle menu options related to game state
         * @param {array} menu - array of context menu action objects that represents the context menu
         * @param {object} clientActions - object from gamesDataFactory describing the game's state and available operations
         */
        function checkOperationsState(menu, clientActions) {
            if (clientActions.updating) {
                makeOperationVisible(menu, clientActions, 'update', 'updatecta');
                makeVisible(menu, 'uninstall', false);

            } else if (clientActions.downloading) {
                if (clientActions.releaseStatus === 'PRELOAD') {
                    makeOperationVisible(menu, clientActions, 'preload', 'preloadcta');
                } else {
                    makeOperationVisible(menu, clientActions, 'download', 'downloadcta');
                }

            } else if (clientActions.installing) {
                makeOperationVisible(menu, clientActions, 'install');

                if (clientActions.repairSupported) {
                    makeVisible(menu, 'repaircta', false);
                }

            } else if (clientActions.repairing) {
                makeOperationVisible(menu, clientActions, 'repair', 'repairnowcta');
                makeVisible(menu, 'uninstallcta', false);

            } else if (clientActions.downloadable) {
                if (clientActions.releaseStatus === 'PRELOAD') {
                    makeVisible(menu, 'preloadcta', clientActions.downloadable && Origin.client.onlineStatus.isOnline());
                } else {
                    makeVisible(menu, 'startdownloadcta', clientActions.downloadable && Origin.client.onlineStatus.isOnline());
                }

            } else if (clientActions.installed) {
                handleInstalledState(menu, clientActions);

            } else if (clientActions.installable) {
                makeVisible(menu, 'installcta');

                if (clientActions.repairSupported) {
                    makeVisible(menu, 'repaircta', Origin.client.onlineStatus.isOnline());
                }
            }
        }

        /**
         * Handle menu options related to game playability
         * @param {array} menu - array of context menu action objects that represents the context menu
         * @param {object} clientActions - object from gamesDataFactory describing the game's state and available operations
         * @param {object} currentCatalog - game's catalog info
         * @param {boolean} isAwaitingTrialActivation
         */
        function checkPlayableState(menu, clientActions, currentCatalog, isAwaitingTrialActivation) {
            if (clientActions.inTrash) {
                makeVisible(menu, 'restorecta', true);

            } else if (clientActions.playable && !cloudSyncInProgress(clientActions)) {
                makeVisible(menu, playVerb(isAwaitingTrialActivation), !clientActions.playing);

            } else if (GameRefiner.isBaseGame(currentCatalog) && clientActions.installed && !cloudSyncInProgress(clientActions)) {
                // It's normal for PDLC to not be playable once installed
                // For everything else show a greyed out play
                makeVisible(menu, playVerb(isAwaitingTrialActivation), false);
            }
        }

        /**
         * Handle the "show game details" menu option
         * @param {array} menu - array of context menu action objects that represents the context menu
         * @param {object} currentCatalog - game's catalog info
         * @param {boolean} ownsGame
         */
        function checkShowGameDetailsState(menu, currentCatalog, ownsGame) {
            var showGameDetails = false;

            var isNonOriginGame = GamesDataFactory.isNonOriginGame(currentCatalog.offerId);
            if ( isNonOriginGame || (ownsGame && GameRefiner.isBaseGame(currentCatalog))) {
                // Only get a game details option for owned games or games+expansions or non-Origin game
                showGameDetails = true;
            }

            if (showGameDetails) {
                makeVisible(menu, 'showgamedetailscta', true);
            }
        }

        /**
         * Handle menu options related to base game state
         * @param {array} menu - array of context menu action objects that represents the context menu
         * @param {string} offerId
         * @param {object} clientActions - object from gamesDataFactory describing the game's state and available operations
         */
        function checkBaseGameState(menu, offerId, clientActions) {

            if (GamesDataFactory.isFavorite(offerId)) {
                makeVisible(menu, 'removeasfavoritecta', Origin.client.onlineStatus.isOnline());
            } else {
                makeVisible(menu, 'addasfavoritecta', Origin.client.onlineStatus.isOnline());
            }

            if (GamesDataFactory.isNonOriginGame(offerId) && !clientActions.playing) {
                makeVisible(menu, 'removefromlibrarycta', true);
            }
            else if (GamesDataFactory.isSubscriptionEntitlement(offerId)) {
                var visible = Origin.client.onlineStatus.isOnline() && !clientActions.installed;
                if (visible) {
                    makeVisible(menu, 'removefromlibrarycta', true);
                }
            }

            if (GamesDataFactory.isHidden(offerId)) {
                makeVisible(menu, 'unhidecta', Origin.client.onlineStatus.isOnline());
            } else {
                makeVisible(menu, 'hidecta', Origin.client.onlineStatus.isOnline());
            }
            if (Origin.client.settings.developerToolAvailable()) {
                makeVisible(menu, 'openinodtcta', true);
            }
        }

        function wrapCatalogInfo(catalogInfo) {
            var obj = {};
            obj[catalogInfo.offerId] = catalogInfo;
            return obj;
        }


        /**
         * @typedef gameInfoObject
         * @type {object}
         * @property {string} offerId
         * @property {object} catalogInfo
         * @property {object} clientActions - object from gamesDataFactory describing the game's state and available operations
         * @property {boolean} ownsGame
         * @property {boolean} isAwaitingTrialActivation
         */

        /**
         * Determine what actions should be visible in the context menu
         * @param {gameInfoObject} gameInfo
         * @param {array} menu - array of context menu action objects that represents the context menu
         */

        function processContextMenu(gameInfo, menu) {

            var clientActions = gameInfo.clientActions,
                currentCatalog = gameInfo.catalogInfo,
                ownsGame = gameInfo.ownsGame,
                isAwaitingTrialActivation = gameInfo.isAwaitingTrialActivation;

            if (clientActions) {
                // this should only be visible if it is not a NOG so the user has easy access to the PDP page for any game
                if (!GamesDataFactory.isNonOriginGame(gameInfo.offerId) && !Origin.user.underAge()) {
                    makeVisible(menu, VIEW_IN_STORE_CTA, Origin.client.onlineStatus.isOnline());
                }

                checkPlayableState(menu, clientActions, currentCatalog, isAwaitingTrialActivation);

                if (cloudSyncInProgress(clientActions)) {
                    makeVisible(menu, 'cancelcloudsynccta', clientActions.clientActions.cloudSaves.syncOperation.cancellable);
                }

                if (!isBrowserGame(currentCatalog) && Origin.client.isEmbeddedBrowser()) {
                    makeVisible(menu, 'gamepropertiescta', !clientActions.playing);
                }

                checkOperationsState(menu, clientActions);
            }

            checkShowGameDetailsState(menu, currentCatalog, ownsGame);

            if (GameRefiner.isBaseGame(currentCatalog)) {
                checkBaseGameState(menu, gameInfo.offerId, clientActions, ownsGame);
            }

            if (clientActions.uninstallable) {
                makeVisible(menu, 'uninstallcta', true);
            }

            var achievementSetId = _.head(AchievementFactory.determineOwnedAchievementSets(wrapCatalogInfo(currentCatalog)));
            if (achievementSetId) {
                AchievementFactory.getAchievementSet(personaId, achievementSetId)
                    .then(function(data) {
                        if (_.get(data, 'achievements')) {
                            makeVisible(menu, 'viewachievementscta', true);
                        }
                    })
                    .catch(function(error) {
                        ComponentsLogFactory.log('GAMESCONTEXTFACTORY: unable to retrieve achievement set', error);
                    });
            }
        }

        /**
         * Remove non-visible actions from the context menu and fix menu dividers
         * @param {array} menu - array of context menu action objects that represents the context menu
         */
        function filterMenu(menu) {
            ctxMenu = [];

            for (var i = 0; i < menu.length; i++) {
                var action = menu[i];
                if (action.visible) {
                    // Add this check because we don't want two dividers showing up next to each other
                    if (!!action.divider && (!!ctxMenu[ctxMenu.length-1] && !!ctxMenu[ctxMenu.length-1].divider)) {
                        continue;
                    }
                    ctxMenu.push(action);
                }
            }
            return ctxMenu;
        }

        function setupContextMenu(context) {
            return function(gameInfo) {
                var menu = GamesContextActionsFactory.baseActionsList(gameInfo.offerId, context);
                processContextMenu(gameInfo, menu);
                return menu;
            };
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

        function playVerb(isAwaitingTrialActivation) {
            // Only show "Start Trial" CTA if trial has not already been started
            if (isAwaitingTrialActivation) {
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
                if (clientActions.releaseStatus === 'PRELOAD') {
                    opLabel = 'preload';
                } else {
                    opLabel = 'download';
                }
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

        /**
         * Determine what the primary context action should be for this game
         * @param {boolean} isMoreDetailsDisabled
         */
        function getPrimaryContext(isMoreDetailsDisabled) {
            return function(gameInfo) {
                var contextObj = {},
                    clientActions = gameInfo.clientActions,
                    currentCatalog = gameInfo.catalogInfo,
                    ownsGame = gameInfo.catalogInfo.isOwned,
                    isAwaitingTrialActivation = gameInfo.isAwaitingTrialActivation,
                    primaryLabel = '',
                    primaryLabelObj = {},
                    enabled = true,
                    launchable = false,
                    isPurchasable = Origin.utils.getProperty(currentCatalog, ['countries', 'isPurchasable']),
                    releaseDate = Origin.utils.getProperty(currentCatalog, ['platforms', Origin.utils.os(), 'releaseDate']),
                    sunsetDate = Origin.utils.getProperty(currentCatalog, ['platforms', Origin.utils.os(), 'useEndDate']),
                    currentTime = new Date(Origin.datetime.getTrustedClock()),
                    primaryActionsList = GamesCTAActionsModel.primaryActionsList(gameInfo.offerId);

                //we may not have received it yet
                if (clientActions) {
                    if (!ownsGame && isPurchasable) {
                        if (releaseDate && currentTime > releaseDate) {
                            primaryLabel = 'primarybuynowcta';
                        } else {
                            primaryLabel = 'primarypreordercta';
                        }
                    } else if (currentCatalog.freeBaseGame && gameInfo.isTrialExpired) {
                        primaryLabel = VIEW_IN_STORE_CTA;
                    } else if (clientActions.inTrash) {
                        primaryLabel = 'primaryrestorecta';
                        //if mandatory update, then show primary action for the updating process, e.g. pause or resume
                        //if update is optional, then show "Play" as the primary action
                    } else if (canShowPlayable(clientActions)) {
                        //if there is a mandatory update, then show "Update", but not if we're progressive-downloading
                        if (clientActions.updateAvailable && !clientActions.uninstalling && !clientActions.downloading && clientActions.updateIsMandatory) {
                            primaryLabel = 'primaryupdatecta';
                        } else {
                            primaryLabel = playVerb(isAwaitingTrialActivation);
                            enabled = !clientActions.uninstalling;
                            launchable = clientActions.launchable;
                        }
                    } else if (clientActions.progressInfo && clientActions.progressInfo.active) {
                        primaryLabelObj = getCurrentOperationLabel(clientActions);
                        primaryLabel = primaryLabelObj.label;
                        enabled = primaryLabelObj.enabled;
                    } else if (clientActions.installed) {
                        if (sunsetDate && (currentTime > sunsetDate)) {
                            // Sunsetted games, called "disabled" by some producers.
                            // It is installed, current time is beyond the useEndDate
                            if (isMoreDetailsDisabled) {
                                primaryLabel = 'primaryuninstallcta';
                            } else {
                                primaryLabel = 'moredetailscta';
                            }
                        } else {
                            primaryLabel = playVerb(isAwaitingTrialActivation);
                            enabled = false;
                        }
                    } else if (sunsetDate && (currentTime > sunsetDate)) {
                        // Sunsetted but not installed.
                        primaryLabel = 'moredetailscta';

                    } else if (clientActions.downloadable ||
                            // Allow download action for extra content, even if clientActions shows as not downloadable
                            // Only allow download for extra content when client is running
                                (GameRefiner.isExtraContent(currentCatalog) && GameRefiner.isDownloadable(currentCatalog) && Origin.client.isEmbeddedBrowser())) {
                        primaryLabel = downloadVerb(clientActions);
                        enabled = Origin.client.onlineStatus.isOnline();
                    } else if (clientActions.installable) {
                        primaryLabel = 'primaryinstallcta';

                        //MY: TODO - need to deal with extra content
                        /*
                         } else if extracontent {

                         */
                    }
                    // Owned but client not available.
                    else if (ownsGame && !Origin.client.isEmbeddedBrowser()) {
                        if (installed) {
                            primaryLabel = 'playonorigincta';
                        }
                        else if (installableOnPlatform) {
                            // compatible platform
                            primaryLabel = 'getitwithorigincta';
                        } else {
                            // incompatible platform
                            primaryLabel = 'getitwithoriginctaincompatible';
                        }
                    }

                    if (primaryLabel.length > 0) {
                        contextObj = _.find(primaryActionsList, {'label': primaryLabel}) || {};
                        if (contextObj.label === 'primaryphasedisplaycta') {
                            contextObj.phaseDisplay = getPhaseDisplay(clientActions);
                        }
                        contextObj.enabled = enabled;
                        contextObj.launchable = launchable;
                    }
                }

                // pass along offerId for cta
                contextObj.offerId = gameInfo.offerId;

                return contextObj;
            };
        }

        // return an object with the offer related info to pass down to subsequent functions
        function getClientActions(offerId) {
            return function(response) {
                return {
                    offerId: offerId,
                    catalogInfo: response[offerId],
                    clientActions: GamesDataFactory.getClientGame(offerId),
                    ownsGame: GamesDataFactory.ownsEntitlement(offerId)
                };
            };
        }

        function getCatalogInfo(offerId) {
            return OwnershipFactory.getGameDataWithEntitlements([offerId]);
        }

        function contextMenu(offerId, context) {
            return getCatalogInfo(offerId)
                .then(getClientActions(offerId))
                .then(getIsAwaitingTrialActivation)
                .then(setupContextMenu(context))
                .then(filterMenu)
                .catch(function(error) {
                    ComponentsLogFactory.error('GAMESCONTEXTFACTORY: unable to retrieve cataloginfo for: ' + offerId, error);
                    return [];
                });
        }

        function primaryAction(offerId, isMoreDetailsDisabled) {
            return getCatalogInfo(offerId)
                .then(getAndSetBeacon)
                .then(getClientActions(offerId))
                .then(getIsAwaitingTrialActivation)
                .then(getIsTrialExpired)
                .then(getPrimaryContext(isMoreDetailsDisabled))
                .catch(function(error) {
                    ComponentsLogFactory.error('GAMESCONTEXTFACTORY: unable to retrieve cataloginfo for: ' + offerId, error);
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
             * @param {string} context
             * @return {promise} <contextMenuObject>[]
             */
            contextMenu: contextMenu,

            /**
             * given an offerid, returns the primary action for that offer
             * @param {string} offerId
             * @param {boolean} isMoreDetailsDisabled
             * @return {promise} contextMenuObject
             */
            primaryAction: primaryAction

        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesContextFactorySingleton(GamesDataFactory, GamesContextActionsFactory, GamesCTAActionsModel, BeaconFactory, ComponentsLogFactory, GamesTrialFactory, GameRefiner, AchievementFactory, SubscriptionFactory, OwnershipFactory, SingletonRegistryFactory) {
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