/**
 * @file observable.js
 */
(function() {
    'use strict';

    var CHANGE_EVENT = 'local:change';

    function ObservableFactory() {
        /**
         * @class Observable
         * Observable data container
         * @param {data} data the inititalizing data set
         * @param {Function} EventHandler event class constructor to use
         * @return {Observable}
         */
        function Observable(data, EventHandler) {
            this.events = new EventHandler();
            this.data = data;
        }

        /**
         * Adds a function to call when the Observable is updated
         * @param {Function} callback function to attach to the change event
         * @return {Object} event handler object
         */
        Observable.prototype.addObserver = function (callback) {
            return this.events.on(CHANGE_EVENT, callback);
        };

        /**
         * Removes the callback from the list of observers
         * @param {Function} callback function to detach from the change event
         * @return {void}
         */
        Observable.prototype.removeObserver = function (callback) {
            return this.events.off(CHANGE_EVENT, callback);
        };

        /**
         * Moves observable into the clean state which executes observing callbacks
         * @return {void}
         */
        Observable.prototype.commit = function () {
            this.events.fire(CHANGE_EVENT, this.data);
        };

        /**
         * Decorates an object with an observable instance
         * @param {Object} initialData initial state of the observable model object
         * @param {Function} EventHandler the optional event class constructor to use (defaults to Origin.utils.Communicator)
         * @return {Observable}
         */
        function observe(initialData, EventHandler) {
            var modelObject = initialData || {};
            EventHandler = EventHandler || Origin.utils.Communicator;

            return new Observable(modelObject, EventHandler);
        }

        /**
         * Extracts data from the observable
         * @param {Object} model Observable model
         * @return {mixed} the data stored in the given observable
         */
        function extract(model) {
            if (model instanceof Observable) {
                return model.data;
            }

            return model;
        }

        return {
            observe: observe,
            extract: extract
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ObservableFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ObservableFactory', ObservableFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ObservableFactory
     * @description
     *
     * Helpers for manipulating objects
     */
    angular.module('origin-components')
        .factory('ObservableFactory', ObservableFactorySingleton);
}());