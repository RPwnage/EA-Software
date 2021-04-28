/**
 * @file notifications.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-notifications';

    function OriginSettingsNotificationsCtrl($scope, $timeout, UtilFactory) {
        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000);

        function assignSettingToScopeSetting(prop) {
            return function(setting) {
                $scope.settings[prop] = setting;
                throttledDigestFunction();

            };
        }

        $scope.chatstr = UtilFactory.getLocalizedStr($scope.chatstr, CONTEXT_NAMESPACE, 'chatstr');
        $scope.notificationstr = UtilFactory.getLocalizedStr($scope.notificationstr, CONTEXT_NAMESPACE, 'notificationstr');
        $scope.soundstr = UtilFactory.getLocalizedStr($scope.soundstr, CONTEXT_NAMESPACE, 'soundstr');
        $scope.incomingstr = UtilFactory.getLocalizedStr($scope.incomingstr, CONTEXT_NAMESPACE, 'incomingstr');
        $scope.incomingvoipstr = UtilFactory.getLocalizedStr($scope.incomingvoipstr, CONTEXT_NAMESPACE, 'incomingvoipstr');
        $scope.gameinvitestr = UtilFactory.getLocalizedStr($scope.gameinvitestr, CONTEXT_NAMESPACE, 'gameinvitestr');
        $scope.groupinvitestr = UtilFactory.getLocalizedStr($scope.groupinvitestr, CONTEXT_NAMESPACE, 'groupinvitestr');
        $scope.friendsactivitystr = UtilFactory.getLocalizedStr($scope.friendsactivitystr, CONTEXT_NAMESPACE, 'friendsactivitystr');
        $scope.friendinstr = UtilFactory.getLocalizedStr($scope.friendinstr, CONTEXT_NAMESPACE, 'friendinstr');
        $scope.friendoutstr = UtilFactory.getLocalizedStr($scope.friendoutstr, CONTEXT_NAMESPACE, 'friendoutstr');
        $scope.friendstartstr = UtilFactory.getLocalizedStr($scope.friendstartstr, CONTEXT_NAMESPACE, 'friendstartstr');
        $scope.friendquitstr = UtilFactory.getLocalizedStr($scope.friendquitstr, CONTEXT_NAMESPACE, 'friendquitstr');
        $scope.friendbroadstr = UtilFactory.getLocalizedStr($scope.friendbroadstr, CONTEXT_NAMESPACE, 'friendbroadstr');
        $scope.friendstopbroadstr = UtilFactory.getLocalizedStr($scope.friendstopbroadstr, CONTEXT_NAMESPACE, 'friendstopbroadstr');
        $scope.gamesstr = UtilFactory.getLocalizedStr($scope.gamesstr, CONTEXT_NAMESPACE, 'gamesstr');
        $scope.achievementstr = UtilFactory.getLocalizedStr($scope.achievementstr, CONTEXT_NAMESPACE, 'achievementstr');
        $scope.downloadstr = UtilFactory.getLocalizedStr($scope.downloadstr, CONTEXT_NAMESPACE, 'downloadstr');
        $scope.failurestr = UtilFactory.getLocalizedStr($scope.failurestr, CONTEXT_NAMESPACE, 'failurestr');
        $scope.installstr = UtilFactory.getLocalizedStr($scope.installstr, CONTEXT_NAMESPACE, 'installstr');

        $scope.underaged = Origin.user.underAge();
        
        /* jshint camelcase: false */
		/*
			Notification settings will remain local to the client until notifications move to the SPA
			So the setting format follows the enum:
                enum AlertMethod
                {
                    TOAST_DISABLED,
                    TOAST_SOUND_ONLY,
                    TOAST_ALERT_ONLY,
                    TOAST_SOUND_ALERT
                };			
		*/
        $scope.settings = {     // put default settings here
            incomingchat : 3,
            incomingvoip : 3,
            gameinvite : 3,
            groupinvite : 3,
			friendin : 3,
			friendout : 3,
			friendstart : 3,
			friendquit : 3,
			friendbroad : 3,
			friendstopbroad : 3,
			achievement : 3,
			download : 3,
			failure : 3,
			install : 3
        };
		
		$scope.spa_to_client_map = {
            incomingchat : "IncomingTextChatNotification",
			incomingvoip : "IncomingVoIPCallNotification",
            gameinvite : "GameInviteNotification",
            groupinvite : "GroupChatInviteNotification",
			friendin : "FriendSigningInNotification",
			friendout : "FriendSigningOutNotification",
			friendstart : "FriendStartsGameNotification",
			friendquit : "FriendQuitsGameNotification",
			friendbroad : "FriendStartBroadcastNotification",
			friendstopbroad : "FriendStopBroadcastNotification",
			achievement : "AchievementNotification",
			download : "FinishedDownloadNotification",
			failure : "DownloadErrorNotification",
			install : "FinishedInstallNotification"
		};
		
		$scope.isSet = function (key, sound) {
			var current = $scope.settings[key];
			if (sound){	// just the low bit
				return ((current === 1) || (current === 3));
			}
			else{
				return (current >= 2);	// on
			}
		};

		/* initialize html based on settings */
		if (Origin.client.isEmbeddedBrowser()) {
			for (var p in $scope.spa_to_client_map){
				if ($scope.spa_to_client_map.hasOwnProperty(p)){
					Origin.client.settings.readSetting($scope.spa_to_client_map[p]).then(assignSettingToScopeSetting(p));
				}
			}
		}
    }

    function originSettingsNotifications(ComponentsConfigFactory) {
        function originSettingsNotificationsLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0];
            
            /* jshint camelcase: false */
            scope.toggleSetting = function (key, sound) {
                var current = scope.settings[key];
                if (sound){	// just the low bit
                    if ((current === 1) || (current === 3)){	// on
                        scope.settings[key] = scope.settings[key] - 1;
                    }
                    else{
                        scope.settings[key] = scope.settings[key] + 1;
                    }
                }
                else{
                    if (current >= 2){	// on
                        scope.settings[key] = scope.settings[key] - 2;
                    }
                    else{
                        scope.settings[key] = scope.settings[key] + 2;
                    }
                }
                
                if (scope.spa_to_client_map[key]){
                    settingsCtrl.triggerSavedToasty();
                    Origin.client.settings.writeSetting(scope.spa_to_client_map[key], scope.settings[key]);
                }
            };

        }    
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
            },
            require: ['^originSettings'],            
            controller: 'OriginSettingsNotificationsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/notifications.html'),
            link: originSettingsNotificationsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsNotifications
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} descriptionstr the description str
     * @description
     *
     * Settings section 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsNotificationsCtrl', OriginSettingsNotificationsCtrl)
        .directive('originSettingsNotifications', originSettingsNotifications);
}());