/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to games with the C++ client
 */

/** @namespace
 * @memberof Origin.client
 * @alias games
 */
define([
    'core/events',
    'core/logger',
    'modules/client/utils'
], function(events, logger, clientUtils) {
    var sendToOriginClient = clientUtils.sendToOriginClient;
    var listenForOriginClientMsg = clientUtils.listenForOriginClientMsg;
    var gamesMgrObj = 'OriginGamesManager';

    function createActionObj(actionName, prodId, params) {
        return {
            gameFn: actionName,
            prodId: prodId,
            params: params
        };
    }
    /**
     * This function takes in an action name and returns a function with a product id
     * @param  {string} actionName the name that corresponds with the C++ function
     */
    function createSendGameActionToClientFunction(actionName) {
        return function(prodId) {
            return sendToOriginClient(gamesMgrObj, 'onRemoteGameAction', createActionObj(actionName, prodId, {}));
        };
    }

    listenForOriginClientMsg(gamesMgrObj, 'changed', function(obj) {
        logger.log('client gameschanged - ', obj);
        events.fire(events.CLIENT_GAMES_CHANGED, [obj]);
    });

    listenForOriginClientMsg(gamesMgrObj, 'progressChanged', function(obj) {
        events.fire(events.CLIENT_GAMES_PROGRESSCHANGED, [obj]);
    });

    listenForOriginClientMsg(gamesMgrObj, 'basegamesupdated', function(obj) {
        logger.log('client basegamesupdated - ', obj);
        events.fire(events.CLIENT_GAMES_BASEGAMESUPDATED);
    });

    listenForOriginClientMsg(gamesMgrObj, 'downloadQueueChanged', function(obj) {
        logger.log('client downloadQueue - ', obj);
        events.fire(events.CLIENT_GAMES_DOWNLOADQUEUECHANGED, obj.data);
    });

    return {
        /**
         * retrieves the current state of the games of the client
         * @return {promise<GameStateInfo>}
         * @memberof Origin.client.games
         */
        requestStatus: function() {
            return sendToOriginClient(gamesMgrObj, 'requestGamesStatus', {});
        },

        ///////////////////////////////////
        /// Downloading
        //////////////////////////////////

        /**
         * start a download of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        startDownload: createSendGameActionToClientFunction('startDownload'),
        /**
         * cancel the download of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        cancelDownload: createSendGameActionToClientFunction('cancelDownload'),
        /**
         * pause the download of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        pauseDownload: createSendGameActionToClientFunction('pauseDownload'),
        /**
         * resume the download of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        resumeDownload: createSendGameActionToClientFunction('resumeDownload'),

        ///////////////////////////////////
        /// Updating
        //////////////////////////////////

        checkForUpdateAndInstall: createSendGameActionToClientFunction('checkForUpdateAndInstall'),
        installUpdate: createSendGameActionToClientFunction('installUpdate'),
        pauseUpdate: createSendGameActionToClientFunction('pauseUpdate'),
        resumeUpdate: createSendGameActionToClientFunction('resumeUpdate'),
        cancelUpdate: createSendGameActionToClientFunction('cancelUpdate'),

        ///////////////////////////////////
        /// Repairing
        //////////////////////////////////
        /**
         * repair a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        repair: createSendGameActionToClientFunction('repair'),
        /**
         * cancel the repair of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        cancelRepair: createSendGameActionToClientFunction('cancelRepair'),
        /**
         * pause the repair a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        pauseRepair: createSendGameActionToClientFunction('pauseRepair'),
        /**
         * resume the repair a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        resumeRepair: createSendGameActionToClientFunction('resumeRepair'),


        ///////////////////////////////////
        /// Cloud syncing
        //////////////////////////////////

        cancelCloudSync: createSendGameActionToClientFunction('cancelCloudSync'),

        ///////////////////////////////////
        /// Non Origin Game
        //////////////////////////////////

        removeFromLibrary: createSendGameActionToClientFunction('removeFromLibrary'),


        ///////////////////////////////////
        /// Playing
        //////////////////////////////////
        /**
         * play a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        play: createSendGameActionToClientFunction('play'),

        ///////////////////////////////////
        /// Installing
        //////////////////////////////////
        /**
         * install a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        install: createSendGameActionToClientFunction('install'),
        /**
         * cancel the install of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        cancelInstall: createSendGameActionToClientFunction('cancelInstall'),
        /**
         * pause the install of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        pauseInstall: createSendGameActionToClientFunction('pauseInstall'),
        /**
         * resume the install of a game
         * @param  {string} productId the product id of the game
         * @memberof Origin.client.games
         */
        resumeInstall: createSendGameActionToClientFunction('resumeInstall'),
        /**
         * set a game as favorite
         * @param  {string} productId the product id of the game
         * @param {boolean} isFavorite true if the game is favorite
         * @memberof Origin.client.games
         */


        customizeBoxArt: createSendGameActionToClientFunction('customizeBoxArt'),
        modifyProperties: createSendGameActionToClientFunction('modifyProperties'),
        uninstall: createSendGameActionToClientFunction('uninstall'),
        restore: createSendGameActionToClientFunction('restore'),
        modifyGameProperties: createSendGameActionToClientFunction('modifyGameProperties'),
        moveToTopOfQueue: createSendGameActionToClientFunction('moveToTopOfQueue'),
        installParent: createSendGameActionToClientFunction('installParent'),

        setFavorite: function(productId, isFavorite) {
            return sendToOriginClient(gamesMgrObj, 'onRemoteGameAction', createActionObj('setFavorite', productId, {
                favorite: isFavorite
            }));
        },
        /**
         * hide a game
         * @param  {string} productId the product id of the game
         * @param {boolean} isHidden true if the game is hidden
         * @memberof Origin.client.games
         */
        setHidden: function(productId, isHidden) {
            return sendToOriginClient(gamesMgrObj, 'onRemoteGameAction', createActionObj('setHidden', productId, {
                hidden: isHidden
            }));
        }
    };
});