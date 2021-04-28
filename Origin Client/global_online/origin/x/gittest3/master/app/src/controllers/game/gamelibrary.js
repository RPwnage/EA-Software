(function() {
    'use strict';

    function GameLibraryCtrl($scope, $state, AuthFactory, AppCommFactory) {

        function logOut() {
            $state.go('app.game_unavailable');
        }

        // Initial page load
        if (!Origin.auth.isLoggedIn()) {
            logOut();
        }

        // Event Listeners
        AppCommFactory.events.once('auth:jssdkLogout', logOut);

        // Remove event listeners on state change
        $scope.$on('$stateChangeStart', function() {
            AppCommFactory.events.off('auth:jssdkLogout', logOut);
        });

        $scope.$on('$destroy', function() {
            AppCommFactory.events.off('auth:jssdkLogout', logOut);
        });
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:GameLibraryCtrl
     * @requires $scope
     * @requires AppCommFactory
     * @requires Origin
     * @description
     *
     * The controller for the game library
     *
     */
    angular.module('originApp')
        .controller('GameLibraryCtrl', GameLibraryCtrl);
}());