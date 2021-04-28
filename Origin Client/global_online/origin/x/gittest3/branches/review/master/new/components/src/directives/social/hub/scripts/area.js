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
    function OriginSocialHubAreaCtrl($scope, $window, $timeout, SocialHubFactory, SocialDataFactory, ChatWindowFactory) {
        var $element,
            self = this;

        this.setContentClass = function(docked) {
            var content = document.querySelector('.origin-content');
            if (docked) {
                content.classList.add('origin-content-isdocked');
            } else {
                content.classList.remove('origin-content-isdocked');
            }
        };

        $scope.optionsContextmenuVisible = false;
        $scope.hubContextmenuCallbacks = {};

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

        SocialHubFactory.events.on('unminimizeHub', function () {
			$element.find('.l-origin-social-hub-area').css('max-height', '100%');
        });

        SocialHubFactory.events.on('minimizeHub', function () {
			$element.find('.l-origin-social-hub-area').css('max-height', '0');
        });

        SocialHubFactory.events.on('popOutHub', function () {

            // console.log('this.popoutHubArea');
            var windowTop = window.screenY + top - 10;
            var windowLeft = window.screenX + left - 20;

            var popoutHubAreaWindow = null;
            if (typeof OriginIGO !== 'undefined') {
                popoutHubAreaWindow = window.openIGOWindow('socialhub', '');
            }
            else {
                popoutHubAreaWindow = window.open('social-hub.html', 'socialhub', 'location=0,toolbar=no,menubar=no,dialog=yes,top='+windowTop+',left='+windowLeft+',width='+width+ ',height='+ (height*0.9));
            }

            popoutHubAreaWindow.onbeforeunload = function() {
                SocialHubFactory.unpopOutWindow();
            };
            window.onbeforeunload = function() {
                popoutHubAreaWindow.close();
            };

			$element.hide();
            self.setContentClass(false);
        });

        SocialHubFactory.events.on('unpopOutHub', function () {
            if (!SocialHubFactory.isWindowMinimized() && !SocialHubFactory.isWindowPoppedOut()) {
				$element.show();

            } else {
				$element.hide();

            }
            self.setContentClass(true);
        });

		function handleJssdkLogin(login) {
			$scope.userLoggedIn = login;
			$scope.$digest();
			if (login) {
				$timeout(function() { setHubAreaDimensions(); }, 0);
            }
            self.setContentClass(login);
		}
		SocialDataFactory.events.on('jssdkLogin', handleJssdkLogin);

        // Context Menu Functions
        $scope.toggleOptionsContextmenu = function () {
            $scope.optionsContextmenuVisible = !$scope.optionsContextmenuVisible;
        };

        $scope.hubContextmenuCallbacks.popOutWindow = function () {
            $window.alert('I"m a pop out window too! (But not the one you"re looking for, am I right?)');
        };

        $scope.hubContextmenuCallbacks.minimizeHub = function () {
            if(!$scope.isDocked) {                         
                SocialHubFactory.minimizeWindow();
            }
        };

        $scope.hubContextmenuCallbacks.goToSettings = function () {
            $window.alert('I"ll get to making a settings page riiiiight away');
        };

        this.closeAllConversations = function () {
            ChatWindowFactory.events.fire('closeChatWindow', {});
        };

        this.closeOptionsContextmenu = function () {
            $scope.optionsContextmenuVisible = false;
            $scope.$digest();
        };

        // Hides options menu when presence menu opens
        $scope.$on('originSocialPresenceMenuVisible', function () {
            $scope.optionsContextmenuVisible = false;
            $scope.$digest();
        });
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubArea
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * origin social hub -> area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-area
     *         ></origin-social-hub-area>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubAreaCtrl', OriginSocialHubAreaCtrl)
        .directive('originSocialHubArea', function(ComponentsConfigFactory, SocialHubFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubAreaCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/area.html'),
                link: function(scope, element, attrs, ctrl) {
                    attrs = attrs;

					scope.userLoggedIn = ctrl.userLoggedIn();
                    ctrl.setContentClass(scope.userLoggedIn);
                    ctrl.setElement($(element));

                    // set isDocked on mouseenter of header to assure element
                    // is loaded to DOM and avoid DOM flicker if set on click
                    $(element).on('mouseenter', '.origin-social-hub-area-header', function () {
                        scope.isDocked = $('.origin-social-hub-area-header').css('content').indexOf('docked') > -1;
                    });

                    $(element).on('click', '.origin-social-hub-area-header', function (event) {
                        var $target = $(event.target);
                        if($target.hasClass('origin-social-hub-area-header') && !scope.isDocked) {							
                            SocialHubFactory.minimizeWindow();
                        }
                    });

                    $(window).bind('resize', function () {
                        scope.isDocked = $('.origin-social-hub-area-header').css('content').indexOf('docked') > -1;
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
                }

            };

        });

}());

