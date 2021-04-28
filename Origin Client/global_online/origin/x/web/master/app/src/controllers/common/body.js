/**
 * @file controllers/body.js
 */
(function() {
    'use strict';

    function BodyCtrl($scope, UtilFactory) {

        $scope.touchEnabled = UtilFactory.isTouchEnabled();

    }

    /**
    * @ngdoc controller
    * @name originApp.controllers:BodyCtrl
    * @requires $scope
    * @requires UtilFactory
    * @description
    *
    * touch control
    *
    */
    angular.module('originApp')
        .controller('BodyCtrl', BodyCtrl);

}());