/**
 * Communication Factory  is a generic publish subscribe factory that allows communication between javascript objects
 * without coupling them tightly together.
 *
 *  Two instances of this communication object are created. AppCommFactory for communicating between the components and
 *  the application and CommunicationFactory used to communicate between components.
 *
 * @file appcomfactory.js
 */
(function() {
    'use strict';

    function CommunicationFactory() {
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
        return SingletonRegistryFactory.get('AppCommFactory', CommunicationFactory, this, arguments);
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ComponentCommunicationFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('CommunicationFactory', CommunicationFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.AppCommFactory
     * @description
     *
     * AppCommFactory
     */
    angular.module('origin-components')
        .factory('AppCommFactory', AppCommFactorySingleton)
        .factory('CommunicationFactory', ComponentCommunicationFactorySingleton);
}());