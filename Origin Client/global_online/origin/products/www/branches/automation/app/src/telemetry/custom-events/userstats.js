/**
 *  * @file userstats.js
 */
(function () {
    'use strict';

    var AUTH_CHANGE_EVENT = 'myauth:change';
    var GLOBAL_TELEMETRY_KEY = 'global';
    var TELEMETRY_NAMESPACE = 'userStatsTelemetryConfig';


    function TelemetryUserStatisticsCustomEventsFactory(AuthFactory, AchievementFactory, WishlistFactory, RosterDataFactory, TelemetryTrackerUtil, GamesEntitlementFactory, TELEMETRY_CONFIG) {

        var hasSentLoginTelemetry = false; // We only need to send the login data telemetry once

        function extendDefaultEventData(key, labelValue) {
            var defaultTelemetryEventData = _.get(TELEMETRY_CONFIG, [GLOBAL_TELEMETRY_KEY, TELEMETRY_NAMESPACE, key]);
            if (defaultTelemetryEventData) {
                return _.extend({label : labelValue}, defaultTelemetryEventData);
            }
        }

        /**
         * How many achievements has the user completed
         */
        function sendNumberOfAchievements() {
            AchievementFactory.getAchievementPortfolio()
                .then(function (data) {
                    if (_.isObject(data) && _.isFunction(data.grantedAchievements)) {
                        TelemetryTrackerUtil.sendTelemetryEvent(extendDefaultEventData('achievements', data.grantedAchievements()));
                    }
                });
        }

        /**
         * How many friends does the user have
         */
        function sendNumberOfFriends() {
            RosterDataFactory.getNumFriends()
                .then(function (data) {
                    if (_.isNumber(data)) {
                        TelemetryTrackerUtil.sendTelemetryEvent(extendDefaultEventData('friends', data - 1));
                    }
                });
        }

        /**
         * How many games is entitled to the user
         */
        function sendNumberOfGames() {
            var entitlementList = GamesEntitlementFactory.getBaseGameEntitlements();
            if (_.isObject(entitlementList)) {
                var numOfGames = Object.keys(entitlementList).length;
                TelemetryTrackerUtil.sendTelemetryEvent(extendDefaultEventData('games', numOfGames));
            }
        }

        /**
         * How many items is in the user's wishlist
         */
        function sendWishlistLength() {
            WishlistFactory.getWishlist(Origin.user.userPid())
                .getOfferList()
                .then(function (wishlist) {
                    var wishlistLength = _.isArray(wishlist) ? wishlist.length : 0;
                    TelemetryTrackerUtil.sendTelemetryEvent(extendDefaultEventData('wishlist', wishlistLength));
                });
        }

        function onAuthChanged(loginObj) {
            if (!hasSentLoginTelemetry && loginObj.isLoggedIn) {
                hasSentLoginTelemetry = true;

                sendNumberOfFriends();
                sendNumberOfGames();
                sendNumberOfAchievements();
                sendWishlistLength();

            }
        }

        /**
         * Bind all events
         * Register all events here
         * Called by TelemetryTracker when route changes
         */
        function bindEvents() {
            // listen for auth changes
            AuthFactory.events.on(AUTH_CHANGE_EVENT, onAuthChanged);
        }

        /**
         * Unbind all events
         * Un-register all events here
         * Called by TelemetryTracker when route changes
         */
        function unbindEvents() {
            AuthFactory.events.off(AUTH_CHANGE_EVENT, onAuthChanged);
        }


        return {
            bindEvents: bindEvents,
            unbindEvents: unbindEvents
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.TelemetryUserStatisticsCustomEventsFactory
     * @description
     *
     * TelemetryUserStatisticsCustomEventsFactory
     */
    angular.module('originApp')
        .factory('TelemetryUserStatisticsCustomEventsFactory', TelemetryUserStatisticsCustomEventsFactory);
}());
