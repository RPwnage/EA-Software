(function () {

    'use strict';
    
    /**
    * The controller
    */
    function OriginSocialChatwindowConversationHistoryitemTimestampCtrl($scope, UtilFactory)
    {
        $scope.time = UtilFactory.getFormattedTimeStr($scope.timestampEvent.time);
        $scope.$watch('timestampEvent.time', function() {
            $scope.time = UtilFactory.getFormattedTimeStr($scope.timestampEvent.time);
        });       
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationHistoryitemTimestamp
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} timestampEvent Javascript object representing the timestamp event
     * @description
     *
     * origin chatwindow -> conversation -> history item -> timestamp
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-historyitem-timestamp
     *            timestampEvent="timestampEvent"
     *         ></origin-social-chatwindow-conversation-historyitem-timestamp>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationHistoryitemTimestampCtrl', OriginSocialChatwindowConversationHistoryitemTimestampCtrl)
        .directive('originSocialChatwindowConversationHistoryitemTimestamp', function(ComponentsConfigFactory) {
        
            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationHistoryitemTimestampCtrl',
                scope: {
                    timestampEvent: '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/historyitem/views/timestamp.html')
            };
        
        });
}());

