(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationToolbarCtrl($scope, $interval) {
        var timer, seconds, minutes, hours;

        $scope.offline = false;
        $scope.inVoip = false;
        $scope.calltimestr = '00:00';
        $scope.voiceSupported = Origin.voice.supported();

        function startCallTimer() {
            if (!angular.isDefined(timer)) {
                seconds = 0;
                minutes = 0;
                hours = 0;

                timer = $interval(function () {

                    ++seconds;
                    if (seconds >= 60) {
                        seconds = 0;
                        minutes++;
                        if (minutes >= 60) {
                            minutes = 0;
                            ++hours;
                        }
                    }

                    if (hours > 0) {
                        $scope.calltimestr = hours + ':' + (minutes < 10 ? ('0'+minutes) : minutes) + ':' + (seconds < 10 ? ('0'+seconds) : seconds);
                    }
                    else {
                        $scope.calltimestr = (minutes < 10 ? ('0'+minutes) : minutes) + ':' + (seconds < 10 ? ('0'+seconds) : seconds);
                    }

                    $scope.$digest();

                }, 1000);
            }
        }

        function endCallTimer() {
            if (angular.isDefined(timer)) {
                $interval.cancel(timer);
                timer = undefined;
                $scope.calltimestr = '00:00';
            }
        }

        function onClientOnlineStateChanged(online) {
            $scope.offline = !online;
            $scope.$digest();
        }

        function onDestroy() {
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
        }

        //listen for connection change (for embedded)
        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
        $scope.$on('$destroy', onDestroy);

        this.onVoipButtonClick = function () {
            $scope.inVoip = !$scope.inVoip;

            if ($scope.inVoip) {
                startCallTimer();
            }
            else {
                endCallTimer();
            }

            $scope.$digest();
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationToolbar
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @description
     *
     * origin chatwindow -> conversation -> toolbar
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-toolbar
     *            conversation="conversation"
     *         ></origin-social-chatwindow-conversation-toolbar>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationToolbarCtrl', OriginSocialChatwindowConversationToolbarCtrl)
        .directive('originSocialChatwindowConversationToolbar', function(ComponentsConfigFactory, NavigationFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationToolbarCtrl',
                scope: {
                    conversation: '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/toolbar.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    // clicked on the voip button
                    $(element).on('click', '.origin-social-conversation-toolbar-voipicon', function () {
                        ctrl.onVoipButtonClick();
                    });

                    // clicked on the settings button
                    $(element).on('click', '.origin-social-conversation-toolbar-settingsicon', function () {
                        NavigationFactory.goSettings();
                    });

                    // clicked on the invite button
                    $(element).on('click', '.origin-social-conversation-toolbar-inviteicon', function () {
                        window.alert('FEATURE NOT IMPLEMENTED - should bring up invite window');
                    });

                }
            };

        });
}());

