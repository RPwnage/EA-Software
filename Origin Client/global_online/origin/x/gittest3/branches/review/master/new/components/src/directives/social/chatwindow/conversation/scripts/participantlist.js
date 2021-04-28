(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationParticipantlistCtrl($scope, ConversationFactory, UtilFactory) {

	    var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-participantlist';

        $scope.participants = [];
        $scope.self = Origin.user.userPid();

        function buildParticipantList() {
            // Get the full list
            $scope.participants = $scope.conversation.participants;

            $scope.inChatLoc = UtilFactory.getLocalizedStr($scope.inChatStr, CONTEXT_NAMESPACE, 'inchatstr', {'%number%': $scope.participants.length});

            // Remove ourselves from the list
            var index = $scope.participants.indexOf(Origin.user.userPid());
            if (index > -1) {
                $scope.participants.splice(index, 1);
            }
        }

        function updateParticipantList() {
            buildParticipantList();
            $scope.$digest();
        }

        function onDestroy() {
            ConversationFactory.events.off('updateParticipantList', updateParticipantList);
        }

        buildParticipantList();

        ConversationFactory.events.on('updateParticipantList', updateParticipantList);
        $scope.$on('$destroy', onDestroy);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationParticipantlist
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @param {LocalizedString} inchatstr "%number% IN CHAT"
     * @description
     *
     * origin chatwindow -> conversation -> participant list
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-participantlist
     *            conversation="conversation"
     *            inchatstr="%number% IN CHAT"
     *         ></origin-social-chatwindow-conversation-participantlist>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationParticipantlistCtrl', OriginSocialChatwindowConversationParticipantlistCtrl)
        .directive('originSocialChatwindowConversationParticipantlist', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationParticipantlistCtrl',
                scope: {
                    conversation: '=',
                    inChatStr: '@inchatstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/participantlist.html'),
            };

        });
}());

