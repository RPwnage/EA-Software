/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to games with the C++ client
 */

define([
    'modules/client/clientobjectregistry',
    'modules/client/communication',
    'modules/client/remote',
    'core/telemetry',
    'core/events'
], function(clientobjectregistry, communication, remote, telemetry, events) {
    /**
     * Contains client games communication methods
     * @module module:games
     * @memberof module:Origin.module:client
     *
     */

    var clientGameWrapper = null,
        clientObjectName = 'OriginGamesManager',
        retrieveSignalName = 'consolidatedEntitlementsFinished',
        retrieveConsolidatedEntitlementsPromise = null;

    /**
     * This function takes in an action name and returns a function with a product id
     * @param  {string} actionName the name that corresponds with the C++ function
     */
    function createSendGameActionToClientFunction(actionName) {
        return function(prodId) {
            telemetry.sendClientAction(actionName, prodId);
            return clientGameWrapper.sendToOriginClient('onRemoteGameAction', [prodId, actionName, {}]);
        };
    }

    /**
    * This method wraps game action calls in an RTP layer which will either
    * execute the action if we are in the client or will trigger an RTP call
    * @param {String} action - the game action to send to the client
    */
    function rtpGameLaunchWrapper(action) {
        if (communication.isEmbeddedBrowser()) {
            return createSendGameActionToClientFunction(action);
        } else {
            //this triggers the RTP call to the client with autoDownload set to true. This call handles the
            //case where the user doesn't have the entitlement
            return function(productId) {
                telemetry.sendClientAction(action, productId);
                return remote.gameLaunch(productId, true);
            };
        }
    }

    function handleConsolidatedGamesResponseFromClient(results) {
        events.fire('consolidated-response', results);
    }


    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientGameWrapper = clientObjectWrapper;
        if (clientGameWrapper.clientObject) {
            clientGameWrapper.connectClientSignalToJSSDKEvent('changed', 'CLIENT_GAMES_CHANGED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('cloudUsageChanged', 'CLIENT_GAMES_CLOUD_USAGE_CHANGED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('progressChanged', 'CLIENT_GAMES_PROGRESSCHANGED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('operationFailed', 'CLIENT_GAMES_OPERATIONFAILED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('playTimeChanged', 'CLIENT_GAMES_PLAYTIMECHANGED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('basegamesupdated', 'CLIENT_GAMES_BASEGAMESUPDATED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('downloadQueueChanged', 'CLIENT_GAMES_DOWNLOADQUEUECHANGED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('nogUpdated', 'CLIENT_GAMES_NOGUPDATED');
            clientGameWrapper.clientObject[retrieveSignalName].connect(handleConsolidatedGamesResponseFromClient);
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);


    return /** @lends module:Origin.module:client.module:games */ {
        /**
         * retrieves the current state of the games of the client
         * @return {promise<GameStateInfo>}
         * @static
         */
        requestGamesStatus: function() {
            return clientGameWrapper.sendToOriginClient('requestGamesStatus', arguments);
        },

        retrieveConsolidatedEntitlements: function(endPoint, forceRetrieve) {
            if (!retrieveConsolidatedEntitlementsPromise) {
                retrieveConsolidatedEntitlementsPromise = new Promise(function(resolve, reject) {
                    events.once('consolidated-response', function(results) {
                        retrieveConsolidatedEntitlementsPromise = null;
                        if ((Object.keys(results).length === 2) && (results.constructor === Array)) {
                            var data = JSON.parse(results[0]),
                                headers = results[1];
                            resolve({
                                headers: headers,
                                data: data
                            });
                        } else {
                            reject(new Error());
                        }
                    });
                    // fire off request to client
                    clientGameWrapper.sendToOriginClient('retrieveConsolidatedEntitlements', [endPoint, forceRetrieve || 'false']);
                });

            }

            return retrieveConsolidatedEntitlementsPromise;
        },
        /**
         * The data structure used by the Origin enbedded client to represent custom box art.
         *
         * @typedef {Object} CustomBoxArtInfo
         * @type {Object}
         *
         * @property {URL} customizedBoxart - The custom box art, encoded as a URL.
         * @property {number} cropCenterX - The horizontal center of the cropped box art.
         * @property {number} cropCenterY - The vertical center of the cropped box art.
         * @property {boolean} croppedBoxart - Indicates whether or not the box art is cropped.
         * @property {number} cropWidth - The cropped width of the custom box art.
         * @property {number} cropHeight - The cropped height of the custom box art.
         * @property {number} cropLeft - The left offset of the cropped custom box art.
         * @property {number} cropTop - The top offset of the cropped custom box art.
         */

        /**
         * Returns custom box art, if any.
         *
         * @param {string} productId - The Product ID of the game.
         * @returns {Promise<CustomBoxArtInfo>} The {@link CustomBoxArtInfo} configured in the embedded client.
         * @static
         * @method
         */
        getCustomBoxArtInfo: createSendGameActionToClientFunction('getCustomBoxArtInfo'),

        ///////////////////////////////////
        /// Downloading
        //////////////////////////////////

        /**
         * start a download of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        startDownload: rtpGameLaunchWrapper('startDownload'),

        /**
         * cancel the download of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        cancelDownload: createSendGameActionToClientFunction('cancelDownload'),
        /**
         * pause the download of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        pauseDownload: createSendGameActionToClientFunction('pauseDownload'),
        /**
         * resume the download of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        resumeDownload: createSendGameActionToClientFunction('resumeDownload'),

        ///////////////////////////////////
        /// Updating
        //////////////////////////////////
        /**
         * force check if there is an update available
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        checkForUpdateAndInstall: createSendGameActionToClientFunction('checkForUpdateAndInstall'),
        /**
         * install the update
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        installUpdate: createSendGameActionToClientFunction('installUpdate'),
        /**
         * pause the update operation
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        pauseUpdate: createSendGameActionToClientFunction('pauseUpdate'),
        /**
         * resume the update operation
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        resumeUpdate: createSendGameActionToClientFunction('resumeUpdate'),
        /**
         * cancel the update operation
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        cancelUpdate: createSendGameActionToClientFunction('cancelUpdate'),

        ///////////////////////////////////
        /// Repairing
        //////////////////////////////////
        /**
         * repair a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        repair: createSendGameActionToClientFunction('repair'),
        /**
         * cancel the repair of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        cancelRepair: createSendGameActionToClientFunction('cancelRepair'),
        /**
         * pause the repair a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        pauseRepair: createSendGameActionToClientFunction('pauseRepair'),
        /**
         * resume the repair a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        resumeRepair: createSendGameActionToClientFunction('resumeRepair'),


        ///////////////////////////////////
        /// Cloud syncing
        //////////////////////////////////
        /**
         * cancel the cloud save sync operation
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        cancelCloudSync: createSendGameActionToClientFunction('cancelCloudSync'),
        /**
         * fetches the size of the data currently stored on the cloud
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        pollCurrentCloudUsage: createSendGameActionToClientFunction('pollCurrentCloudUsage'),
        /**
         * if user downloaded a cloud save accidentally, this operation will rollback to the most recent on-disk save
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        restoreLastCloudBackup: createSendGameActionToClientFunction('restoreLastCloudBackup'),

        ///////////////////////////////////
        /// Non Origin Game
        //////////////////////////////////
        /**
         * remove the non-Origin game from the library
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        removeFromLibrary: createSendGameActionToClientFunction('removeFromLibrary'),

        /**
         * retrieves all non-Origin games from the client, as a single JSON array.
         * @returns {Promise<NOGInfo>}
         * @static
         * @method
         */
        getNonOriginGames: function() {
          return clientGameWrapper.sendToOriginClient('getNonOriginGames', arguments);
        },

        ///////////////////////////////////
        /// Playing
        //////////////////////////////////
        /**
         * play a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        play: rtpGameLaunchWrapper('play'),

        /**
         * is a game playing
         * @param
         * @static
         * @method
         */
        isGamePlaying: function() {
            if (communication.isEmbeddedBrowser()) {
                return clientGameWrapper.sendToOriginClient('isGamePlaying', arguments);
            } else {
                return Promise.resolve(false);
            }
        },
        ///////////////////////////////////
        /// Installing
        //////////////////////////////////
        /**
         * install a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        install: createSendGameActionToClientFunction('install'),
        /**
         * cancel the install of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        cancelInstall: createSendGameActionToClientFunction('cancelInstall'),
        /**
         * pause the install of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        pauseInstall: createSendGameActionToClientFunction('pauseInstall'),
        /**
         * resume the install of a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        resumeInstall: createSendGameActionToClientFunction('resumeInstall'),
        /**
         * customize box art of the game, calling this function will bring up a dialog to start the process
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        customizeBoxArt: createSendGameActionToClientFunction('customizeBoxArt'),
        /**
         * uninstall a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        uninstall: function(prodId, silent) {
            return clientGameWrapper.sendToOriginClient('onRemoteGameAction', [prodId, 'uninstall', {silent: silent ? silent : false}]);
        },
        /**
         * restore a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        restore: createSendGameActionToClientFunction('restore'),
        /**
         * move the game to the top of the download queue
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        moveToTopOfQueue: createSendGameActionToClientFunction('moveToTopOfQueue'),
        /**
         * install the parent offer for a DLC
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        installParent: createSendGameActionToClientFunction('installParent'),
        /**
         * retrieve the multi-launch options for a game
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        getMultiLaunchOptions: createSendGameActionToClientFunction('getMultiLaunchOptions'),

        /**
         * update the multi-launch options for a game to settings
         * @param  {string} productId the product id of the game
         * @param  {string} launcher title user selected
         * @static
         * @method
         */
        updateMultiLaunchOptions: function(prodId, launcher) {
            return clientGameWrapper.sendToOriginClient('onRemoteGameAction', [prodId, 'updateMultiLaunchOptions', {launcher: launcher}]);
        },

        /**
         * retrieve the available locales for a game
         * @param  {string} productId the product id of the game
         * @static
         * @method getAvailableLocales
         */
        getAvailableLocales: createSendGameActionToClientFunction('getAvailableLocales'),
        /**
         * retrieve the executable path for a non-origin game
         * @param  {string} productId the product id of the game
         * @static
         * @method getAvailableLocales
         */
        getNOGExecutablePath: createSendGameActionToClientFunction('getNOGExecutablePath'),
        /**
         * Launch the game in ODT
         * @param  {string} productId the product id of the game
         * @static
         * @method
         */
        openInODT: createSendGameActionToClientFunction('openInODT'),
        /**
         * update the installed locale for a game
         * @param  {string} productId the product id of the game
         * @param  {string} newLocale the new locale of the game installation
         * @static
         * @method updateInstalledLocale
         */
        updateInstalledLocale: function(prodId, newLocale) {
            return clientGameWrapper.sendToOriginClient('onRemoteGameAction', [prodId, 'updateInstalledLocale', {newLocale: newLocale}]);
        },

        /**
         * update the installed locale for a game
         * @param  {string} productId the product id of the game
         * @param  {string} newLocale the new locale of the game installation
         * @static
         * @method updateInstalledLocale
         */
        updateNOGGameTitle: function(prodId, gameTitle) {
            return clientGameWrapper.sendToOriginClient('onRemoteGameAction', [prodId, 'updateNOGGameTitle', {gameTitle: gameTitle}]);
        },

        /**
         * show dialog to bring up product code redemption
         * @static
         * @method redeemProductCode
         */
        redeemProductCode: function() {
            return clientGameWrapper.sendToOriginClient('redeemProductCode', arguments);
        },

        /**
         * bring up file folder navigation to select non-origin game to add
         * @static
         * @method addNonOriginGame
         */
        addNonOriginGame: function() {
            return clientGameWrapper.sendToOriginClient('addNonOriginGame', arguments);
        }
    };
});
