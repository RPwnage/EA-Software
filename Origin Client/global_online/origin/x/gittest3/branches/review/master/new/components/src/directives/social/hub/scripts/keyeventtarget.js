/**
 * @file keyeventtarget.js
 */

(function () {
    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubKeyeventtargetCtrl($scope) {
        $scope.isActive = false;
        $scope.isHeaderCollapsed = false;
        
        this.setActive = function () {
            $scope.isActive = true;
        };

        this.setInactive = function () {
            $scope.isActive = false;
        };

        this.reEvalScope = function () {
            $scope.$digest();
        };

    }

    /**
    * The directive
    */
    function OriginSocialHubKeyeventtarget () {
        
        /**
        * The link 
        */
        function OriginSocialHubKeyeventtargetLink (scope, elem, attrs, ctrl) {
            scope.targetType = attrs.originSocialHubKeyeventtarget; // from DOM
            scope.isItem     = scope.targetType === 'item';
            scope.isHeader   = scope.targetType === 'header';
			
			var keyEventsAPI = ctrl[0]; // keyEvents.js controller
			var thisController = ctrl[1]; // this controller
            // This will be roster ctrl OR chatrooms ctrl
			var rosterController = ( !!ctrl[2] ? ctrl[2] : ctrl[3]);	

            $(elem).on('click', function () {
                keyEventsAPI.setFocus(elem);
                keyEventsAPI.handleClick('leftClick');
            });

            $(elem).on('keydown', function (e) {                            
                e.preventDefault();
                return false;
            });

            $(elem).on('keyup', function (e) {
                var keyboardMap = keyEventsAPI.getKeyboardMap(e);
                switch(keyboardMap.keyCode) {
                    case keyboardMap.keys.down:
                        keyEventsAPI.handleDownKey(elem);
                    break;    
                    case keyboardMap.keys.up: 
                        keyEventsAPI.handleUpKey(elem);
                    break;
                    case keyboardMap.keys.numpadPlus:
                    case keyboardMap.keys.right:
                        keyEventsAPI.handleRightKey(elem);
                    break;
                    case keyboardMap.keys.left:
                    case keyboardMap.keys.numpadSubtract:
                        keyEventsAPI.handleLeftKey(elem);
                    break;
                    case keyboardMap.keys.enter:
                        keyEventsAPI.handleEnterKey(elem);
                    break;
                }
            });

            scope.$on('keyupDOWN', function() {
                
                if(scope.isActive) {
                    var next = keyEventsAPI.getNext(elem);
                    thisController.setInactive();
                    keyEventsAPI.setFocus(next);
                }
            });

            scope.$on('keyupUP', function() {
                
                if(scope.isActive) {
                    var prev = keyEventsAPI.getPrevious(elem);

                    thisController.setInactive();
                    keyEventsAPI.setFocus(prev);
                }
            });

            scope.$on('keyupRIGHT', function() {
            
               if(scope.isActive && scope.isHeader) {
                    // If is collapsed, expand
                    if(!$(elem).hasClass('header-visible')) {
                        rosterController.toggleSectionVisible();
                    }
                }
            });

            scope.$on('keyupLEFT', function() {
                    
                if(scope.isActive && scope.isHeader) {
                    // If is not collapsed, collapse
                    if($(elem).hasClass('header-visible')) {
                        rosterController.toggleSectionVisible();
                    }
                }
            });

            scope.$on('keyupENTER', function() {

                if(scope.isActive) {
                    scope.$broadcast('openSocialConversationOnEnter', { 'thisItem' : elem });
                }
            });

            scope.$on('newSocialSelection', function() { 
                thisController.setInactive();

                if($(elem).is(':focus')) {
                    thisController.setActive();
                }
                thisController.reEvalScope();
            });

            // To select item like a left click does
            // Inner items handle respectively
            $(elem).bind('contextmenu', function (e) {
                e.preventDefault();
                keyEventsAPI.setFocus(elem);
                keyEventsAPI.handleClick('rightClick');
            });

        }

        return {
            restrict: 'A',
            priority: -1,
            require: ['^originSocialHubKeyevents', 'originSocialHubKeyeventtarget', '^?originSocialHubRosterSection', '^?originSocialHubChatroomsSection'],
            controller: 'OriginSocialHubKeyeventtargetCtrl',
            link: OriginSocialHubKeyeventtargetLink
        };
    }
    
    angular.module('origin-components')
        .controller('OriginSocialHubKeyeventtargetCtrl', OriginSocialHubKeyeventtargetCtrl)
        .directive('originSocialHubKeyeventtarget', OriginSocialHubKeyeventtarget);
}());