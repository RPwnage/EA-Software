(function() {
    'use strict';

    function GameLibraryOgdCtrl($scope, $rootScope, $stateParams) {
        $scope.getStateParams = function() {
            return $stateParams;
        };
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:GameLibraryOgdCtrl
     * @requires $scope
     * @description
     *
     * The controller for the game library
     *
     */
    angular.module('originApp')
        .controller('GameLibraryOgdCtrl', GameLibraryOgdCtrl);
}());