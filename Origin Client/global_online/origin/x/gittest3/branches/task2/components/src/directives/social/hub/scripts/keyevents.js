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
    function OriginSocialHubKeyeventsCtrl($scope) {
        var VISIBLETARGET = '[origin-social-hub-keyeventtarget]:visible';

        this.getKeyboardMap = function (e) {
            return {
                keyCode: e.which || e.keyCode,
                keys: {
                    left:               37,
                    up:                 38,
                    right:              39,
                    down:               40,
                    numpadPlus:         107,
                    numpadSubtract:     109,
                    enter:              13
                }
            };
        };

        this.handleClick = function (clickType) {
            $scope.$broadcast('newSocialSelection', { triggerSource : clickType });
        };

        this.handleDownKey = function () {
            $scope.$broadcast('keyupDOWN', {});
            $scope.$broadcast('newSocialSelection', { triggerSource : 'keyboard' });
        };

        this.handleUpKey = function () {
            $scope.$broadcast('keyupUP', {});
            $scope.$broadcast('newSocialSelection', { triggerSource : 'keyboard' });
        };

        this.handleRightKey = function () {
            $scope.$broadcast('keyupRIGHT', {});
        };

        this.handleLeftKey = function () {
            $scope.$broadcast('keyupLEFT', {});
        };

        this.handleEnterKey = function () {
            $scope.$broadcast('keyupENTER', {});
        };

        /* ----------------------------------
        * API functions
        */

        this.getPrevious = function (focused) {
			var previous = focused;
			var keyboardTargets = $(VISIBLETARGET);
			var index = keyboardTargets.index(focused);
			
			if (index > 0) {
				previous = keyboardTargets.get(index-1);
			} 
			return previous;
			
        };

        this.getNext = function (focused) {
			var next = focused;
			var keyboardTargets = $(VISIBLETARGET);
			var index = keyboardTargets.index(focused);
			
			if (index < (keyboardTargets.length-1)) {
				next = keyboardTargets.get(index+1);
			} 
			return next;
        };
		
        this.setFocus = function (element) {
            return $(element).focus();
        };
        
    }

    /**
    * The directive
    */
    function OriginSocialHubKeyevents () {
        
        /**
        * The link 
        */
        function OriginSocialHubKeyeventsLink () {
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