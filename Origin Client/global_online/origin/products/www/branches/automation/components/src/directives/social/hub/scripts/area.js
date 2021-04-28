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
    function OriginSocialHubAreaCtrl($scope, $window, $timeout, SocialHubFactory, SocialDataFactory, ChatWindowFactory, UtilFactory, AuthFactory, LayoutStatesFactory, EventsHelperFactory, RosterDataFactory, NavigationFactory, ObserverFactory) {
        var $element,
            self = this,
            handles = [],
            popoutHubAreaWindow = null,
            deregisterSocialPresenceMenuVisibleListener;

        var CONTEXT_NAMESPACE = 'origin-social-hub-area';

        // for now assume that if we have extra parameters on the window it's an OIG window
        $scope.igoContext = window.oigParams ? true : false;

        $scope.notAvailableLoc = UtilFactory.getLocalizedStr($scope.notAvailableStr, CONTEXT_NAMESPACE, 'notavailablestr');
        $scope.playingLoc = UtilFactory.getLocalizedStr($scope.playingStr, CONTEXT_NAMESPACE, 'playingstr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcastingStr, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.minimizeLoc = UtilFactory.getLocalizedStr($scope.minimizeStr, CONTEXT_NAMESPACE, 'minimize-str');
        $scope.openInWindowLoc = UtilFactory.getLocalizedStr($scope.openInWindowStr, CONTEXT_NAMESPACE, 'open-in-window-str');
        $scope.chatSettingsLoc = UtilFactory.getLocalizedStr($scope.chatSettingsStr, CONTEXT_NAMESPACE, 'chat-settings-str');
        $scope.closeAllChatLoc = UtilFactory.getLocalizedStr($scope.closeAllChatStr, CONTEXT_NAMESPACE, 'close-all-chat-str');

        $scope.margin = 0;
        $scope.optionsContextmenuVisible = false;
        $scope.hubContextmenuCallbacks = {};
        $scope.isMinimized = false;
        $scope.isVoiceSupported = Origin.voice.supported();

        this.setContentClass = function(docked) {
            var content = document.querySelector('.origin-content');
            if (content) {
                if (docked && !SocialHubFactory.isWindowPoppedOut() && !$scope.isMinimized) {
                    content.classList.add('origin-content-isdocked');
                    LayoutStatesFactory.setEmbeddedChat(true);
                } else {
                    content.classList.remove('origin-content-isdocked');
                    LayoutStatesFactory.setEmbeddedChat(false);
                }
            }
        };

        function onXmppConnectionChanged() {
            if (Origin.xmpp.isConnected()) {
                RosterDataFactory.getRosterUser(Origin.user.userPid()).then(function (user) {
                    if (!!user) {
                        ObserverFactory.create(user._presence)
                            .defaultTo({ jid: Origin.user.userPid(), presenceType: '', show: 'UNAVAILABLE', gameActivity: {} })
                            .attachToScope($scope, 'presence');
                    }
                });
            }
        }

        // listen for xmpp connect/disconnect
        ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
            .defaultTo({isConnected: Origin.xmpp.isConnected()})
            .attachToScope($scope, 'xmppConnection')
            .attachToScope($scope, onXmppConnectionChanged);

        $scope.isPlayingGame = function () {
            return !!$scope.presence && !!$scope.presence.gameActivity && !!$scope.presence.gameActivity.gameTitle && $scope.presence.gameActivity.gameTitle.length;
        };

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
            self.setContentClass(true);
            $timeout(function() { $scope.$digest(); }, 0, false);
            $timeout(function() { RosterDataFactory.updateRosterViewport(); }, 1000, false);
        }

        function onMinimizeHub() {
            self.setContentClass(false);
            $scope.isMinimized = true;
            $timeout(function () { $scope.$digest(); }, 0, false);
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

            window._openPopOutWindow('socialhub', '', '', size).then(function(childWindow){
                popoutHubAreaWindow = childWindow;
            

                popoutHubAreaWindow.onunload = function() {
                    // Add this check here so the code isn't run on initialization
                    if(popoutHubAreaWindow.location.href !== "about:blank") {
                        SocialHubFactory.unpopOutWindow();
                    }
                };

                // This is for web
                window.onunload = function() {
                    popoutHubAreaWindow.close();
                    popoutHubAreaWindow = null;
                };

                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.events.on(Origin.events.CLIENT_POP_OUT_CLOSED, function(url) {
                        // C++ closed this window
                        if (!!popoutHubAreaWindow && popoutHubAreaWindow.url === url) {
                            popoutHubAreaWindow.close();
                            popoutHubAreaWindow = null;
                            SocialHubFactory.unpopOutWindow();
                        }
                    });
                }
            });
            self.setContentClass(false);
            if ($scope.updateMargin) {
                $scope.updateMargin();
            }
        }

        function onUnpopOutHub() {
            self.setContentClass(true);
            if ($scope.updateMargin) {
                $scope.updateMargin();
            }
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

        function onFocusOnFriendsList() {
            if ($scope.isMinimized) {
                SocialHubFactory.unminimizeWindow();
            }
        }

        function destroy() {
            EventsHelperFactory.detachAll(handles);
            deregisterSocialPresenceMenuVisibleListener();
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
        $scope.toggleOptionsContextmenu = function ($event) {
            $scope.optionsContextmenuVisible = !$scope.optionsContextmenuVisible;
            $event.stopPropagation();
        };

        $scope.hubContextmenuCallbacks.popOutWindow = function ($event) {
            SocialHubFactory.popOutWindow();
            $scope.optionsContextmenuVisible = false;
            $event.stopImmediatePropagation();

        };

        $scope.hubContextmenuCallbacks.minimizeHub = function ($event) {
            $scope.isMinimized = true;
            SocialHubFactory.minimizeWindow();
            $scope.optionsContextmenuVisible = false;
            $event.stopImmediatePropagation();
        };

        $scope.hubContextmenuCallbacks.goToSettings = function ($event) {
            if (Origin.client.oig.IGOIsActive()) {
                // Open OIG settings window
                NavigationFactory.openIGOSPA('SETTINGS', 'voice');
            } else {
                NavigationFactory.goSettingsPage('voice');
            }
            $scope.optionsContextmenuVisible = false;
            $event.stopImmediatePropagation();

        };

        $scope.hubContextmenuCallbacks.closeAllConversations = function ($event) {
            ChatWindowFactory.closeWindow();
            $scope.optionsContextmenuVisible = false;
            $event.stopImmediatePropagation();

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

        handles[handles.length] = Origin.events.on(Origin.events.CLIENT_SOCIAL_FOCUSONFRIENDSLIST, onFocusOnFriendsList);

        $scope.$on('$destroy', destroy);

        // Hides options menu when presence menu opens
        deregisterSocialPresenceMenuVisibleListener = $scope.$on('originSocialPresenceMenuVisible', onOriginSocialPresenceMenuVisible);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubArea
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} notavailablestr "Your Friends and Conversations are not available in offline mode."
     * @param {LocalizedString} playingstr "Playing"
     * @param {LocalizedString} broadcastingstr "Broadcasting"
     * @param {LocalizedString} minimize-str minimize window
     * @param {LocalizedString} open-in-window-str open in new window, popout
     * @param {LocalizedString} chat-settings-str chat settings
     * @param {LocalizedString} close-all-chat-str close all chat tabs
     * @description
     *
     * origin social hub -> area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-area
     *            notavailablestr="Your Friends and Conversations are not available in offline mode."
     *            playingstr="Playing"
     *            broadcastingstr="Broadcasting"
     *         ></origin-social-hub-area>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubAreaCtrl', OriginSocialHubAreaCtrl)
        .directive('originSocialHubArea', function(ComponentsConfigFactory, SocialHubFactory, AuthFactory, KeyEventsHelper, ActiveElementHelper) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubAreaCtrl',
                scope: {
                    notAvailableStr: '@notavailablestr',
                    playingStr: '@playingstr',
                    broadcastingStr: '@broadcastingstr',
                    standalone: '@',
                    minimizeStr: '@',
                    openInWindowStr: '@',
                    chatSettingsStr: '@',
                    closeAllChatStr: '@'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/area.html'),
                link: function(scope, element, attrs, ctrl) {

                    function isDocked() {
                       return angular.element('.origin-social-hub-area-header').length && angular.element('.origin-social-hub-area-header').css('content').indexOf('docked') > -1;
                    }
                    attrs = attrs;

                    scope.userLoggedIn = ctrl.userLoggedIn();

                    ctrl.setContentClass(scope.userLoggedIn);
                    scope.clientOffline = !AuthFactory.isClientOnline();

                    ctrl.setElement(angular.element(element));


                    //the check for the CSS is not valid until the next event loop
                    //if its not docked, the showMinimized
                    setTimeout(function() {
                        if(!isDocked() && (!scope.isPoppedOut() && !scope.igoContext)) {
                            SocialHubFactory.minimizeWindow();
                        }
                    }, 0);

                    // set isDocked on mouseenter of header to assure element
                    // is loaded to DOM and avoid DOM flicker if set on click
                    angular.element(element).on('mouseenter', '.origin-social-hub-area-header', function () {
                        scope.isDocked = isDocked();
                    });

                    angular.element(element).on('click', '.origin-social-hub-area-header, .origin-social-hub-area-structure, .origin-social-hub-area-richpresence', function (event) {
                        if (SocialHubFactory.isWindowPoppedOut() || scope.igoContext) { return; }
                        var $target = angular.element(event.target);
                        if(($target.hasClass('origin-social-hub-area-header') || $target.hasClass('origin-social-hub-area-structure') || $target.hasClass('origin-social-hub-area-richpresence'))) {
                            SocialHubFactory.minimizeWindow();
                        }
                    });

                    // Cancel options menu by clicking elsewhere
                    angular.element(document).on('click', function (e) {
                        var $target = angular.element(e.target);
                        if(!$target.hasClass('origin-social-hub-area-options-menu-icon') && scope.optionsContextmenuVisible) {
                            ctrl.closeOptionsContextmenu();
                        }
                    });

                    function minimizeWindowFromKeyboard() {
                        if(!scope.isDocked && !SocialHubFactory.isWindowPoppedOut()) {
                            SocialHubFactory.minimizeWindow();
                            return true;
                        }
                    }



                    function closeMenuByKeyboard() {
                        if (ActiveElementHelper.isActiveElementMenuOrMenuItem()) {
                            var menuRoot = angular.element(document.activeElement).closest('[keytarget=contextmenu]');
                            if (menuRoot.hasClass('origin-social-hub-area-options')) {
                                ctrl.closeOptionsContextmenu();
                                menuRoot.focus();
                                return false;
                            }
                        }
                    }

                    /**
                     * listens for key presses to minimize the roster window
                     * @return true if handled
                     */
                    function handleMinimizeWindowShortcut(event) {
                        var modifierPressed = (Origin.utils.os() === 'MAC') ? event.metaKey && event.ctrlKey : event.ctrlKey && event.altKey;
                        if (modifierPressed) {
                            return KeyEventsHelper.mapEvents(event, {
                                'M': minimizeWindowFromKeyboard
                            });
                        }
                    }

                    //we handle this event in the key down to handle the command key on the mac which does not work with keyup
                    function handleKeyDown(event) {
                        if (handleMinimizeWindowShortcut(event)) {
                            return false;
                        }
                    }

                    function handleKeyUp(event) {
                        return KeyEventsHelper.mapEvents(event, {
                            'left': closeMenuByKeyboard,
                            'numpadSubtract': closeMenuByKeyboard,
                            'escape': closeMenuByKeyboard,
                            'backspace': closeMenuByKeyboard
                        });
                    }

                    function onDestroy() {
                        angular.element(element).off('keydown', handleKeyDown);
                        angular.element(element).off('keyup', handleKeyUp);

                    }

                    scope.$on('$destroy', onDestroy);
                    angular.element(element).on('keydown', handleKeyDown);
                    angular.element(element).on('keyup', handleKeyUp);


                    scope.setSitestripeMargin = function(margin) {
                        scope.margin = margin;
                        scope.updateMargin();
                    };

                    scope.updateMargin = function() {
                        if (!(SocialHubFactory.isWindowPoppedOut() || scope.igoContext)) {
                            angular.element('.origin-social-hub-area').css('margin-top', scope.margin);
                        } else {
                            angular.element('.origin-social-hub-area').css('margin-top', 0);
                        }
                    };

                }

            };

        });

}());
