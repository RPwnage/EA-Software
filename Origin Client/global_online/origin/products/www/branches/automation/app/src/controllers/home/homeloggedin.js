(function() {
    'use strict';

    function HomeloggedinCtrl($state, $scope, AuthFactory) {
        function onAuthChanged(loginObj) {
            if (loginObj.isLoggedIn && Origin.user.underAge()) {
                $state.go('app.game_gamelibrary');
            }
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:change', onAuthChanged);
        }

        $scope.$on('$destroy', onDestroy);

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onAuthChanged);
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:HomeloggedinCtrl
     * @requires $state
     * @requires $scope
     * @requires AuthFactory
     * @requires Origin
     * @description
     *
     * The controller for Home
     */
    angular.module('originApp')
        .controller('HomeloggedinCtrl', HomeloggedinCtrl);
}());
