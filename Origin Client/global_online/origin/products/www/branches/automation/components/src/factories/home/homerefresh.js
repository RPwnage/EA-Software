/**
 * @file home/feedrefresh.js
 */
(function() {
    'use strict';

    function HomeRefreshFactory($timeout, $state, FeedModelsFactory, GamesDataFactory, CQTargetingStateChangeFactory) {
        var refreshInterval = 60 * 1000, //initial value is 1 minute, should get overwritten by value from CMS
            refreshSuggestedFlag = false,
            refreshTimeOutPromise,
            myEvents = new Origin.utils.Communicator();


        /**
         * refresh the home page by reloading the current state
         */
        function refresh(preserveSourceFlag) {
            updateRefreshTimer();
            FeedModelsFactory.reset();

            //we need to trigger a dummy param change so ui-router will refresh the same state
            //we cannot use $state.reload as that reloads all the states (social etc)
            $state.go($state.current, {
                source: preserveSourceFlag ? $state.params.source : false
            }, {
                reload: $state.current.name
            });
        }

        function stopRefreshTimer() {
            if (refreshTimeOutPromise) {
                $timeout.cancel(refreshTimeOutPromise);
            }
        }

        /**
         * Show 'Refresh this Page' button
         */
        function suggestRefresh() {
            //let everyone know who cares that we should refresh the feeds
            myEvents.fire('suggestRefresh');

            //set a flag so directives can check this value when they are initialized
            refreshSuggestedFlag = true;
        }

        /**
         * updates the refresh timer once we get the interval from CMS
         * @param  {number} newInterval the time in milliseconds of when we should notify the user they should refresh
         */
        function updateRefreshTimer(newInterval) {

            //stop the running timer if there is one
            stopRefreshTimer();

            //reset the refresh suggested flag
            refreshSuggestedFlag = false;

            //update the interval value
            if (newInterval) {
                refreshInterval = newInterval;
            }

            //kick off a timer
            refreshTimeOutPromise = $timeout(suggestRefresh, refreshInterval, false);
        }

        /**
         * returns the refresh suggested flag
         * @return {boolean} true if there is a refresh suggested
         */
        function refreshSuggested() {
            return refreshSuggestedFlag;
        }

        function handleEntitlementUpdate() {
            //Show the 'Refresh this Page' button if we're already on Home,
            //If not already on Home, the next entry to Home will trigger a full refresh
            stopRefreshTimer();
            suggestRefresh();
        }


        GamesDataFactory.waitForInitialRefreshCompleted()
            .then(function(){
                GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', handleEntitlementUpdate);
            });

        FeedModelsFactory.events.on('FeedModelsFactory:modelDirty', suggestRefresh);
        CQTargetingStateChangeFactory.events.on('CQTargetingStateChangeFactory:stateChanged', suggestRefresh);

        return {
            /**
             * events for the factory
             */
            events: myEvents,
            /**
             * updates the refresh timer once we get the interval from CMS
             * @param  {number} newInterval the time in milliseconds of when we should notify the user they should refresh
             */
            updateRefreshTimer: updateRefreshTimer,
            /**
             * returns the refresh suggested flag
             * @return {boolean} true if there is a refresh suggested
             */
            refreshSuggested: refreshSuggested,
            /**
             * refresh the home page by reloading the current state
             */
            refresh: refresh
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */

    function HomeRefreshFactorySingleton($timeout, $state, FeedModelsFactory, GamesDataFactory, CQTargetingStateChangeFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('HomeRefreshFactory', HomeRefreshFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.HomeRefreshFactory
     
     * @description
     *
     * HomeRefreshFactory
     */
    angular.module('origin-components')
        .factory('HomeRefreshFactory', HomeRefreshFactorySingleton);
}());