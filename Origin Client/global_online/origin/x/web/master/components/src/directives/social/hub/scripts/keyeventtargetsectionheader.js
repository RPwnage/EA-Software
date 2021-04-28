/**
 * @file keyeventtarget.js
 */

(function () {
    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubKeyeventtargetsectionheaderCtrl($scope) {
        $scope.hasHighlight = false;
        $scope.isHeaderCollapsed = false;

        function getVisibleTargets () {
                return $('[keytarget]:visible');
            }

        this.getPrevious = function (focused) {
            var previous = focused;
            var keyboardTargets = getVisibleTargets();
            var index = keyboardTargets.index(focused);
            
            if (index > 0) {
                previous = keyboardTargets.get(index-1);
            }
            return previous;
        };

        this.getNext = function (focused) {
            var next = focused;
            var keyboardTargets = getVisibleTargets();
            var index = keyboardTargets.index(focused);
            
            if (index < (keyboardTargets.length-1)) {
                next = keyboardTargets.get(index+1);
            }
            return next;
        };
    }

    /**
    * The directive
    */
    function OriginSocialHubKeyeventtargetsectionheader (AppCommFactory, EventsHelperFactory) {
        
        /**
        * The link 
        */
        function OriginSocialHubKeyeventtargetsectionheaderLink (scope, elem, attrs, ctrl) {
			var keyEventsAPI = ctrl[0]; // keyEvents.js controller
			var rosterController = ctrl[1];
            var thisController = ctrl[2];
            
            $(elem).on('click', function (e) {
                $(elem).focus();
                keyEventsAPI.registerEventHandlers(elem);
                keyEventsAPI.handleLeftClickRosterEvent(e);
            });

            $(elem).on('keydown', function (e) {
                e.preventDefault();
                return false;
            });

            $(elem).on('keyup', function (event) {
                keyEventsAPI.registerEventHandlers(elem);
                keyEventsAPI.handleKeyboardEvent(event, elem);
            });

            function handleSocialKeyupDown () {
                if(scope.hasHighlight) {
                    var next = thisController.getNext(elem);
                    
                    scope.hasHighlight = false;
                    $(next).focus();

                    keyEventsAPI.registerEventHandlers(next);
                }
            }

            function handleSocialKeyupUp () {
                if(scope.hasHighlight) {
                    var prev = thisController.getPrevious(elem);

                    scope.hasHighlight = false;
                    $(prev).focus();

                    keyEventsAPI.registerEventHandlers(prev);
                }
            }
            
            function handleSocialKeyupRight () {
                // If is collapsed, expand
                if(scope.hasHighlight && !$(elem).hasClass('header-visible')) {
                    rosterController.toggleSectionVisible();
                }
            }

            function handleSocialKeyupLeft () {
                if(scope.hasHighlight && $(elem).hasClass('header-visible')) {
                    rosterController.toggleSectionVisible();
                }
            }

            AppCommFactory.events.on('social:setHighlight', function () {
                scope.hasHighlight = keyEventsAPI.isFocused(elem);
            });

            // Gather events
            var handles = [];

            scope.attachHandlers = function () {
				if (!handles.length) {
					handles = [
						AppCommFactory.events.on('social:keyupUp', handleSocialKeyupUp),
						AppCommFactory.events.on('social:keyupDown', handleSocialKeyupDown),
						AppCommFactory.events.on('social:keyupLeft', handleSocialKeyupLeft),
						AppCommFactory.events.on('social:keyupRight', handleSocialKeyupRight)
					];

				} else {
					EventsHelperFactory.attachAll(handles);
				}
            };

            scope.detachHandlers = function () {
                EventsHelperFactory.detachAll(handles);
            };

            /* Reset */
            AppCommFactory.events.on('social:resetRosterDefaults', function () {
                scope.hasHighlight = false;
            });

        }

        return {
            restrict: 'A',
            priority: -1,
            require: ['^originSocialHubKeyevents', '^?originSocialHubRosterSection', 'originSocialHubKeyeventtargetsectionheader'],
            controller: 'OriginSocialHubKeyeventtargetsectionheaderCtrl',
            link: OriginSocialHubKeyeventtargetsectionheaderLink
        };
    }
    
    angular.module('origin-components')
        .controller('OriginSocialHubKeyeventtargetsectionheaderCtrl', OriginSocialHubKeyeventtargetsectionheaderCtrl)
        .directive('originSocialHubKeyeventtargetsectionheader', OriginSocialHubKeyeventtargetsectionheader);
}());