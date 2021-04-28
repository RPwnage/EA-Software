(function () {

    'use strict';
    
    /**
    * The controller
    */
    function OriginSocialChatwindowConversationHistoryitemSystemCtrl($scope, $timeout, SocialDataFactory, RosterDataFactory, UtilFactory)
    {
        var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-historyitem-system';

        var subtype = $scope.systemEvent.subtype, message = '', originIds = [], originIdsStr = '';

        function messageForMultipleUserEventType(type) {
            var msg = '';
            if (originIds.length > 0) {
                switch(type) {
                    case 'participant_enter':
                        msg = UtilFactory.getLocalizedStr($scope.userJoinedChatStr, CONTEXT_NAMESPACE, 'userjoinedchatstr', {'%username%': originIdsStr});
                        break;
                    case 'participant_exit':
                        if (originIds.length > 1) {
                            msg = UtilFactory.getLocalizedStr($scope.usersLeftChatStr, CONTEXT_NAMESPACE, 'usersleftchatstr', {'%username%': originIdsStr});
                        }
                        else {
                            msg = UtilFactory.getLocalizedStr($scope.userLeftChatStr, CONTEXT_NAMESPACE, 'userleftchatstr', {'%username%': originIdsStr});
                        }
                        break;
                }
            }
            return msg;
        }

        function buildOriginIdsArray() {
            originIds = [];
            $.each($scope.systemEvent.from, function (index, value) {
                if (Number(value) === Origin.user.userPid()) {
                    if (originIds.indexOf(Origin.user.originId()) === -1) {
                        originIds.push(Origin.user.originId());
                        updateOriginIdsString();
                    }
                }
                else {
                    SocialDataFactory.getGroupMemberInfo(value).then(function (originId) {
                        if (originIds.indexOf(originId) === -1) {
                            originIds.push(originId);
                            updateOriginIdsString();
                        }
                    });
                }
            });
        }

        function updateOriginIdsString() {
            originIdsStr = originIds.join(', ');
            $scope.message = messageForMultipleUserEventType(subtype);
            $scope.$digest();
        }

        function messageForPresence(presence, originId)
        {
            var msg = '';
            switch(presence)
            {
                case 'online':
                    msg = UtilFactory.getLocalizedStr($scope.userOnlineStr, CONTEXT_NAMESPACE, 'useronlinestr', {'%username%': originId});
                    break;
                case 'unavailable':
                case 'offline':
                    msg = UtilFactory.getLocalizedStr($scope.userOfflineStr, CONTEXT_NAMESPACE, 'userofflinestr', {'%username%': originId});
                    break;
                case 'away':
                    msg = UtilFactory.getLocalizedStr($scope.userAwayStr, CONTEXT_NAMESPACE, 'userawaystr', {'%username%': originId});
                    break;
            }
            return msg;
        }

        if (subtype === 'presence') {
            RosterDataFactory.getFriendInfo($scope.systemEvent.from).then(function (friend) {
                originIdsStr = friend.originId;
                $scope.message = messageForPresence($scope.systemEvent.presence, originIdsStr);
                $scope.$digest();
            });        
        }
        else if (subtype === 'call_started') {
            message = UtilFactory.getLocalizedStr($scope.callStartedStr, CONTEXT_NAMESPACE, 'callstartedstr');
        }
        else if (subtype === 'call_ended') {
            message = UtilFactory.getLocalizedStr($scope.callEndedStr, CONTEXT_NAMESPACE, 'callendedstr');
        }
        else if (subtype === 'empty_room') {
            message = UtilFactory.getLocalizedStr($scope.emptyRoomStr, CONTEXT_NAMESPACE, 'emptyroomstr');
        }
        else if ((subtype === 'participant_enter') || (subtype === 'participant_exit')) {
            buildOriginIdsArray();
        }
        else if (subtype === 'phishing') {
            message = UtilFactory.getLocalizedStr($scope.phishingStr, CONTEXT_NAMESPACE, 'phishingstr');
        }
        else if (subtype === 'offline_mode') {
            message = UtilFactory.getLocalizedStr($scope.youAreOfflineStr, CONTEXT_NAMESPACE, 'youareofflinestr');
        }

        $scope.message = message;

        $scope.$watch('systemEvent.presence', function() {
            if (subtype === 'presence') {
                $scope.message = messageForPresence($scope.systemEvent.presence, originIdsStr);
            }
        });

        if ((subtype === 'participant_enter') || (subtype === 'participant_exit')) {
            $scope.$watch(function (scope) { return scope.systemEvent.from.length; }, function () {
                buildOriginIdsArray();
            });
        }            
     
        $scope.$watch('systemEvent.time', function() {
            $scope.time = UtilFactory.getFormattedTimeStr($scope.systemEvent.time);
        });       
     
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationHistoryitemSystem
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} systemEvent Javascript object representing the system message event
     * @param {LocalizedString} userjoinedchatstr "%username% joined the chat"
     * @param {LocalizedString} userleftchatstr "%username% has left the chat"
     * @param {LocalizedString} usersleftchatstr "%username% have left the chat"
     * @param {LocalizedString} useronlinestr "%username% is now online"
     * @param {LocalizedString} userofflinestr "%username% is offline. Messages you send will be delivered when %username% comes online."
     * @param {LocalizedString} userawaystr "%username% is away"
     * @param {LocalizedString} callstartedstr "Call Started"
     * @param {LocalizedString} callendedstr "Call Ended"
     * @param {LocalizedString} emptyroomstr "There\'s no one in your chat room yet. Invite friends into this chat room"
     * @param {LocalizedString} phishingstr "Never share your password with anyone."
     * @param {LocalizedString} youareofflinestr "You are offline"
     * @description
     *
     * origin chatwindow -> conversation -> history item -> system message
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-historyitem-system
     *            systemEvent="systemEvent"
     *            userjoinedchatstr="%username% joined the chat"
     *            userleftchatstr="%username% has left the chat"
     *            usersleftchatstr="%username% have left the chat"
     *            useronlinestr="%username% is now online"
     *            userofflinestr="%username% is offline. Messages you send will be delivered when %username% comes online."
     *            userawaystr="%username% is away"
     *            callstartedstr="Call Started"
     *            callendedstr="Call Ended"
     *            emptyroomstr="There\'s no one in your chat room yet. Invite friends into this chat room"
     *            phishingstr="Never share your password with anyone."
     *            youareofflinestr="You are offline"
     *         ></origin-social-chatwindow-conversation-historyitem-system>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationHistoryitemSystemCtrl', OriginSocialChatwindowConversationHistoryitemSystemCtrl)
        .directive('originSocialChatwindowConversationHistoryitemSystem', function(ComponentsConfigFactory) {
        
            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationHistoryitemSystemCtrl',
                scope: {
                    systemEvent: '=',
                    userJoinedChatStr: '@userjoinedchatstr',
                    userLeftChatStr: '@userleftchatstr',
                    usersLeftChatStr: '@usersleftchatstr',
                    userOnlineStr: '@useronlinestr',
                    userOfflineStr: '@userofflinestr',
                    userAwayStr: '@userawaystr',
                    callStartedStr: '@callstartedstr',
                    callEndedStr: '@callendedstr',
                    emptyRoomStr: '@emptyroomstr',
                    phishingStr: '@phishingstr',
                    youAreOfflineStr: '@youareofflinestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/historyitem/views/system.html')
            };
        
        });
}());

