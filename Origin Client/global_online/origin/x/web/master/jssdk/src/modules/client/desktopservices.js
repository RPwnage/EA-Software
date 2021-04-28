/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to desktopservices with the C++ client
 */


define([
    'modules/client/clientobjectregistry',
    'core/errorhandler'
], function(clientobjectregistry, errorhandler) {

    /**
     * Contains client desktopServices communication methods
     * @module module:desktopServices
     * @memberof module:Origin.module:client
     *
     */
    
    var clientDesktopServicesWrapper = null,
        clientObjectName = 'DesktopServices';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientDesktopServicesWrapper = clientObjectWrapper;
        if(clientDesktopServicesWrapper.clientObject){
            clientDesktopServicesWrapper.connectClientSignalToJSSDKEvent('dockIconClicked', 'CLIENT_DESKTOP_DOCKICONCLICKED');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName)
        .then(executeWhenClientObjectInitialized)
        .catch(errorhandler.logErrorMessage);
    
    return /** @lends module:Origin.module:client.module:desktopServices */ {
        /**
         * opens a url in a external browser
         * @static
         * @method
         */
        asyncOpenUrl: function(url) {
            return clientDesktopServicesWrapper.sendToOriginClient('asyncOpenUrl', arguments);
        },

        flashIcon: function(int) {
            return clientDesktopServicesWrapper.sendToOriginClient('flashIcon', arguments);
        },
        /**
         * opens a url in a external browser with EADP SSO
         * @static
         * @method
         */        
        asyncOpenUrlWithEADPSSO: function(url) {
            return clientDesktopServicesWrapper.sendToOriginClient('launchExternalBrowserWithEADPSSO', arguments);
        }
    };
});
