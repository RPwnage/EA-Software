(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationHistoryCtrl($scope, ComponentsConfigFactory, ConversationFactory, UtilFactory, RosterDataFactory, EventsHelperFactory) {
        var $element, isManuallyScrolled = false, handles = [];

        $scope.typingState = 'ACTIVE';
        $scope.typingImageSrc = ComponentsConfigFactory.getImagePath('social\\composing.gif');
        $scope.idleImageSrc = ComponentsConfigFactory.getImagePath('social\\idle.gif');

        $scope.eventTrackingFunction = function (index, subtype) {
            var id = index;
            if (subtype !== undefined) {
                id += subtype;
            }
            return id;
        };

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

        function onHistoryUpdated(conversationId, forceScrollToBottom) {
            if (conversationId.toString() === $scope.conversation.id.toString()){
                $scope.$digest();
                // Scroll the new message into view
                if (!isManuallyScrolled || forceScrollToBottom) {
                    scrollToBottom();
                }
            }
        }

        function onTypingStateUpdated(state) {
            RosterDataFactory.getFriendInfo($scope.conversation.participants[0]).then(function (friend) {
                // If the user is unavailable don't show any animation
                // this only works in the embedded client web client presence is different
                $scope.typingState = friend.presence.show === 'UNAVAILABLE' ? 'ACTIVE' : state;
                $scope.$digest();
                // Scroll the typing indicator into view
                if (!isManuallyScrolled) {
                    scrollToBottom();
                }
            });
        }

        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        handles[handles.length] = ConversationFactory.events.on('historyUpdated', onHistoryUpdated);
        handles[handles.length] = ConversationFactory.events.on('typingStateUpdated:' + $scope.conversation.id, onTypingStateUpdated);
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
                    if (!isManuallyScrolled) {
                        scrollToBottom();
                    }
                }, 100).apply();
        }, true);

        $scope.$watch($scope.getHistoryHeight, function () {
            // Keep the view scrolled to the bottom when the view size changes
            UtilFactory.throttle(
                function () {
                    if (!isManuallyScrolled) {
                        scrollToBottom();
                    }
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

                    // Prevents page from scrolling while
                    // scrolling inside viewport area
                    $history.bind('mousewheel', function (e) {
                        $(this).scrollTop($(this).scrollTop() - e.originalEvent.wheelDeltaY);
                        return false;
                    });

                    function onDestroy() {
                        $history.unbind('mousewheel');
                        $history.off('scroll');
                    }

                    scope.$on('$destroy', onDestroy);

                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/history.html'),
            };

        });
}());

