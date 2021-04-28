/**
 * Factory to be used to communicate with the application layer
 * @file appcomfactory.js
 */
(function() {
    'use strict';

    function AppCommFactory() {
        var myEvents = new Origin.utils.Communicator();
        return {
            events: myEvents
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function AppCommFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('AppCommFactory', AppCommFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.AppCommFactory
     * @description
     *
     * AppCommFactory
     */
    angular.module('origin-components')
        .factory('AppCommFactory', AppCommFactorySingleton);
}());