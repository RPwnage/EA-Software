(function() {
    'use strict';

    function LandingCtrl($state, SubscriptionFactory) {
        // send subscribers to discovery page
        if (SubscriptionFactory.userHasSubscription()) {
            $state.go('app.store.wrapper.origin-access-vault');
        }
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:OriginAccessLandingCtrl
     * @requires $state
     * @requires $scope
     * @requires SubscriptionFactory
     * @requires Origin
     * @description
     *
     * The controller for the Origin Access landing page
     */
    angular.module('originApp')
        .controller('LandingCtrl', LandingCtrl);
}());
