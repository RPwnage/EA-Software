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
            clientDesktopServicesWrapper.connectClientSignalToJSSDKEvent('appVisibilityChanged', 'CLIENT_VISIBILITY_CHANGED');
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

        /**
         * flashes the dock icon (Windows) or badges the dock icon with unread message count (Mac)
         * @static
         * @method
         */
        flashIcon: function(int) {
            return clientDesktopServicesWrapper.sendToOriginClient('flashIcon', arguments);
        },

        /**
         * sets the next UUID to be used by a created js window
         * @static
         * @method
         */
        setNextWindowUUID: function(uuid) {
            return clientDesktopServicesWrapper.sendToOriginClient('setNextWindowUUID', arguments);
        },

       
        /**
         * miniaturizes the window associated with the given uuid to the dock
         * @static
         * @method
         */
        miniaturize: function(uuid) {
            return clientDesktopServicesWrapper.sendToOriginClient('miniaturize', arguments);
        },

        /**
         * deminiaturizes the window associated with the given uuid to the dock
         * @static
         * @method
         */
        deminiaturize: function(uuid) {
            return clientDesktopServicesWrapper.sendToOriginClient('deminiaturize', arguments);
        },

        /**
         * enquires whether the window associated with the given uuid is miniaturized
         * @static
         * @method
         */
        isMiniaturized: function(uuid) {
            return clientDesktopServicesWrapper.sendToOriginClient('isMiniaturized', arguments);
        },

        /**
         * unminized if minimized and show if hidden
         * @static
         * @method
         */
        showWindow: function(uuid) {
            return clientDesktopServicesWrapper.sendToOriginClient('showWindow', arguments);
        },

        /**
         * moves the window associated with the given UUID to the front
         * @static
         * @method
         */
        moveWindowToForeground: function(uuid) {
            return clientDesktopServicesWrapper.sendToOriginClient('moveWindowToForeground', arguments);
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
