/**
 * @file observer.js
 */
(function() {
    'use strict';

    function ObserverFactory(ObjectHelperFactory, FunctionHelperFactory, $rootScope) {
        var where = ObjectHelperFactory.where,
            toArray = ObjectHelperFactory.toArray,
            getProperty = ObjectHelperFactory.getProperty,
            pick = ObjectHelperFactory.pick,
            pipe = FunctionHelperFactory.pipe,
            substitute = ObjectHelperFactory.substitute;

        /**
         * Creates setter function to bind new observable data to an AngularJS scope object
         *
         * @param {Object} scope - AngularJS scope object
         * @param {string|Function} bindTo - Scope variable name or a setter function
         * @return {Function}
         */
        function createSetter(scope, bindTo) {
            if (typeof bindTo === 'function') {
                return bindTo;
            } else if (typeof bindTo === 'string') {
                return function (newData) {
                    scope[bindTo] = newData;
                };
            } else {
                throw new Error("[Observer] Unexpected bind variable/setter: " + bindTo);
            }
        }

        /**
         * Creates function that triggers scope digestion process
         *
         * @param {Object} scope - AngularJS scope object
         * @return {Function}
         */
        function createDigester(scope) {
            return function () {
                if (!scope.$$phase && !$rootScope.$$phase) {
                    scope.$digest();
                }
            };
        }

        /**
         * @class Observer
         * Observer object with filters
         */
        function Observer(observable) {
            this.observable = observable;
            this.filters = [];
        }

        /**
         * Adds new filter function to the observer
         *
         * @param {Function} filterFunction - Filter function
         * @return {Observer}
         */
        Observer.prototype.addFilter = function (filterFunction) {
            if (typeof filterFunction !== 'function') {
                throw new Error("[Observer] Unexpected filter function: " + filterFunction);
            }

            this.filters.push(filterFunction);

            return this;
        };

        /**
         * Adds new conditional filter to the observer
         *
         * @param {Object} filterConditions - Filter conditions
         * @return {Observer}
         */
        Observer.prototype.filterBy = function (filterConditions) {
            if (!_.isObject(filterConditions)) {
                throw new Error("[Observer] Unexpected filter conditions: " + filterConditions);
            }

            this.filters.push(where(filterConditions));

            return this;
        };

        /**
         * Adds a single property filter to the observer
         *
         * @param {string|Array} propertyName - Property name/property deep link
         * @return {Observer}
         */
        Observer.prototype.getProperty = function (propertyName) {
            if (typeof propertyName !== 'string') {
                throw new Error("[Observer] Unexpected property name: " + propertyName);
            }

            this.filters.push(getProperty(propertyName));

            return this;
        };

        /**
         * Adds a multy-property picker filter to the observer
         *
         * @param {Array} propertyNames - Names of properties to pick from the observable data
         * @return {Observer}
         */
        Observer.prototype.pick = function (propertyNames) {
            if (!_.isArray(propertyNames)) {
                propertyNames = toArray(propertyNames);
            }

            this.filters.push(pick(propertyNames));

            return this;
        };

        /**
         * Adds a default value that will be used in case when the observed data it undefined or null
         *
         * @param {mixed} value - default value
         * @return {Observer}
         */
        Observer.prototype.defaultTo = function (value) {
            this.filters.push(substitute(null, value));
            this.filters.push(substitute(undefined, value));

            return this;
        };

        /**
         * Attaches the observer to the given AngularJS scope object and binds
         * the scope's digest cycle to the observable's change event so that the digest is
         * automatically triggered whenever the observable data change is committed.
         *
         * @param {Object} scope AngularJS scope object
         * @param {string|Function} bindTo a setter function or a name of the scope
         *                                 variable to bind the observable data to
         * @return {Observer}
         */
        Observer.prototype.attachToScope = function(scope, bindTo) {
            var set = createSetter(scope, bindTo),
                digest = createDigester(scope),
                callback = pipe([this.filters, set, digest]),
                handler = this.observable.addObserver(callback);

            scope.$on('$destroy', handler.detach);
            callback(this.observable.data);

            return this;
        };

        /**
         * Creates a new observer for the observable object
         *
         * @param {Observable} observable - Object to observe
         */
        function create(observable) {
            return new Observer(observable);
        }

        return {
            create: create
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ObserverFactorySingleton(ObjectHelperFactory, FunctionHelperFactory, $rootScope, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ObserverFactory', ObserverFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ObserverFactory
     * @description
     *
     * Observer: adds dynamic filtering and scope binding to observable data
     */
    angular.module('origin-components')
        .factory('ObserverFactory', ObserverFactorySingleton);
}());