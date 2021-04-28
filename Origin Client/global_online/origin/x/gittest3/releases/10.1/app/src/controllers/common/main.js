/**
 * @file controllers/main.js
 */
(function() {
    'use strict';

    function MainCtrl($scope, AppCommFactory, AuthFactory) {

        $scope.isLoaded = false;

        /**
         * Handle login/logout of the app
         * @return {void}
         * @method
         */
        function handleLogin() {
            $scope.isLoaded = true;
        }

        function onDestroy() {
            AppCommFactory.events.off('auth:ready', handleLogin);
        }

        if (AuthFactory.isAppLoggedIn()) {
            handleLogin();
        }

        AppCommFactory.events.on('auth:ready', handleLogin);
        $scope.$on('$destroy', onDestroy);
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