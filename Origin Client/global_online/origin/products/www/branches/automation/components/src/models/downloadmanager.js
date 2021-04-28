/**
 * @file models/userobserver.js
 */

(function() {
    'use strict';

    function DownloadManagerFactory(ObservableFactory, ObserverFactory, GamesDataFactory) {

        function getDownloadData(offerId) {
            return GamesDataFactory.getClientGame(offerId);
        }

        function getObserver(offerId, scope, bindTo) {
            var observable = ObservableFactory.observe([offerId]);

            /*
             * Call commit in the observable's context to prevent inheriting the event scope frame
             */
            var callbackCommit = function() {
                observable.commit.apply(observable);
            };

            GamesDataFactory.events.on('games:progressupdate:' + offerId, callbackCommit);
            GamesDataFactory.events.on('games:update:' + offerId, callbackCommit);
            GamesDataFactory.events.on('games:downloadqueueupdate:' + offerId, callbackCommit);
            GamesDataFactory.events.on('games:operationfailedupdate:' + offerId, callbackCommit);

            var destroyEventListeners = function() {
                GamesDataFactory.events.off('games:progressupdate:' + offerId, callbackCommit);
                GamesDataFactory.events.off('games:update:' + offerId, callbackCommit);
                GamesDataFactory.events.off('games:downloadqueueupdate:' + offerId, callbackCommit);
                GamesDataFactory.events.off('games:operationfailedupdate:' + offerId, callbackCommit);
            };

            scope.$on('$destroy', destroyEventListeners);

            return ObserverFactory
                .create(observable)
                .addFilter(getDownloadData)
                .attachToScope(scope, bindTo);
        }

        return {
            getObserver: getObserver
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.DownloadManagerFactory

     * @description
     *
     * Download Manager Factory
     */
    angular.module('origin-components')
        .factory('DownloadManagerFactory', DownloadManagerFactory);
}());
