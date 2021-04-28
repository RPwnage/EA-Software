/**
 * @file controllers/main.js
 */
(function() {
    'use strict';

    function MainCtrl($scope) {

        $scope.isLoaded = false;
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:MainCtrl
     * @requires $scope
     * @requires AppCommFactory
     * @description
     *
     * listens for auth changes and sets the isLoaded flag accordingly
     *
     */
    angular.module('originApp')
        .controller('MainCtrl', MainCtrl);

}());