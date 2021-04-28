(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationPresenceindicatorCtrl($scope, RosterDataFactory, ObserverFactory) {

        // This directive is intended to be used only for 1:1 conversations, so we can do the following...
        var nucleusId = $scope.conversation.participants[0];

		RosterDataFactory.getRosterUser(nucleusId).then(function(user) {
			if (!!user) {
				ObserverFactory.create(user._presence)
					.defaultTo({jid: nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
					.attachToScope($scope, 'presence');									
			} 
		});			

        // listen for xmpp connect/disconnect
		ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
			.defaultTo({isConnected: Origin.xmpp.isConnected()})
			.attachToScope($scope, 'xmppConnection');		
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationPresenceindicator
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @description
     *
     * origin chatwindow -> conversation -> presence indicator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-presenceindicator
     *            conversation="conversation"
     *         ></origin-social-chatwindow-conversation-presenceindicator>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationPresenceindicatorCtrl', OriginSocialChatwindowConversationPresenceindicatorCtrl)
        .directive('originSocialChatwindowConversationPresenceindicator', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationPresenceindicatorCtrl',
                scope: {
                    conversation: '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/presenceindicator.html'),
            };

        });
}());

