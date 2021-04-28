(function() {
    'use strict';

    function GameLibraryAnonCtrl($scope, $state, AuthFactory, AppCommFactory) {
        function logIn() {
            $state.go('app.game_gamelibrary');
        }

        // Initial page load
        if (Origin.auth.isLoggedIn()) {
            logIn();
        }

        // Event Listeners
        AppCommFactory.events.on('auth:jssdkLogin', logIn);

        // Remove event listeners on state change
        $scope.$on('$stateChangeStart', function() {
            AppCommFactory.events.off('auth:jssdkLogin', logIn);
        });

        $scope.$on('$destroy', function() {
            AppCommFactory.events.off('auth:jssdkLogin', logIn);
        });
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:GameLibraryAnonCtrl
     * @requires $state
     * @requires AppCommFactory
     * @description
     *
     * Controller for Game Library in anonymous mode
     *
     */
    angular.module('originApp')
         .controller('GameLibraryAnonCtrl', GameLibraryAnonCtrl);
 }());