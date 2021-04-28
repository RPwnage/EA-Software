/**
 * Util functions to detach or reattach events
 * @file events-helper.js
 */
(function() {
    'use strict';

    function EventsHelperFactory() {
        return {
            /**
            * Detach all of the events in the array
            * @param {Array} handles - event handles
            * @return {void}
            * @method detachAll
            */
            detachAll: function(handles) {
                handles.map(function(handle) {
                    handle.detach();
                });
            },

            /**
            * Reattach all of the events in the array
            * @param {Array} handles - event handles
            * @return {void}
            * @method attachAll
            */
            attachAll: function(handles) {
                handles.map(function(handle) {
                    handle.attach();
                });
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function EventsHelperFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('EventsHelperFactory', EventsHelperFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.EventsHelperFactory
     * @description
     *
     * EventsHelperFactory
     */
    angular.module('origin-components')
        .factory('EventsHelperFactory', EventsHelperFactorySingleton);
}());