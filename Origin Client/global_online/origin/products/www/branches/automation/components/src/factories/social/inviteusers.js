(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-inviteusers';

    function InviteUsersFactory(DialogFactory, UtilFactory, AuthFactory) {

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('origin-dialog-content-invite-users-id');
            }
        }

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        function onInviteUsersToGameFinished(result) {
            if (result.accepted) {
                var userList = result.content.userList;
                angular.forEach(userList, function (userId) {
                    Origin.xmpp.inviteToGame(userId);
                });
            }
        }

        return {

            inviteUsersToGame: function (gameTitle) {
                // Show the Find Friends dialog
                var content = DialogFactory.createDirective('origin-dialog-content-inviteusers', { gametitlestr: gameTitle });
                DialogFactory.openPrompt({
                    id: 'origin-dialog-content-invite-users-id',
                    title: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'invite-user-dialog-title-text'),
                    acceptText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'invite-user-dialog-invite-text'),
                    acceptEnabled: false,
                    rejectText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'invite-user-dialog-cancel-text'),
                    defaultBtn: 'yes',
                    contentDirective: content,
                    callback: onInviteUsersToGameFinished
                });
            }

        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function InviteUsersFactorySingleton(DialogFactory, UtilFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('InviteUsersFactory', InviteUsersFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.InviteUsersFactory
     
     * @description
     *
     * InviteUserFactory
     */
    angular.module('origin-components')
        .factory('InviteUsersFactory', InviteUsersFactorySingleton);
}());
