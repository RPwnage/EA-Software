/**
* @file areaminimized.js
*/
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubAreaminimizedCtrl($scope, $timeout, ChatWindowFactory, ConversationFactory, RosterDataFactory, SocialHubFactory, UserDataFactory, AppCommFactory, SocialDataFactory) {
        var $element;

        $scope.titlestr = '';
        $scope.presenceObj = RosterDataFactory.getCurrentUserPresence();
        $scope.isInvisible = RosterDataFactory.getIsInvisibile();

        function updateTitle(onlineCount) {
            $scope.titlestr = 'Chat (%1)'.replace('%1', onlineCount);
			if(SocialHubFactory.isWindowMinimized()) {
				$scope.$digest();
			}
        }

        updateTitle(0);

		this.userLoggedIn = function() {
			return SocialDataFactory.userLoggedIn();
		};

        this.setElement = function($el) {
            $element = $el;
        };

        SocialHubFactory.events.on('unminimizeHub', function () {
			$element.find('.origin-social-hub-area-minimized').css('max-height', '0');
        });

        SocialHubFactory.events.on('minimizeHub', function () {
			$timeout(function() {
				$element.find('.origin-social-hub-area-minimized').css('max-height', '100%');

			}, 400);
        });

		this.getOnlineCount = function() {			
			function handleUpdateRosterCount() {
				RosterDataFactory.getRoster('ONLINE').then(function(roster) {			
					updateTitle(Object.keys(roster).length);
				});			
			}
			function handleDeleteRosterCount() {
				RosterDataFactory.getRoster('ONLINE').then(function(roster) {			
					updateTitle(Object.keys(roster).length);
				});			
			}
						
			RosterDataFactory.events.on('socialRosterUpdateONLINE', handleUpdateRosterCount);
			RosterDataFactory.events.on('socialRosterDeleteONLINE', handleDeleteRosterCount);

			RosterDataFactory.getRoster('ONLINE').then(function(roster) {			
				updateTitle(Object.keys(roster).length);
			});			
		};
		
        function handlePresenceChange(newPresence) {
            $scope.presenceObj = newPresence;
			if(SocialHubFactory.isWindowMinimized()) {
				$scope.$digest();
			}
        }

        handlePresenceChange(RosterDataFactory.getCurrentUserPresence());

        function init() {
            if (Origin.auth.isLoggedIn()) {
                RosterDataFactory.events.on('socialPresenceChanged:' + Origin.user.userPid(), handlePresenceChange);
            }
            else {
                Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, function () {
                    RosterDataFactory.events.on('socialPresenceChanged:' + Origin.user.userPid(), handlePresenceChange);
                });
            }

            RosterDataFactory.events.on('visibilityChanged', function (value) {
                $scope.isInvisible = value;
                $scope.$digest();
            });
        }

        if (UserDataFactory.isAuthReady()) {
            init();
        } else {
            AppCommFactory.events.on('auth:ready', init);
        }

		function handleJssdkLogin(login) {
			$scope.userLoggedIn = login;
			$scope.$digest();
		}
		SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubAreaminimized
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * origin social hub -> area minimized
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-areaminimized
     *         ></origin-social-hub-areaminimized>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubAreaminimizedCtrl', OriginSocialHubAreaminimizedCtrl)
        .directive('originSocialHubAreaminimized', function(ComponentsConfigFactory, SocialHubFactory) {
        
            return {
                restrict: 'E',
                controller: 'OriginSocialHubAreaminimizedCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/areaminimized.html'),
                link: function(scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

					scope.userLoggedIn = ctrl.userLoggedIn();
					
                    ctrl.setElement($(element));

                    $(element).find('.origin-social-hub-area-minimized').on('click', function() {                        
                        SocialHubFactory.unminimizeWindow();
                    });
                    
					ctrl.getOnlineCount();
                }
            };
        
        });

} ());
