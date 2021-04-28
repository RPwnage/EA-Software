/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to games with the C++ client
 */

define([
    'modules/client/clientobjectregistry'
], function(clientobjectregistry) {
    /**
     * Contains client games communication methods
     * @module module:games
     * @memberof module:Origin.module:client
     *
     */

    var clientGameWrapper = null,
        clientObjectName = 'OriginGamesManager';

    /**
     * This function takes in an action name and returns a function with a product id
     * @param  {string} actionName the name that corresponds with the C++ function
     */
    function createSendGameActionToClientFunction(actionName) {
        return function(prodId) {
            return clientGameWrapper.sendToOriginClient('onRemoteGameAction', [prodId, actionName, {}]);
        };
    }

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientGameWrapper = clientObjectWrapper;
        if (clientGameWrapper.clientObject) {
            clientGameWrapper.connectClientSignalToJSSDKEvent('changed', 'CLIENT_GAMES_CHANGED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('progressChanged', 'CLIENT_GAMES_PROGRESSCHANGED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('operationFailed', 'CLIENT_GAMES_OPERATIONFAILED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('basegamesupdated', 'CLIENT_GAMES_BASEGAMESUPDATED');
            clientGameWrapper.connectClientSignalToJSSDKEvent('downloadQueueChanged', 'CLIENT_GAMES_DOWNLOADQUEUECHANGED');
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
        startDownload: createSendGameActionToClientFunction('startDownload'),
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
        play: createSendGameActionToClientFunction('play'),
        /**
         * is a game playing
         * @param
         * @static
         * @method
         */
        isGamePlaying: function() {
            return clientGameWrapper.propertyFromOriginClient('isGamePlaying');
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
        uninstall: createSendGameActionToClientFunction('uninstall'),
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
        installParent: createSendGameActionToClientFunction('installParent')
    };
});
