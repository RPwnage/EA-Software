/**
 * For all those who love to press buttons... I present key controls.
 * Handles up, down, right, left, numpadleft, numpadright.
 * @file keyboardevents.js
 */

(function () {
    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubKeyeventsCtrl($scope, AppCommFactory, UtilFactory) {
        var self = this;

        /* Roster Events */
        this.registerEventHandlers = function (toFocus) {
            var attachScope = angular.element(toFocus).scope(),
                detachScope = angular.element($scope.focused).scope();

            if (typeof attachScope !== 'undefined') {
                if (typeof $scope.focused === 'undefined') {
                    attachScope.attachHandlers();
                } else if(!$(toFocus).is($scope.focused)) {
                    
                    if(typeof detachScope !== 'undefined') {
                        angular.element($scope.focused).scope().detachHandlers();
                    }

                    attachScope.attachHandlers();
                }
                $scope.focused = $(toFocus);
            }
        };

        this.isFocused = function (elem) {
            return $(elem).is(':focus');
        };

        this.handleKeyboardEvent = function (event) {
            var keyCode = event.which || event.keyCode,
                keys = {
                    left:               37,
                    up:                 38,
                    right:              39,
                    down:               40,
                    numpadPlus:         107,
                    numpadSubtract:     109,
                    enter:              13,
                    escape:             27
                };

            switch(keyCode) {
                case keys.down:
                    AppCommFactory.events.fire('social:keyupDown');
                break;    
                case keys.up: 
                    AppCommFactory.events.fire('social:keyupUp');
                break;
                case keys.numpadPlus:
                case keys.right:
                    AppCommFactory.events.fire('social:keyupRight');
                break;
                case keys.left:
                case keys.numpadSubtract:
                    AppCommFactory.events.fire('social:keyupLeft');
                break;
                case keys.enter:
                    AppCommFactory.events.fire('social:keyupEnter');
                break;
                case keys.escape:
                    AppCommFactory.events.fire('social:keyupEscape');
                break;
                }

            AppCommFactory.events.fire('social:setHighlight');

            $scope.$digest();
        };

        this.handleLeftClickRosterEvent = function (event) {
            AppCommFactory.events.fire('social:rosterLeftClick', { 'eventTarget': event.target });

            AppCommFactory.events.fire('social:updateContextmenuState');
            AppCommFactory.events.fire('social:setHighlight');

            $scope.$digest();
        };

        this.handleRightClickEvent = function () {
            AppCommFactory.events.fire('social:rosterRightClick');

            AppCommFactory.events.fire('social:updateContextmenuState');
            AppCommFactory.events.fire('social:setHighlight');

            $scope.$digest();
        };
			
        this.clearSocialRoster = function () {
            AppCommFactory.events.fire('social:resetRosterDefaults');
            AppCommFactory.events.fire('social:updateContextmenuState');
			
            $scope.$digest();
        };

        /* Creates throttle for less digests() on
        * contextmenu item mouseenter events 
        */
        function mouseenterDropThrottle () {
            AppCommFactory.events.fire('social:setHighlight');
            self.registerEventHandlers(document.activeElement);
			
            $scope.$digest();
			} 
		
        this.handleMouseenterEvent = UtilFactory.reverseThrottle(mouseenterDropThrottle, 200);
        
    }

    /**
    * The directive
    */
    function OriginSocialHubKeyevents () {
        
        /**
        * The link 
        */
        function OriginSocialHubKeyeventsLink (scope, attrs, elem, ctrl) {
            scope = scope;
            attrs = attrs;
            elem = elem;

            $(document).on('click', function (e) {
                // If not a child of a [keytarget], reset roster to default
                if($(e.target).closest('[keytarget]').length < 1) {
                    ctrl.clearSocialRoster();
                }
            });



           
            

        }

        return {
            restrict: 'A',
            priority: -1, 
            controller: 'OriginSocialHubKeyeventsCtrl',
            link: OriginSocialHubKeyeventsLink
        };
    }
    
    angular.module('origin-components')
        .controller('OriginSocialHubKeyeventsCtrl', OriginSocialHubKeyeventsCtrl)
        .directive('originSocialHubKeyevents', OriginSocialHubKeyevents);
}());