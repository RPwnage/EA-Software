/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to OriginPopout with the C++ client
 */


define([
    'modules/client/clientobjectregistry'
], function(clientobjectregistry) {

    /**
     * Contains client origin popout communication methods
     * @module module:popout
     * @memberof module:Origin.module:client
     *
     */


    var clientPopoutWrapper = null,
        clientObjectName = 'OriginPopout';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientPopoutWrapper = clientObjectWrapper;
        if(clientPopoutWrapper.clientObject) {
            clientPopoutWrapper.connectClientSignalToJSSDKEvent('popOutClosed', 'CLIENT_POP_OUT_CLOSED');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName)
        .then(executeWhenClientObjectInitialized);


    return /** @lends module:Origin.module:client.module:popout */ {
        /**
         * Pops the window back in
         */
        popInWindow: function() {
            return clientPopoutWrapper.sendToOriginClient('popInWindow', arguments);
        }
    };
});
