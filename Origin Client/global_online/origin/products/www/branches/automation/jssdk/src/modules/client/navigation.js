/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to navigation with the C++ client
 */


define([
    'modules/client/clientobjectregistry',
    'core/errorhandler'
], function(clientobjectregistry, errorhandler) {

    /**
     * Contains client navigation communication methods
     * @module module:navigation
     * @memberof module:Origin.module:client
     *
     */
    var clientObjectName = 'OriginClientRouting',
        clientNavWrapper = null;

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientNavWrapper = clientObjectWrapper;
        if (clientNavWrapper.clientObject) {
            clientNavWrapper.connectClientSignalToJSSDKEvent('routeChange', 'CLIENT_NAV_ROUTECHANGE');
            clientNavWrapper.connectClientSignalToJSSDKEvent('navigateToStoreByProductId', 'CLIENT_NAV_STOREBYPRODUCTID');
            clientNavWrapper.connectClientSignalToJSSDKEvent('navigateToStoreByMasterTitleId', 'CLIENT_NAV_STOREBYMASTERTITLEID');
            clientNavWrapper.connectClientSignalToJSSDKEvent('openModalFindFriends', 'CLIENT_NAV_OPENMODAL_FINDFRIENDS');
            clientNavWrapper.connectClientSignalToJSSDKEvent('openDownloadQueue', 'CLIENT_NAV_OPEN_DOWNLOADQUEUE');
            clientNavWrapper.connectClientSignalToJSSDKEvent('focusOnSearch', 'CLIENT_NAV_FOCUSONSEARCH');
            clientNavWrapper.connectClientSignalToJSSDKEvent('showPendingUpdateSitestripe', 'CLIENT_NAV_SHOW_PENDING_UPDATE_STRIPE');
            clientNavWrapper.connectClientSignalToJSSDKEvent('showPendingUpdateCountdownSitestripe', 'CLIENT_NAV_SHOW_PENDING_UPDATE_COUNTDOWN_STRIPE');
            clientNavWrapper.connectClientSignalToJSSDKEvent('showPendingUpdateKickedOfflineSitestripe', 'CLIENT_NAV_SHOW_PENDING_UPDATE_KICKED_OFFLINE_STRIPE');
            clientNavWrapper.connectClientSignalToJSSDKEvent('openGameDetails', 'CLIENT_NAV_OPEN_GAME_DETAILS');
            clientNavWrapper.connectClientSignalToJSSDKEvent('renewSubscription', 'CLIENT_NAV_RENEW_SUBSCRIPTION');
            clientNavWrapper.connectClientSignalToJSSDKEvent('redrawVideoPlayer', 'CLIENT_NAV_REDRAW_VIDEO_PLAYER');            
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName)
        .then(executeWhenClientObjectInitialized)
        .catch(errorhandler.logErrorMessage);

    return /** @lends module:Origin.module:client.module:client */ {
        showOriginHelp: function() {
            return clientNavWrapper.sendToOriginClient('showOriginHelp', arguments);
        }
    };

});
