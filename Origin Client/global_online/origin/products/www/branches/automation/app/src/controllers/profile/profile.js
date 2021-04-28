(function() {
    'use strict';

    function ProfileCtrl($scope, $stateParams, AuthFactory, RosterDataFactory, ObserverFactory, NavigationFactory, NucleusIdObfuscateFactory) {

        $scope.isLoading = true;

        function addNucleusIdToScope(id){
            $scope.nucleusId = id;
        }

        function setNucleusId() {
            if (typeof($stateParams.id) !== 'undefined') {
                return NucleusIdObfuscateFactory.decode($stateParams.id)
                    .then(function(response){addNucleusIdToScope(response.id);})
                    .catch(_.partial(NavigationFactory.goToState, 'app.error_notfound'));
            } else {
                // This is my profile
                $scope.nucleusId = Origin.user.userPid();
                return Promise.resolve(addNucleusIdToScope(Origin.user.userPid()));
            }
        }

        function setUpObservers(user) {
            ObserverFactory.create(user.privacy)
                .defaultTo({isPrivate: false})
                .attachToScope($scope, 'privacy');
            ObserverFactory.create(user.subscription)
                .defaultTo({state: 'NONE', requestSent: false, relationship: 'NON_FRIEND'})
                .attachToScope($scope, 'subscription');
        }

        function getRosterUser() {
            $scope.isLoading = false;
            RosterDataFactory.getRosterUser($scope.nucleusId).then(function(user) {
                if (!!user) {
                    RosterDataFactory.isBlocked(user.nucleusId).then(function(value) {
                        if (value) {
                            user.subscription.data.relationship = 'BLOCKED';
                        }
                        return user;
                    }).then(setUpObservers);
                }
            });
        }

        function init() {
            $scope.isOnline = AuthFactory.isClientOnline();
            $scope.isAuth = AuthFactory.isAppLoggedIn();
            $scope.privacy = {isPrivate: false};
            setNucleusId().then(getRosterUser);
        }

        function onClientAuthChanged(changeObj) {
            $scope.isOnline = changeObj && changeObj.isOnline;
            $scope.isAuth = changeObj && changeObj.isLoggedIn;
            setNucleusId().then(function(){$scope.$digest();});            
        }

        function onClientOnlineStateChanged(onlineObj) {
            $scope.isOnline = onlineObj && onlineObj.isOnline;
            $scope.isAuth = onlineObj && onlineObj.isLoggedIn;
            $scope.isLoading = false;
            $scope.$digest();
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            AuthFactory.events.off('myauth:change', onClientAuthChanged);
            AuthFactory.events.off('myauth:ready', onClientAuthChanged);
        }
        $scope.$on('$destroy', onDestroy);
        
        // listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onClientAuthChanged);
        AuthFactory.events.on('myauth:ready', onClientAuthChanged);

        AuthFactory.waitForAuthReady()
            .then(init);
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
