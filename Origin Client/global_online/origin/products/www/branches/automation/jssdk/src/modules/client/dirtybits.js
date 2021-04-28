/*jshint strict: false */
/**
 * this module handles all the communication related to dirtybits with the C++ client
 */
define([
    'modules/client/clientobjectregistry',
    'core/errorhandler',
    'core/events',
    'core/logger',
    'core/dirtybits'
], function(clientobjectregistry, errorhandler, events, logger, dirtybits) {

    /**
     * Contains connection to dirtybits on client
     * @module module:dirtybits
     * @memberof module:Origin.module:client
     *
     */
    var clientObjectName = 'OriginDirtyBits',
        logPrefix = '[DIRTYBITS-CLIENT]',
        signalName = 'dirtyBitEvent';

    function onMessage(info) {
        var jssdkEvent = dirtybits.contextToJSSDKEventMap[info.ctx];
        if (jssdkEvent) {
            //here we intercept the signal from the C++ and wait till the next event loop
            //before relaying the signal
            //
            //We've seen strange we behavior with out of focus client and promises that are called as a part
            //of the callstack from a C++ signal. Promises seem to hang until the user clicks focus again
            //
            //Putting the signal on the next event loop fixes this
            setTimeout(function() {
                events.fire(jssdkEvent, info.data);
                logger.log(logPrefix, '[UPDATE]:', jssdkEvent, ':', info);
            }, 0);
        }
    }

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        var clientSettingsWrapper = clientObjectWrapper;
        clientSettingsWrapper.clientObject[signalName].connect(onMessage);
    }

    clientobjectregistry.registerClientObject(clientObjectName)
        .then(executeWhenClientObjectInitialized)
        .catch(errorhandler.logErrorMessage);
});
