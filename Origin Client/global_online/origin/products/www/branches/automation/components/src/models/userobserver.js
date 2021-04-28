/**
 * @file models/userobserver.js
 */

(function() {
    'use strict';

    function UserObserverFactory(ObservableFactory, ObserverFactory, SubscriptionFactory, AuthFactory, GamesEntitlementFactory) {
        var template = {
                isSubscriber: false,
                subscriptions: {},
                basegames: {},
                addons: {}
            };

        function getSubscriberModel() {
            template.subscriptions = SubscriptionFactory.getUserSubscriptionInfo();
            template.isSubscriber = SubscriptionFactory.userHasSubscription();

            return template;
        }

        function getBasegameEntitlements() {
            template.basegames = GamesEntitlementFactory.getBaseGameEntitlements();

            return template;
        }

        function getExtraContentEntitlements() {
            template.addons = GamesEntitlementFactory.getExtraContentEntitlements();

            return template;
        }


        function getObserver(scope, bindTo) {
            var observable = ObservableFactory.observe();

            /**
             * Call commit in the observable's context to prevent inheriting the event scope frame
             */
            var callbackCommit = function() {
                observable.commit.apply(observable);
            };

            AuthFactory.events.on('myauth:ready', callbackCommit);
            AuthFactory.events.on('myauth:change', callbackCommit);
            GamesEntitlementFactory.events.on('GamesEntitlementFactory:getEntitlementSuccess', callbackCommit);
            GamesEntitlementFactory.events.on('GamesEntitlementFactory:isNewUpdate', callbackCommit);
            SubscriptionFactory.events.on('SubscriptionFactory:subscriptionUpdate', callbackCommit);

            var destroyEventListeners = function() {
                AuthFactory.events.off('myauth:ready', callbackCommit);
                AuthFactory.events.off('myauth:change', callbackCommit);
                GamesEntitlementFactory.events.off('GamesEntitlementFactory:getEntitlementSuccess', callbackCommit);
                GamesEntitlementFactory.events.off('GamesEntitlementFactory:isNewUpdate', callbackCommit);
                SubscriptionFactory.events.off('SubscriptionFactory:subscriptionUpdate', callbackCommit);
            };

            scope.$on('$destroy', destroyEventListeners);

            return ObserverFactory
                .create(observable)
                .addFilter(getSubscriberModel)
                .addFilter(getBasegameEntitlements)
                .addFilter(getExtraContentEntitlements)
                .attachToScope(scope, bindTo);
        }



        return {
            getObserver: getObserver
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.UserObserverFactory

     * @description
     *
     * User Oberserver Factory
     */
    angular.module('origin-components')
        .factory('UserObserverFactory', UserObserverFactory);
}());
