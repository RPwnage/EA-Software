(function() {
    'use strict';

    function HomeOfflineCtrl($scope, $state, AuthFactory) {
        function ensureOnline(isOnline) {
            if (isOnline) {
                $state.go('app.home_home');
            }
        }

        // Initial page load
        if (AuthFactory.isClientOnline()) {
            ensureOnline(true);
        }

        //Event listener
        Origin.events.once(Origin.events.CLIENT_ONLINESTATECHANGED, ensureOnline);

        // Remove event listeners on state change
        $scope.$on('$stateChangeStart', function() {
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, ensureOnline);
        });

        $scope.$on('$destroy', function() {
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, ensureOnline);
        });
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:HomeOfflineCtrl
     * @requires $state
     * @requires AuthFactory
     * @requires AppCommFactory
     * @description
     *
     * The offline-mode homepage controller
     *
     */
    angular.module('originApp')
         .controller('HomeOfflineCtrl', HomeOfflineCtrl);
 }());