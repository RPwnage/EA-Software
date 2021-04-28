(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationHistoryCtrl($scope, ComponentsConfigFactory, ConversationFactory, UtilFactory) {
        var $element, isManuallyScrolled = false;

        $scope.typingState = 'ACTIVE';
        $scope.typingImageSrc = ComponentsConfigFactory.getImagePath('social\\composing.gif');
        $scope.idleImageSrc = ComponentsConfigFactory.getImagePath('social\\idle.gif');

        this.setElement = function(el) {
            $element = el;
        };

        this.onScroll = function() {
            var historyEl = $element[0],
                isAtBottom = historyEl.scrollHeight - historyEl.scrollTop === historyEl.clientHeight;
            isManuallyScrolled = !isAtBottom;
        };

        function scrollToBottom () {
            $element.scrollTop( $element[0].scrollHeight );
        }

        function onHistoryUpdated(conversationId) {
            if (conversationId.toString() === $scope.conversation.id.toString()){
                $scope.$digest();
                // Scroll the new message into view
                if (!isManuallyScrolled) {
                    scrollToBottom();
                }
            }
        }

        function onTypingStateUpdated(state) {
            $scope.typingState = state;
            $scope.$digest();
            // Scroll the typing indicator into view
            if (!isManuallyScrolled) {
                scrollToBottom();
            }
        }

        function onDestroy() {
            ConversationFactory.events.off('historyUpdated', onHistoryUpdated);
            ConversationFactory.events.off('typingStateUpdated:' + $scope.conversation.id, onTypingStateUpdated);
        }

        ConversationFactory.events.on('historyUpdated', onHistoryUpdated);
        ConversationFactory.events.on('typingStateUpdated:' + $scope.conversation.id, onTypingStateUpdated);
        $scope.$on('$destroy', onDestroy);

        $scope.getHistoryWidth = function () {
            return $element.innerWidth();
        };

        $scope.getHistoryHeight = function () {
            return $element.innerHeight();
        };

        $scope.$watch($scope.getHistoryWidth, function () {
            // Keep the view scrolled to the bottom when the view size changes
            UtilFactory.throttle(
                function () {
                    scrollToBottom();
                }, 100).apply();
        }, true);

        $scope.$watch($scope.getHistoryHeight, function () {
            // Keep the view scrolled to the bottom when the view size changes
            UtilFactory.throttle(
                function () {
                    scrollToBottom();
                }, 100).apply();
        }, true);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationHistory
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @description
     *
     * origin chatwindow -> conversation -> history
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-history
     *            conversation="conversation"
     *         ></origin-social-chatwindow-conversation-history>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationHistoryCtrl', OriginSocialChatwindowConversationHistoryCtrl)
        .directive('originSocialChatwindowConversationHistory', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationHistoryCtrl',
                scope: {
                    conversation : '='
                },
                link: function(scope, element, attrs, ctrl) {
                    var $history = $(element).find('.origin-social-chatwindow-conversation-history');

                    ctrl.setElement($history);

                    $history.on('scroll', function() {
                        ctrl.onScroll();
                    });

                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/history.html'),
            };

        });
}());

