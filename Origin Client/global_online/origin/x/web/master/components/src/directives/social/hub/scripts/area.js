/**
 * Created by tirhodes on 2015-02-19.
 * @file area.js
 */
(function () {

    'use strict';
    var top,left,width,height;

    /**
    * The controller
    */
    function OriginSocialHubAreaCtrl($scope, $window, $timeout, SocialHubFactory, SocialDataFactory, ChatWindowFactory, UtilFactory, AuthFactory, EventsHelperFactory) {
        var $element,
            self = this,
            handles = [],
            popoutHubAreaWindow = null;

        var CONTEXT_NAMESPACE = 'origin-social-hub-area';

        $scope.offlineCtaTitleLoc = UtilFactory.getLocalizedStr($scope.offlineCtaTitleStr, CONTEXT_NAMESPACE, 'offlinectatitlestr');
        $scope.offlineCtaDescLoc = UtilFactory.getLocalizedStr($scope.offlineCtaDescStr, CONTEXT_NAMESPACE, 'offlinectadescstr');
        $scope.offlineCtaButtonLoc = UtilFactory.getLocalizedStr($scope.offlineCtaButtonStr, CONTEXT_NAMESPACE, 'offlinectabuttonstr');

        this.setContentClass = function(docked) {
            var content = document.querySelector('.origin-content');
            if (content) {
                if (docked && !SocialHubFactory.isWindowPoppedOut()) {
                    content.classList.add('origin-content-isdocked');
                } else {
                    content.classList.remove('origin-content-isdocked');
                }
            }
        };

        $scope.optionsContextmenuVisible = false;
        $scope.hubContextmenuCallbacks = {};
        $scope.isMinimized = false;

        $scope.isPoppedOut = function() {
            return SocialHubFactory.isWindowPoppedOut();
        };

        this.setElement = function($el) {
            $element = $el;
            setHubAreaDimensions();
        };

        this.userLoggedIn = function() {
            return SocialDataFactory.userLoggedIn();
        };

        function setHubAreaDimensions() {
            if (!!$element) {
                var $hubArea = $element.children('.origin-social-hub-area');
                if ($hubArea.length && !!$hubArea.offset()) {
                    top = $hubArea.offset().top;
                    left = $hubArea.offset().left;
                    width = $hubArea.width();
                    height = $hubArea.height();
                }
            }
        }

        function onUnminimizeHub() {
            $scope.isMinimized = false;
            $timeout(function() { $scope.$digest(); }, 0, false);
        }

        function onMinimizeHub() {
            $scope.isMinimized = true;
        }

        function onFocusHub() {
            if (popoutHubAreaWindow) {
                window._moveWindowToFront(popoutHubAreaWindow);
            }
        }

        function onPopOutHub() {
            var size = {
                'top' : window.screenY + top - 10,
                'left' : window.screenX + left - 20,
                'width' : width,
                'height' : (height*0.9)
            };

            popoutHubAreaWindow = window._openPopOutWindow('socialhub', '', size);

            popoutHubAreaWindow.onbeforeunload = function() {
                SocialHubFactory.unpopOutWindow();
            };

            // This is for web
            window.onunload = function() {
                popoutHubAreaWindow.close();
                popoutHubAreaWindow = null;
            };

            if (window.OriginPopOut) {
                window.OriginPopOut.popOutClosed.connect(function(url) {
                    // C++ closed this window
                    if (!!popoutHubAreaWindow && popoutHubAreaWindow.url === url) {
                        popoutHubAreaWindow.close();
                        popoutHubAreaWindow = null;
                        SocialHubFactory.unpopOutWindow();
                    }
                });
            }

            self.setContentClass(false);
        }

        function onUnpopOutHub() {
            self.setContentClass(true);
            $scope.$digest();
        }

        function onOriginSocialPresenceMenuVisible() {
            $scope.optionsContextmenuVisible = false;
            $scope.$digest();
        }

        function onClientOnlineStateChanged(onlineObj) {
            $scope.clientOffline = (onlineObj && !onlineObj.isOnline);
            $scope.$digest();
        }

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }

        //listen for connection change
        handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        function handleJssdkLogin(login) {
            $scope.userLoggedIn = login;
           // $scope.$digest();
            if (login) {
                $scope.clientOffline = false;
                $timeout(function() { setHubAreaDimensions(); }, 0);
            }
            self.setContentClass(login);
        }

        // Context Menu Functions
        $scope.toggleOptionsContextmenu = function () {
            $scope.optionsContextmenuVisible = !$scope.optionsContextmenuVisible;
        };

        $scope.hubContextmenuCallbacks.popOutWindow = function () {
            SocialHubFactory.popOutWindow();
        };

        $scope.hubContextmenuCallbacks.minimizeHub = function () {
            if(!$scope.isDocked) {
                $scope.isMinimized = true;
                SocialHubFactory.minimizeWindow();
            }
        };

        $scope.hubContextmenuCallbacks.goToSettings = function () {
            $window.alert('I"ll get to making a settings page riiiiight away');
        };

        this.closeAllConversations = function () {
            ChatWindowFactory.closeWindow();
        };

        this.closeOptionsContextmenu = function () {
            $scope.optionsContextmenuVisible = false;
            $scope.$digest();
        };

        handles[handles.length] = SocialHubFactory.events.on('minimizeHub', onMinimizeHub);
        handles[handles.length] = SocialHubFactory.events.on('unminimizeHub', onUnminimizeHub);
        handles[handles.length] = SocialHubFactory.events.on('popOutHub', onPopOutHub);
        handles[handles.length] = SocialHubFactory.events.on('unpopOutHub', onUnpopOutHub);
        handles[handles.length] = SocialHubFactory.events.on('focusHub', onFocusHub);
        handles[handles.length] = SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);

        $scope.$on('$destroy', destroy);

        // Hides options menu when presence menu opens
        handles[handles.length] = $scope.$on('originSocialPresenceMenuVisible', onOriginSocialPresenceMenuVisible);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubArea
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} offlinectatitlestr "You're Offline"
     * @param {LocalizedString} offlinectadescstr "We can't load your friends when you're offline."
     * @param {LocalizedString} offlinectabuttonstr "Go Online"
     * @param {string} sets this directive as standalone
     * @description
     *
     * origin social hub -> area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-area
     *            offlinectatitlestr="You're Offline"
     *            offlinectadescstr="We can't load your friends when you're offline."
     *            offlinectabuttonstr="Go Online"
     *            standalone="true"
     *         ></origin-social-hub-area>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubAreaCtrl', OriginSocialHubAreaCtrl)
        .directive('originSocialHubArea', function(ComponentsConfigFactory, SocialHubFactory, AuthFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubAreaCtrl',
                scope: {
                    offlineCtaTitleStr: '@offlinectatitlestr',
                    offlineCtaDescStr: '@offlinectadescstr',
                    offlineCtaButtonStr: '@offlinectabuttonstr',
                    standalone: '@'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/area.html'),
                link: function(scope, element, attrs, ctrl) {
                    attrs = attrs;

                    scope.userLoggedIn = ctrl.userLoggedIn();

                    ctrl.setContentClass(scope.userLoggedIn);
                    scope.clientOffline = !AuthFactory.isClientOnline();

                    ctrl.setElement($(element));

                    // set isDocked on mouseenter of header to assure element
                    // is loaded to DOM and avoid DOM flicker if set on click
                    $(element).on('mouseenter', '.origin-social-hub-area-header', function () {
                        scope.isDocked = $('.origin-social-hub-area-header').length && $('.origin-social-hub-area-header').css('content').indexOf('docked') > -1;
                    });

                    $(element).on('click', '.origin-social-hub-area-header', function (event) {
                        if (SocialHubFactory.isWindowPoppedOut()) {return;}
                        var $target = $(event.target);
                        if($target.hasClass('origin-social-hub-area-header') && !scope.isDocked) {
                            SocialHubFactory.minimizeWindow();
                        }
                    });

                    $(window).bind('resize', function () {
                        scope.isDocked = $('.origin-social-hub-area-header').length && $('.origin-social-hub-area-header').css('content').indexOf('docked') > -1;
                    });

                    // Cancel options menu by clicking elsewhere
                    $(document).on('click', function (e) {
                        var $target = $(e.target);
                        if(!$target.hasClass('origin-social-hub-area-options-menu-icon') && scope.optionsContextmenuVisible) {
                            ctrl.closeOptionsContextmenu();
                        }
                    });

                    // must capture click here to avoid 'digest cycle already
                    // in progress' errors from firing new event on ng-click()
                    $(element).on('click', '.origin-social-hub-area-options .contextmenu-close-all', function () {
                        ctrl.closeAllConversations();
                    });

                    // Listen for clicks on the "offline mode" cta button
                    $(element).on('buttonClicked', 'origin-social-hub-cta', function () {
                        Origin.client.onlineStatus.goOnline();
                    });

                }

            };

        });

}());
