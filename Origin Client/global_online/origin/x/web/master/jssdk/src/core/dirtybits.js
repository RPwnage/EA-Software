/*jshint strict: false */
define([
    'core/user',
    'core/urls',
    'core/logger',
    'core/events',
    'core/utils'
], function(user, urls, logger, events, utils) {

    var myEvents = new utils.Communicator(),
        dirtyBitsConnection = null,
        keepAliveData = new Uint8Array(),
        KEEP_CONNECTION_ALIVE_TIMEOUT = 55000,
        logPrefix = '[DIRTYBITS]',
        contextToJSSDKEventMap = {
            'ach': events.DIRTYBITS_ACHIEVEMENTS,
            'group': events.DIRTYBITS_GROUP,
            'email': events.DIRTYBITS_EMAIL,
            'password': events.DIRTYBITS_PASSWORD,
            'originid': events.DIRTYBITS_ORIGINID,
            'gamelib': events.DIRTYBITS_GAMELIB,
            'privacy': events.DIRTYBITS_PRIVACY,
            'avatar': events.DIRTYBITS_AVATAR,
            'entitlement': events.DIRTYBITS_ENTITLEMENT,
            'catalog': events.DIRTYBITS_CATALOG
        };

    /**
     * sends a dummy piece of data to keep the connection alive
     */
    function sendPong() {
        if (dirtyBitsConnection) {
            dirtyBitsConnection.send(keepAliveData);
        }
    }

    /**
     * instantiates a new websocket object and connects to the websocket server
     * @param  {function} completedCallback callback triggered when successfully connects
     * @param  {function} errorCallback     callback triggered when hits and error
     */
    function createNewWebSocketConnection() {
        var serverUrl = urls.endPoints.dirtyBitsServer.replace('{userPid}', user.publicObjs.userPid()).replace('{accessToken}', user.publicObjs.accessToken()),
            intervalID = null;
        dirtyBitsConnection = new WebSocket(serverUrl);

        dirtyBitsConnection.onmessage = function(dirtyBitEvent) {
            var dirtyBitObject = JSON.parse(dirtyBitEvent.data);

            var jssdkEvent = contextToJSSDKEventMap[dirtyBitObject.ctx];
            if (jssdkEvent) {
                events.fire(jssdkEvent, dirtyBitObject.data);
                logger.log(logPrefix, '[UPDATE]:', dirtyBitObject.ctx, ':', dirtyBitObject.data);
            }
        };

        dirtyBitsConnection.onerror = function(dirtyBitEvent) {
            logger.error(logPrefix, dirtyBitEvent);
            myEvents.fire('dirtybits:connectionchanged');
        };

        dirtyBitsConnection.onopen = function() {
            logger.log(logPrefix, 'connection established.');
            intervalID = setInterval(sendPong, KEEP_CONNECTION_ALIVE_TIMEOUT);
            myEvents.fire('dirtybits:connectionchanged');
        };

        dirtyBitsConnection.onclose = function() {
            //we null out our connection, if we reconnect we need to reinstantiate the socket object anyways
            dirtyBitsConnection = null;
            clearInterval(intervalID);

            logger.log(logPrefix, 'connection closed.');
            myEvents.fire('dirtybits:connectionchanged');
        };
    }

    function handleConnectionPromiseError(error) {
        logger.error(error.message);
    }

    function setupConnectionListener(callback) {
        myEvents.once('dirtybits:connectionchanged', callback);
    }

    /**
     * connect to dirty bits server
     */
    function connect() {
        return new Promise(function(resolve) {
            //if we are already connected lets just resolve and not try again
            //1 means connection established           
            if (dirtyBitsConnection && dirtyBitsConnection.readyState === 1) {
                resolve();
            } else {
                //listen for connection change so we can resolve
                setupConnectionListener(resolve);

                //we always want to resolve the websocket connection even for failure so that we continue
                createNewWebSocketConnection();
            }
        }).catch(handleConnectionPromiseError); //catch the promise here to handle any errors so that auth will always continue
    }

    /**
     * disconnect from dirty bits server
     */
    function disconnect() {
        return new Promise(function(resolve) {
            if (dirtyBitsConnection) {
                setupConnectionListener(resolve);
                dirtyBitsConnection.close();
            } else {
                //if there's no connection (cause we timed out or something) lets just resolve;
                resolve();
            }
        }).catch(handleConnectionPromiseError);
    }

    return {
        /**
         * Connect to the dirty bits server
         * @static
         * @method
         */
        connect: connect,
        /**
         * Disconnect from the dirty bits server
         * @static
         * @method
         */
        disconnect: disconnect
    };
});