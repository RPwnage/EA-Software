(function() {
    'use strict';

    function GameLibraryCtrl($scope, $state, GamesDataFactory, GamesUsageFactory, EventsHelperFactory, GamesEntitlementFactory, RoutingHelperFactory, LogFactory) {
        var eventHandles = [];

        /**
         * clean up the event listeners
         */
        function onDestroy() {
            EventsHelperFactory.detachAll(eventHandles);
        }
        

        /**
         * setup the event listeners
         */
        function listenForUpdates() {
            eventHandles = [
                GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', RoutingHelperFactory.reloadCurrentRoute),
                GamesDataFactory.events.on('games:extraContentEntitlementsUpdate', RoutingHelperFactory.reloadCurrentRoute),
            ];

            $scope.$on('$destroy', onDestroy);
        }

        /**
         * print out an error message
         * @param  {Error} the error object
         */
        function handleError(error) {
            LogFactory.log('[GameLibraryRoute Controller]:', error.message);
        }

        //after intial refresh we listen for any entitlement/catalog and reload route on those events
        GamesDataFactory.waitForInitialRefreshCompleted()
            .then(listenForUpdates)
            .catch(handleError);
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:GameLibraryCtrl
     * @description
     *
     * The controller for the game library
     *
     */
    angular.module('originApp')
        .controller('GameLibraryCtrl', GameLibraryCtrl);
}());