/*jshint unused: false */
/*jshint strict: false */
define([
    'QWebChannel',
    'core/logger'
], function(QWebChannel, logger) {
    var channel = null,
        connectionPromise = null,
        CONNECTION_TIMEOUT = 40000,
        connectAttemptCompleted = false,
        defaultBridgeObject = 'OriginGamesManager',
        typeEnum = {
            BRIDGE: 'BRIDGE',
            NOTCONNECTED: 'NOTCONNECTED',
            WEBCHANNEL: 'WEBCHANNEL',
            CONNECTIONERROR: 'CONNECTIONERROR'
        },
        connectionType = typeEnum.NOTCONNECTED;

    function remoteTransportStub() {
        //this is what we would use to communicate remotely in the future
        logger.log('[WEBCHANNEL] Stub Transport for remote selected');
        return null;
    }

    function embeddedTransportAvailable() {
        //the qt object exists only in the embedded browser for web channel
        //
        //window.OriginOIGBrowser is injected by the C++ client, we add this check for the OIGBrowser instead of check for a specific ClientViewController
        //param to be true, because in case the injection of the window.OriginOIGBrowser fails the worst that can happen is the user has to wait for the 
        //webchannel connection attempt to time out.
        return (typeof qt !== 'undefined') && (typeof qt.webChannelTransport !== 'undefined') && (!window.OriginOIGBrowser);
    }

    function bridgeAvailable() {
        //we just check here if one of the bridge objects exist
        return (typeof window[defaultBridgeObject] !== 'undefined');
    }

    function isEmbeddedBrowser() {
        //if we have a connection error, do not consider this an embedded browser
        return (embeddedTransportAvailable() || bridgeAvailable()) && (connectionType !== typeEnum.CONNECTIONERROR) ;
    }

    function createConnectionPromise() {
        return new Promise(function(resolve) {
            var timeoutHandle,
                transport = embeddedTransportAvailable() ? qt.webChannelTransport : remoteTransportStub();
            //if we have a transport available, then we can use webchannel
            if (transport) {
                timeoutHandle = setTimeout(timeoutCallback(resolve), CONNECTION_TIMEOUT);
                //attempt a web channel connection
                channel = new QWebChannel(transport, function() {
                    clearTimeout(timeoutHandle);
                    connectAttemptCompleted = true;
                    connectionType = typeEnum.WEBCHANNEL;
                    resolve();
                });
            } else {
                //if not webchannel then lets try bridge
                if (bridgeAvailable()) {
                    connectionType = typeEnum.BRIDGE;
                }

                // we always resolve here so anythign waiting (like the initialization flow) can continue on
                resolve();
            }
        });
    }

    function connectionError() {
        connectionType = typeEnum.CONNECTIONERROR;
        resetConnectionPromise();
        logger.error('[CLIENTCONNECT] unable to connect');
    }

    function timeoutCallback(resolve) {
        return function() {
            logger.error('[CLIENTCONNECT] timedout');
            connectionError();
            resolve();
        };
    }

    function resetConnectionPromise() {
        connectionPromise = null;
    }

    function waitForConnectionEstablished() {
        //if we've attempted the connection the webchannel is ready
        if (!isEmbeddedBrowser() || connectAttemptCompleted) {
            return Promise.resolve();
        }

        //if we are not in the middle of a promise, instantiate a new one, else return the existing one
        if (!connectionPromise) {
            connectionPromise = createConnectionPromise().then(resetConnectionPromise).catch(connectionError);
        }

        return connectionPromise;
    }



    function getClientObject(objectName) {

        var clientObject = null;
        if (channel && channel.objects[objectName]) {
            //webchannel
            clientObject = channel.objects[objectName];
        } else if (typeof window[objectName] !== 'undefined') {
            //bridge
            clientObject = window[objectName];
        } 
        
        return clientObject;
    }

    return {
        /**
         * get the current connecton type
         * @return {CommunicationTypeEnum} returns either BRIDGE WEBCHANNEL NOTCONNECTED
         */
        getConnectionType: function() {
            return connectionType;
        },
        /**
         * are we in an embedded browser
         * @method
         * @returns {boolean} true if we are in an embedded browser, false otherwise
         */
        isEmbeddedBrowser: isEmbeddedBrowser,
        /**
         * retrieves the client object based on the communication type. If we have a webchannel connection, we will return the client objects
         * that live in the channel. If we are using the bridge it will return the global bridge object
         * @method
         * @returns {clientObject} The actual C++ object over the bridge or webchannel
         */
        getClientObject: getClientObject,
        /**
         * This function returns a promise once a connection with the client has been established or determined that we are in a remote browser
         * @returns {Promise}
         * @method
         */
        waitForConnectionEstablished: waitForConnectionEstablished,
        /**
         * public enums so that other modules can check the connection type
         */
        typeEnum: typeEnum
    };

});