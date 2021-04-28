/**
 * @file keyeventtarget.js
 */

(function () {
    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubKeyeventtargetcontextmenuitemCtrl($scope) {
        $scope.hasHighlight = false;

        function getVisibleTargets () {
            return $('[origin-social-hub-keyeventtargetcontextmenuitem][keytarget]:visible');
        }

        this.getPrevious = function (focused) {
            var previous = focused;
            var $keyboardContextmenuTargets = getVisibleTargets();
            var index = $keyboardContextmenuTargets.index(focused);
            
            if (index > 0) {
                previous = $keyboardContextmenuTargets.get(index-1);
            } else {
                previous = $keyboardContextmenuTargets.last();
            }
            return previous;
        };

        this.getNext = function (focused) {
            var next = focused;
            var $keyboardContextmenuTargets = getVisibleTargets();
            var index = $keyboardContextmenuTargets.index(focused);

            if (index < ($keyboardContextmenuTargets.length-1)) {
                next = $keyboardContextmenuTargets.get(index+1);
            } else if (index === $keyboardContextmenuTargets.length-1) {
                next = $keyboardContextmenuTargets.eq(0);
            }
            return next;
        };

    }

    /**
    * The directive
    */
    function OriginSocialHubKeyeventtargetcontextmenuitem (AppCommFactory, EventsHelperFactory) {
        
        /**
        * The link 
        */
        function OriginSocialHubKeyeventtargetcontextmenuitemLink (scope, elem, attrs, ctrl) {
			var keyEventsAPI = ctrl[0]; // keyEvents.js controller
			var thisController = ctrl[1]; // this controller

            /* Contextmenu capture events
            * --------------------------
            */
            $(elem).on('keydown', function (e) {
                e.preventDefault();
                return false;
            });

            $(elem).on('keyup', function (event) {
                $(elem).focus();
                keyEventsAPI.registerEventHandlers(elem);
                keyEventsAPI.handleKeyboardEvent(event, elem); 
            });

            $(elem).on('mouseenter', function () {
                if(!keyEventsAPI.isFocused(elem)) {
                    $(elem).focus();

                    keyEventsAPI.handleMouseenterEvent();
                }
                
            });

            /* Contextmenu down/up/enter 
            * --------------------------
            */

            function handleSocialKeyupUp () {
                var prev = thisController.getPrevious(elem);
                $(prev).focus();
            }

            function handleSocialKeyupDown () {
                var next = thisController.getNext(elem);
                $(next).focus();
            }

            function handleSocialKeyupEnter () {
                if(scope.hasHighlight || keyEventsAPI.isFocused(elem)) {
                    AppCommFactory.events.fire('social:triggerEnterEvent', { 'targetType': 'contextmenuitem', 'thisItem' : elem });
                }         
            }

            function handleSocialSetHighlight () {
                scope.hasHighlight = keyEventsAPI.isFocused(elem);
            }

            // Gather events
            var handles = [];

            scope.attachHandlers = function () {
				if (!handles.length) {
					handles = [
						AppCommFactory.events.on('social:keyupUp', handleSocialKeyupUp),
						AppCommFactory.events.on('social:keyupDown', handleSocialKeyupDown),
						AppCommFactory.events.on('social:keyupEnter', handleSocialKeyupEnter),

						AppCommFactory.events.on('social:setHighlight', handleSocialSetHighlight)
					];
				} else {
					EventsHelperFactory.attachAll(handles);
				}
            };

            scope.detachHandlers = function () {
                EventsHelperFactory.detachAll(handles);
            };

        }

        return {
            restrict: 'A',
            priority: -1,
            scope: true,
            require: ['^originSocialHubKeyevents', 'originSocialHubKeyeventtargetcontextmenuitem'],
            controller: 'OriginSocialHubKeyeventtargetcontextmenuitemCtrl',
            link: OriginSocialHubKeyeventtargetcontextmenuitemLink
        };
    }
    
    angular.module('origin-components')
        .controller('OriginSocialHubKeyeventtargetcontextmenuitemCtrl', OriginSocialHubKeyeventtargetcontextmenuitemCtrl)
        .directive('originSocialHubKeyeventtargetcontextmenuitem', OriginSocialHubKeyeventtargetcontextmenuitem);
}());