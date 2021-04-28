/**
 * @file directives/touchdetect.js
 */
(function() {
    'use strict';

    function originTouchDetect(UtilFactory) {

        function originTouchDetectLink(scope, element) {
            element.toggleClass('no-touch', !UtilFactory.isTouchEnabled());
        }

        return {
            restrict: 'A',
            link: originTouchDetectLink
        };
    }

    /**
    * @ngdoc directive
    * @name originApp.directives:originTouchDetect
    * @requires UtilFactory
    * @description
    *
    * touch detection
    *
    */
    angular.module('originApp')
        .directive('originTouchDetect', originTouchDetect);

}());