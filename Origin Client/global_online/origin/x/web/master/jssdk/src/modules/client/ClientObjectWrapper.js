/*jshint strict: false */
/*jshint unused: false */

define([
    'core/logger',
    'modules/client/communication',
    'core/events'
], function(logger, communication, events) {

    /**
     * @class Communicator
     */

    function clientObjectHasError(self, property) {
        var error = null;
        if (!self.clientObject) {
            error = new Error('ERROR: client object is null -- trying to use ' + self.clientObjectName + ':' + property);
        } else if (typeof self.clientObject[property] === 'undefined') {
            new Error('ERROR: client property/signal/function not found:' + self.clientObjectName + ':' + property);
        }

        return error;
    }

    function ClientObjectWrapper(clientObjectName) {
        this.clientObjectName = clientObjectName;
        this.clientObject = communication.getClientObject(clientObjectName);
    }

    /**
     * Subscribe to an event so when the event is fired, the callback
     * is executed in the context passed
     * @param {string} eventName - the event name
     * @param {Function} fn - the callback
     * @param {object} context - the context (what this refers to)
     * @return {void}
     * @method on
     */
    ClientObjectWrapper.prototype.sendToOriginClient = function(clientFnName, params) {
        var paramArray = [],
            self = this,
            error = clientObjectHasError(self, clientFnName);

        if (error) {
            return Promise.reject(error);
        }

        return new Promise(function(resolve, reject) {
            var connectionType = communication.getConnectionType(),
                result = null;
            if (params) {
                paramArray = Array.prototype.slice.call(params);
            }

            //if its webchannel, it expects we pass the callback function as the last param
            if (connectionType === communication.typeEnum.WEBCHANNEL) {
                paramArray.push(resolve);
            }

            result = self.clientObject[clientFnName].apply(self, paramArray);

            //if its bridge, we resolve the result immediately after we get it (since function calls are synchronous)
            if (connectionType === communication.typeEnum.BRIDGE) {
                resolve(result);
            }
        });
    };

    ClientObjectWrapper.prototype.connectClientSignalToJSSDKEvent = function(signalName, jssdkSignalName) {
        var self = this,
            error = clientObjectHasError(self, signalName);
        //check if we have an event by that name
        if (typeof events[jssdkSignalName] === 'undefined') {
            logger.error('ERROR: jssdk event not found:', jssdkSignalName);
            return;
        }

        //check if our client object is in order
        if (error) {
            logger.error(error.message);
            return;
        }

        self.clientObject[signalName].connect(function() {
            var args = Array.prototype.slice.call(arguments);
            args.unshift(events[jssdkSignalName]);
            //here we intercept the signal from the C++ and wait till the next event loop
            //before relaying the signal
            //
            //We've seen strange we behavior with out of focus client and promises that are called as a part
            //of the callstack from a C++ signal. Promises seem to hang until the user clicks focus again
            //
            //Putting the signal on the next event loop fixes this
            setTimeout(function() {
                events.fire.apply(events, args);                
            }, 0);
        });

    };

    ClientObjectWrapper.prototype.propertyFromOriginClient = function(propertyName) {
        var self = this,
            error = clientObjectHasError(self, propertyName);

        if (error) {
            logger.error(error.message);
            return null;
        }

        return self.clientObject[propertyName];
    };

    return ClientObjectWrapper;

});