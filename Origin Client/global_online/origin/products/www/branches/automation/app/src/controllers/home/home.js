(function() {
    'use strict';

    function HomeCtrl($state, AuthFactory) {

        if (!AuthFactory.isAppLoggedIn()) {
            $state.go('app.store.wrapper.home');
        } else if (Origin.user.underAge()) {
            $state.go('app.game_gamelibrary');
        } else {
            //use the $stateParams from home as the initial params for home logged in
            $state.go('app.home_loggedin', $state.params);
        }
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:HomeCtrl
     * @requires $state
     * @requires $scope
     * @requires AuthFactory
     * @requires Origin
     * @description
     *
     * The controller for Home
     */
    angular.module('originApp')
        .controller('HomeCtrl', HomeCtrl);
}());
