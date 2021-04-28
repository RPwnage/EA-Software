/*jshint unused: false */
/*jshint strict: false */
define([
    'uuid',
    'core/events',
    'core/utils',
    'core/logger'
], function (uuid, events, utils, logger) {

    var remoteClientAvailable = false;
    var eventsQueue = [];
    var activeRequest = [];
    var clientEvents = new utils.Communicator();

    /**
     * create an msg object to send to the client
     * @param  {string} proxyName        name of the C++ proxy object
     * @param  {string} functionName     name of the C++ proxy function
     * @param  {object} params           any specific params needed by the proxy function in a json object
     * @param  {function} responseFunction callback triggered when we get a confirmation that the C++ received the object
     * @param  {number} timeout          amount in millisecs before we abort request
     * @return {object}
     */
    function createRequest(proxyName, functionName, params, responseFunction, timeout) {
        var myuuid = uuid.v4();
        return {
            callbackObj: {
                uuid: myuuid,
                callbackFunc: responseFunction,
                timeout: timeout
            },
            requestObj: {
                'uuid': myuuid,
                'proxyName': proxyName,
                'functionName': functionName,
                'params': params
            }
        };
    }

    /**
     * gets triggered if the request to C++ timed out
     * @param  {string} uuid the uuid of the request
     */
    function timeOutResponse(uuid) {
        var response = {
            header: {
                uuid: uuid
            },
            data: {}
        };
        return function() {
            onActionMsgResponseFromClient(response);
        };
    }


    /**
     * sends the msg to the Origin client over a websocket
     */
    function sendActionMsgToClient(actionObj) {
        if (remoteClientAvailable) {
            if (actionObj.callbackObj.timeout) {
                setTimeout(timeOutResponse(actionObj.requestObj.uuid), actionObj.callbackObj.timeout);
            }
            activeRequest.push(actionObj.callbackObj);
            events.fire(events.priv.REMOTE_STATUS_SENDACTION, JSON.stringify(actionObj.requestObj));
        } else {
            eventsQueue.push(actionObj);
        }
    }

    /**
     * triggered when we get a confirmation that our request was received
     */
    function onActionMsgResponseFromClient(response) {
        var index = -1;
        for (var i = 0; i < activeRequest.length; i++) {
            if (activeRequest[i].uuid === response.header.uuid) {
                if ((typeof activeRequest[i].callbackFunc) === 'function') {
                    activeRequest[i].callbackFunc(response.data);
                }
                index = i;
                break;
            }
        }
        if (index > -1) {
            activeRequest.splice(index, 1);
        }
    }

    /**
     * triggered when we get status update from the client
     */
    function onStatusUpdateFromClient(msgObject) {
        if (msgObject.header.confirm) {
            events.fire(events.priv.REMOTE_SEND_CONFIRMATION_TO_CLIENT, JSON.stringify({
                uuid: msgObject.header.uuid
            }));
        }
        clientEvents.fire(msgObject.header.id, msgObject.data);
    }

    /**
     * triggered when wthe client is ready to send/receive messages
     */
    function onClientReady(ready) {
        remoteClientAvailable = ready;
        if (ready) {
            while (eventsQueue.length > 0) {
                var actionObj = eventsQueue.pop();
                sendActionMsgToClient(actionObj);
            }
        }
    }

    /**
     * injected in the standalone browser case, sends message to client via websocket
     */
    function sendToClientRemote(proxyName, functionName, params, timeout) {
        var promiseResolve = function(resolve, reject) {
            function responseReceived(data) {
                resolve(data.data);
            }

            sendActionMsgToClient(createRequest(proxyName, functionName, params, responseReceived, timeout));
        };
        return new Promise(promiseResolve);
    }

    /**
     * injected in the embedded browser case, sends message to client via bridge
     */
    function sendToClientBridge(proxyName, functionName, params) {
        var promiseResolve = function(resolve, reject) {
            resolve(window[proxyName][functionName](params).data);
        };
        return new Promise(promiseResolve);
    }

    /**
     * injected in the standalone browser case, listens for messages from client via websocket
     */
    function listenForOriginClientMsgRemote(proxyObjName, signalName, callback) {
        clientEvents.on(proxyObjName + ':' + signalName, callback);
    }

    /**
     * injected in the embedded browser case, listens for messages from client via qt bridge/signals
     */
    function listenForOriginClientMsgBridge(proxyObjName, signalName, callback) {
        if (window[proxyObjName] && window[proxyObjName][signalName] && window[proxyObjName][signalName].connect) {
            window[proxyObjName][signalName].connect(callback);
        } else {
            logger.error('ERROR: bridge signal not found:', proxyObjName, signalName);
        }
    }

    function chooseStrategy(strategyBridge, strategyRemote) {
        if (utils.bridgeAvailable()) {
            return strategyBridge;
        } else {
            return strategyRemote;
        }
    }

    //events from strophe
    events.on(events.priv.REMOTE_STATUS_UPDATE_FROM_CLIENT, onStatusUpdateFromClient);
    events.on(events.priv.REMOTE_CONFIRMATION_FROM_CLIENT, onActionMsgResponseFromClient);
    events.on(events.priv.REMOTE_CLIENT_AVAILABLE, onClientReady);

    return {
        //choose one of the functions to use for sendToOriginClient depending if we are in the remote or embedded case
        sendToOriginClient: chooseStrategy(sendToClientBridge, sendToClientRemote),
        listenForOriginClientMsg: chooseStrategy(listenForOriginClientMsgBridge, listenForOriginClientMsgRemote)
    };

});
