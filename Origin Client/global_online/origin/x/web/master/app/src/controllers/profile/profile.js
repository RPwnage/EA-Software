(function() {
    'use strict';

    function ProfileCtrl($scope, $stateParams, AuthFactory, RosterDataFactory) {

        $scope.isOnline = AuthFactory.isClientOnline();
        $scope.isAuth = AuthFactory.isAppLoggedIn();
        $scope.isBlocked = false;
        $scope.isPrivate = false;
        $scope.isLoading = !$scope.isAuth;

        if (typeof($stateParams.id) !== 'undefined') {
            $scope.nucleusId = $stateParams.id;
            RosterDataFactory.isBlocked($scope.nucleusId).then(function(result) {
                $scope.isBlocked = result;
            });

        } else {
            // This is my profile
            $scope.nucleusId = '';
            $scope.isBlocked = false;
        }
        
        function onBlockListChanged() {
            RosterDataFactory.isBlocked($scope.nucleusId).then(function(result) {
                $scope.isBlocked = result;
                $scope.$digest();                
            });
        }

        function onClientAuthChanged(changeObj) {
            $scope.isLoading = false;
            $scope.isOnline = changeObj && changeObj.isOnline;
            $scope.isAuth = changeObj && changeObj.isLoggedIn;
            $scope.$digest();
        }

        function onClientOnlineStateChanged(onlineObj) {
            $scope.isOnline = onlineObj && onlineObj.isOnline;
            $scope.isAuth = onlineObj && onlineObj.isLoggedIn;
            $scope.$digest();
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            AuthFactory.events.off('myauth:change', onClientAuthChanged);
            AuthFactory.events.off('myauth:ready', onClientAuthChanged);
            RosterDataFactory.events.off('xmppBlockListChanged', onBlockListChanged);
            RosterDataFactory.events.off('privacyChanged', onPrivacyChanged);
        }
        $scope.$on('$destroy', onDestroy);
        
        function onPrivacyChanged(isPrivate) {
            $scope.isPrivate = isPrivate;
            $scope.$digest();
        }

        // listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onClientAuthChanged);
        AuthFactory.events.on('myauth:ready', onClientAuthChanged);

        // listen for block list changes
        RosterDataFactory.events.on('xmppBlockListChanged', onBlockListChanged);
        
        // listen for privacy changes
        RosterDataFactory.events.on('privacyChanged', onPrivacyChanged);

    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:ProfileCtrl
     * @requires $state
     * @requires AppCommFactory
     * @requires Origin
     * @description
     *
     *
     */
    angular.module('originApp')
        .controller('ProfileCtrl', ProfileCtrl);
}());
