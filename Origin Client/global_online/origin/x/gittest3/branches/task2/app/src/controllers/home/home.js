(function() {
    'use strict';

    function HomeCtrl($scope, $state, AuthFactory, AppCommFactory) {
        function logOut() {
            $state.go('app.home_welcome');
        }

        function ensureOnline(isOnline) {
            if (!isOnline) {
                $state.go('app.home_offline');
            }
        }

        // Initial page load
        if (!Origin.auth.isLoggedIn()) {
            logOut();
        }
        if (!AuthFactory.isClientOnline()) {
            ensureOnline(false);
        }

        // Event Listeners
        AppCommFactory.events.on('auth:jssdkLogout', logOut);
        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, ensureOnline);

        // Remove event listeners on state change
        $scope.$on('$stateChangeStart', function() {
            AppCommFactory.events.off('auth:jssdkLogout', logOut);
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, ensureOnline);
        });

        $scope.$on('$destroy', function() {
            AppCommFactory.events.off('auth:jssdkLogout', logOut);
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, ensureOnline);
        });
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:RouteCtrl
     * @requires $state
     * @requires AppCommFactory
     * @requires Origin
     * @description
     *
     * listens for auth changes and sets the isLoaded flag accordingly
     *
     */
    angular.module('originApp')
         .controller('HomeCtrl', HomeCtrl);
 }());