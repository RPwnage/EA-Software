/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to social ui with the C++ client
 */


define([
    'modules/client/clientobjectregistry'
], function(clientobjectregistry) {

    /**
     * Contains client socialui communication methods
     * @module module:socialui
     * @memberof module:Origin.module:client
     *
     */

    var clientSocialUIWrapper = null,
        clientObjectName = 'OriginSocialUIManager';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientSocialUIWrapper = clientObjectWrapper;
        if (clientSocialUIWrapper.clientObject) {
            clientSocialUIWrapper.connectClientSignalToJSSDKEvent('showChatWindowForFriend', 'CLIENT_SOCIAL_SHOWCHATWINDOWFORFRIEND');
            clientSocialUIWrapper.connectClientSignalToJSSDKEvent('focusOnFriendsList', 'CLIENT_SOCIAL_FOCUSONFRIENDSLIST');
            clientSocialUIWrapper.connectClientSignalToJSSDKEvent('focusOnActiveChatWindow', 'CLIENT_SOCIAL_FOCUSONACTIVECHATWINDOW');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:socialui */ {
    };
});
