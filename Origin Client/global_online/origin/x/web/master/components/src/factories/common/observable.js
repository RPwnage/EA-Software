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
         */
        function Observable(data) {
            this.events = new Origin.utils.Communicator();
            this.updateInProgress = false;
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
         * Moves observable into the mutation (loading) state
         * @return {Object}
         */
        Observable.prototype.startChanging = function () {
            this.updateInProgress = true;
        };

        /**
         * Moves observable into the clean state which executes observing callbacks
         * @return {void}
         */
        Observable.prototype.commit = function () {
            this.updateInProgress = false;
            this.events.fire(CHANGE_EVENT, this.data);
        };

        /**
         * Returns true if the object was put into the mutation (loading) state.
         * @return {boolean}
         */
        Observable.prototype.isLoading = function () {
            return this.updateInProgress;
        };

        /**
         * Decorates an object with an observable instance
         * @param {Object} initialData initial state of the observable model object
         * @return {Observable}
         */
        function observe(initialData) {
            var modelObject = initialData || {};

            return new Observable(modelObject);
        }

        /**
         * Extracts data from the observable
         * @param {Object} model Observable model
         * @return {Observable}
         */
        function extract(model) {
            if (model instanceof Observable) {
                return model.data;
            } else {
                return model;
            }
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