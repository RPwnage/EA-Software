(function() {
    'use strict';

    function GameLibraryOgdCtrl($scope, $stateParams) {
        $scope.offerId = $stateParams.offerid;
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