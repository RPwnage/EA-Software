/**
 * @file areaminimized.js
 */
(function() {

    'use strict';

    /**
     * The controller
     */
    function OriginSocialHubAreaminimizedCtrl($scope, $timeout, ChatWindowFactory, ConversationFactory, RosterDataFactory, SocialHubFactory, AuthFactory, ComponentsLogFactory, SocialDataFactory, EventsHelperFactory) {
        var $element,
        handles = [];

        $scope.titlestr = '';
        $scope.presenceObj = RosterDataFactory.getCurrentUserPresence();
        $scope.isInvisible = RosterDataFactory.getIsInvisible();

        $scope.isPoppedOut = function() {
            return SocialHubFactory.isWindowPoppedOut();
        };

        function updateTitle(onlineCount) {
            $scope.titlestr = 'Chat (%1)'.replace('%1', onlineCount);
            if (SocialHubFactory.isWindowMinimized()) {
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

        function onUnminimizeHub() {
            $element.find('.origin-social-hub-area-minimized').css('max-height', '0');
        }

        function onMinimizeHub() {
            $timeout(function() {
                $element.find('.origin-social-hub-area-minimized').css('max-height', '100%');

            }, 400);
        }

        handles[handles.length] = SocialHubFactory.events.on('unminimizeHub', onUnminimizeHub);
        handles[handles.length] = SocialHubFactory.events.on('minimizeHub', onMinimizeHub);

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

            handles[handles.length] = RosterDataFactory.events.on('socialRosterUpdateONLINE', handleUpdateRosterCount);
            handles[handles.length] = RosterDataFactory.events.on('socialRosterDeleteONLINE', handleDeleteRosterCount);

            RosterDataFactory.getRoster('ONLINE').then(function(roster) {
                updateTitle(Object.keys(roster).length);
            });
        };

        function handlePresenceChange(newPresence) {
            $scope.presenceObj = newPresence;
			if ( (newPresence.presenceType === 'UNAVAILABLE') || (newPresence.show === 'OFFLINE')) {
				$scope.isInvisible = true;
			} else {
				$scope.isInvisible = false;
			}
			
            if (SocialHubFactory.isWindowMinimized()) {
                $scope.$digest();
            }
        }

        handlePresenceChange(RosterDataFactory.getCurrentUserPresence());

        function handleRosterCleared() {
            updateTitle(0);
            if (SocialHubFactory.isWindowMinimized()) {
                $scope.$digest();
            }
        }
        handles[handles.length] = RosterDataFactory.events.on('rosterCleared', handleRosterCleared);

        function onAuthChange(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                handles[handles.length] = RosterDataFactory.events.on('socialPresenceChanged:' + Origin.user.userPid(), handlePresenceChange);
            }
        }

        function onVisibilityChanged(value) {
            $scope.isInvisible = value;
            $scope.$digest();
        }

        function init(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                onAuthChange(loginObj);
            } else {
                handles[handles.length] = AuthFactory.events.on('myauth:change', onAuthChange);
            }

            handles[handles.length] = RosterDataFactory.events.on('visibilityChanged', onVisibilityChanged);
        }

        function authReadyError(error) {
            ComponentsLogFactory.error('OriginSocialHubAreaminimizedCtrl -  authReadyError:', error.message);
        }

        AuthFactory.waitForAuthReady()
            .then(init)
            .catch(authReadyError);


        function handleJssdkLogin(login) {
            $scope.userLoggedIn = login;
           // $scope.$digest();
        }
        handles[handles.length] = SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }

        window.addEventListener('unload', destroy);

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

}());