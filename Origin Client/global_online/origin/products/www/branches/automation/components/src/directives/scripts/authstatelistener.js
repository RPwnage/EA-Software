/**
 * @file /ownedgamecount.js
 */

(function() {
    'use strict';

    function OriginAuthstatelistenerCtrl($scope, AuthFactory, EventsHelperFactory) {
        var handles = [];
        if (!$scope.loginEventConfig) {
            $scope.loginEventConfig = {
                obj: AuthFactory,
                eventName: 'myauth:change'
            };

        }

        if (!$scope.onlineStateEventConfig) {
            $scope.onlineStateEventConfig = {
                obj: AuthFactory,
                eventName: 'myauth:clientonlinestatechanged'
            };

        }

        function setLoginState() {
            $scope.isLoggedIn = AuthFactory.isAppLoggedIn();
        }

        function updateLoginState() {
            setLoginState();
            $scope.$digest();
        }

        function setOfflineState() {
            $scope.isOffline = !AuthFactory.isClientOnline();
        }

        function updateOfflineState() {
            setOfflineState();
            $scope.$digest();
        }

        $scope.$on('$destroy', function() {
            EventsHelperFactory.detachAll(handles);
        });

        setLoginState();
        setOfflineState();

        handles = [
            $scope.loginEventConfig.obj.events.on($scope.loginEventConfig.eventName, updateLoginState),
            $scope.onlineStateEventConfig.obj.events.on($scope.onlineStateEventConfig.eventName, updateOfflineState)        
        ];
    }

    function originAuthstatelistener() {
        return {
            restrict: 'A',
            controller: 'OriginAuthstatelistenerCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:authstatelistener
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * a attribute to listen for login/online state changes
     * 
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <ANY ng-if="isLoading && hasGames() && isOnline" origin-authstatelistener></ANY>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAuthstatelistenerCtrl', OriginAuthstatelistenerCtrl)
        .directive('originAuthstatelistener', originAuthstatelistener);
}());