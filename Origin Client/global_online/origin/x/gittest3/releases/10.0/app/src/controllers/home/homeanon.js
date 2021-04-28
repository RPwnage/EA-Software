(function() {
    'use strict';

    function HomeAnonCtrl($scope, $state, AppCommFactory) {

        function logIn() {
            $state.go('app.home_home');
        }

        // Initial page load
        if (Origin.auth.isLoggedIn()) {
            logIn();
        }

        // Event Listeners
        AppCommFactory.events.once('auth:jssdkLogin', logIn);

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
     * @name originApp.controllers:HomeAnonCtrl
     * @requires $state
     * @requires AppCommFactory
     * @requires Origin
     * @description
     *
     * The controller for the anonymous Origin Home page
     *
     */
    angular.module('originApp')
        .controller('HomeAnonCtrl', HomeAnonCtrl);
}());