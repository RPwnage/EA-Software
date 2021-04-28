(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationHistoryitemSystemCtrl($scope, $timeout, $sce, SocialDataFactory, RosterDataFactory, UtilFactory, NavigationFactory)
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

        function createHyperlink(url, string) {
            return '<a class="otka" href=\"' + url + '\">' + string + '</a>';
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
            Origin.client.settings.readSetting('EnablePushToTalk').then( function(pttEnabled) {
                if (pttEnabled) {
                    Origin.client.settings.readSetting('PushToTalkKeyString').then( function(pttString) {
                        if (pttString.indexOf('mouse') !== -1) {
                            var pttButton = pttString.match(/\d+/);
                            pttString = UtilFactory.getLocalizedStr($scope.mouseButtonStr, CONTEXT_NAMESPACE, 'mousebuttonstr').replace('%number%', pttButton);
                        }
                        $scope.message = UtilFactory.getLocalizedStr($scope.joinedVoiceChatStr, CONTEXT_NAMESPACE, 'joinedvoicechatstr').replace('%pttString%', pttString);
                        $scope.$digest();
                    });
                }
                else {
                    $scope.message = UtilFactory.getLocalizedStr($scope.callStartedStr, CONTEXT_NAMESPACE, 'callstartedstr');
                    $scope.$digest();
                }
            });
        }
        else if (subtype === 'call_ended') {
            message = UtilFactory.getLocalizedStr($scope.callEndedStr, CONTEXT_NAMESPACE, 'callendedstr').replace('%link%',
                        createHyperlink('', UtilFactory.getLocalizedStr($scope.callEndedLinkTextStr, CONTEXT_NAMESPACE, 'callendedlinktextstr')));
        }
        else if (subtype === 'call_noanswer') {
            message = UtilFactory.getLocalizedStr($scope.callNoAnswerStr, CONTEXT_NAMESPACE, 'callnoanswerstr', {'%username%': $scope.systemEvent.from});
        }
        else if (subtype === 'call_missed') {
            message = UtilFactory.getLocalizedStr($scope.callMissedStr, CONTEXT_NAMESPACE, 'callmissedstr', {'%username%': $scope.systemEvent.from});
        }
        else if (subtype === 'call_ended_unexpectedly') {
            message = UtilFactory.getLocalizedStr($scope.callEndedUnexpectedlyStr, CONTEXT_NAMESPACE, 'callendedunexpectedlystr');
        }
        else if (subtype === 'call_ended_incoming') {
            message = UtilFactory.getLocalizedStr($scope.callEndedIncomingStr, CONTEXT_NAMESPACE, 'callendedincomingstr');
        }
        else if (subtype === 'call_connect_error') {
            message = UtilFactory.getLocalizedStr($scope.callConnectErrorStr, CONTEXT_NAMESPACE, 'callconnecterrorstr',
                {
                  '%errorCode%': $scope.systemEvent.errorCode,
                  '%link%': createHyperlink(UtilFactory.getLocalizedStr($scope.callConnectErrorLinkUrlStr, CONTEXT_NAMESPACE, 'callconnecterrorlinkurlstr'),
                                            UtilFactory.getLocalizedStr($scope.callConnectErrorLinkTextStr, CONTEXT_NAMESPACE, 'callconnecterrorlinktextstr'))
                });
        }
        else if (subtype === 'local_muted') {
            message = UtilFactory.getLocalizedStr($scope.localMutedStr, CONTEXT_NAMESPACE, 'localmutedstr');
        }
        else if (subtype === 'local_unmuted') {
            message = UtilFactory.getLocalizedStr($scope.localUnmutedStr, CONTEXT_NAMESPACE, 'localunmutedstr');
        }
        else if (subtype === 'device_changed') {
            if ($scope.systemEvent.device === undefined) {
                $scope.systemEvent.device = UtilFactory.getLocalizedStr($scope.unpluggedStr, CONTEXT_NAMESPACE, 'unpluggedstr');
            }
            message = UtilFactory.getLocalizedStr($scope.deviceChangedStr, CONTEXT_NAMESPACE, 'devicechangedstr', {'%devicename%': $scope.systemEvent.device});
        }
        else if (subtype === 'inactivity') {
            message = UtilFactory.getLocalizedStr($scope.inactivityStr, CONTEXT_NAMESPACE, 'inactivitystr');
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
        else if (subtype === 'revoked_invite') {
            var gameName = $scope.systemEvent.gameName || UtilFactory.getLocalizedStr($scope.unknownGameStr, CONTEXT_NAMESPACE, 'unknowngamestr');
            message = UtilFactory.getLocalizedStr($scope.inviteRevokedStr, CONTEXT_NAMESPACE, 'inviterevokedstr', {'%username%': $scope.systemEvent.from, '%gametitle%': gameName});
        }

        $scope.message = message;

        $scope.trustAsHtml = function (html) {
            return $sce.trustAsHtml(html);
        };

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

        this.clickLink = function (url) {
            if (subtype === 'call_ended') {
                Origin.voice.showSurvey($scope.systemEvent.channelId);
            }
            else {
                NavigationFactory.asyncOpenUrl(url);
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationHistoryitemSystem
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} system-event Javascript object representing the system message event
     * @param {LocalizedString} userjoinedchatstr "%username% joined the chat"
     * @param {LocalizedString} userleftchatstr "%username% has left the chat"
     * @param {LocalizedString} usersleftchatstr "%username% have left the chat"
     * @param {LocalizedString} useronlinestr "%username% is now online"
     * @param {LocalizedString} userofflinestr "%username% is offline. Messages you send will be delivered when %username% comes online."
     * @param {LocalizedString} userawaystr "%username% is away"
     * @param {LocalizedString} callstartedstr "Call Started"
     * @param {LocalizedString} joinedvoicechatstr "Joined voice chat. Push %pttString% to talk."
     * @param {LocalizedString} mousebuttonstr "Mouse Button %number%"
     * @param {LocalizedString} callendedstr "Call Ended. %link%"
     * @param {LocalizedString} callendedlinktextstr "How did your chat go?"
     * @param {LocalizedString} callnoanswerstr "%username% did not answer"
     * @param {LocalizedString} callmissedstr "Missed call from %username%"
     * @param {LocalizedString} callendedunexpectedlystr "Voice chat unexpectedly ended. Please try your call again."
     * @param {LocalizedString} callendedincomingstr "Call Ended"
     * @param {LocalizedString} callconnecterrorstr "Unable to start voice chat due to issues with your sound drivers. DirectSound error code: %errorCode%. %link%"
     * @param {LocalizedString} callconnecterrorlinktextstr "Learn More"
     * @param {LocalizedString} callconnecterrorlinkurlstr "https://help.ea.com/article/troubleshoot-origin-voice-chat-directsound-errors"
     * @param {LocalizedString} localmutedstr "You are now muted"
     * @param {LocalizedString} localunmutedstr "You are now unmuted"
     * @param {LocalizedString} devicechangedstr "Your microphone is now %devicename%"
     * @param {LocalizedString} unpluggedstr "unplugged"
     * @param {LocalizedString} inactivitystr "Voice chat ended due to inactivity"
     * @param {LocalizedString} emptyroomstr "There\'s no one in your chat room yet. Invite friends into this chat room"
     * @param {LocalizedString} phishingstr "Never share your password with anyone."
     * @param {LocalizedString} youareofflinestr "You are offline"
     * @param {LocalizedString} inviterevokedstr "%username% invited you to play %gametitle%, but it is no longer joinable."
     * @param {LocalizedString} unknowngamestr "Unknown Game"
     * @description
     *
     * origin chatwindow -> conversation -> history item -> system message
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-historyitem-system
     *            system-event="systemEvent"
     *            userjoinedchatstr="%username% joined the chat"
     *            userleftchatstr="%username% has left the chat"
     *            usersleftchatstr="%username% have left the chat"
     *            useronlinestr="%username% is now online"
     *            userofflinestr="%username% is offline. Messages you send will be delivered when %username% comes online."
     *            userawaystr="%username% is away"
     *            callstartedstr="Call Started"
     *            joinedvoicechatstr="Joined voice chat. Push %pttString% to talk."
     *            mousebuttonstr="Mouse Button %number%"
     *            callendedstr="Call Ended. %link%"
     *            callendedlinktextstr="How did your chat go?"
     *            callnoanswerstr="%username% did not answer"
     *            callmissedstr="Missed call from %username%"
     *            callendedunexpectedlystr="Voice chat unexpectedly ended. Please try your call again."
     *            callendedincomingstr="Call Ended"
     *            callconnecterrorstr="Unable to start voice chat due to issues with your sound drivers. DirectSound error code: %errorCode%. %link%"
     *            callconnecterrorlinktextstr="Learn More"
     *            callconnecterrorlinkurlstr="https://help.ea.com/article/troubleshoot-origin-voice-chat-directsound-errors"
     *            localmutedstr="You are now muted"
     *            localunmutedstr="You are now unmuted"
     *            devicechangedstr="Your microphone is now %devicename%"
     *            unpluggedstr="unplugged"
     *            inactivitystr="Voice chat ended due to inactivity"
     *            emptyroomstr="There\'s no one in your chat room yet. Invite friends into this chat room"
     *            phishingstr="Never share your password with anyone."
     *            youareofflinestr="You are offline"
     *            inviterevokedstr="%username% invited you to play %gametitle%, but it is no longer joinable."
     *            unknowngamestr="Unknown Game"
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
                    joinedVoiceChatStr: '@joinedvoicechatstr',
                    mouseButtonStr: '@mousebuttonstr',
                    callEndedStr: '@callendedstr',
                    callEndedLinkTextStr: '@callendedlinktextstr',
                    callNoAnswerStr: '@callnoanswerstr',
                    callMissedStr: '@callmissedstr',
                    callEndedUnexpectedlyStr: '@callendedunexpectedlystr',
                    callEndedIncomingStr: '@callendedincomingstr',
                    callConnectErrorStr: '@callconnecterrorstr',
                    callConnectErrorLinkTextStr: '@callconnecterrorlinktextstr',
                    callConnectErrorLinkUrlStr: '@callconnecterrorlinkurlstr',
                    localMutedStr: '@localmutedstr',
                    localUnmutedStr: '@localunmutedstr',
                    deviceChangedStr: '@devicechangedstr',
                    unpluggedStr: '@unpluggedstr',
                    inactivityStr: '@inactivitystr',
                    emptyRoomStr: '@emptyroomstr',
                    phishingStr: '@phishingstr',
                    youAreOfflineStr: '@youareofflinestr',
                    inviteRevokedStr: '@inviterevokedstr',
                    unknownGameStr: '@unknowngamestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/historyitem/views/system.html'),
                link: function(scope, element, attrs, ctrl) {

                    // User clicked on a hyperlink in the chat history
                    function onLinkClick(event) {
                        var url = $(this).attr('href');
                        event.stopImmediatePropagation();
                        event.preventDefault();
                        ctrl.clickLink(url);
                    }

                    function onDestroy() {
                        $(element).off('click');
                    }

                    $(element).on('click', '.otka', onLinkClick);

                    scope.$on('$destroy', onDestroy);

                }
            };

        });
}());

