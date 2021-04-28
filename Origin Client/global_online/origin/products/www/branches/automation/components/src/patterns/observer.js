/**
 * @file observer.js
 */
(function() {
    'use strict';

    function ObserverFactory(ObjectHelperFactory, FunctionHelperFactory, ScopeHelper) {
        var where = ObjectHelperFactory.where,
            toArray = ObjectHelperFactory.toArray,
            getProperty = ObjectHelperFactory.getProperty,
            pick = ObjectHelperFactory.pick,
            pipe = FunctionHelperFactory.pipe,
            substitute = ObjectHelperFactory.substitute;

        /**
         * Creates setter function to bind new observable data to an AngularJS scope object
         * It will unwrap promises if passed to the setter using a string name binding.
         *
         * @param {Object} scope - AngularJS scope object
         * @param {string|Function} bindTo - Scope variable name or a setter function
         * @return {Function}
         */
        function createSetter(scope, bindTo) {
            if (_.isString(bindTo) || _.isNumber(bindTo)) {
                /**
                 * Return a pipeline filter for the callback
                 * @param  {mixed} newData the incoming data from the filters
                 */
                return function (newData) {
                    if(_.isObject(newData) && _.isFunction(newData.then)) {
                        newData.then(function(data) {
                            scope[bindTo] = data;
                        });
                    } else {
                        scope[bindTo] = newData;
                    }
                };
            } else if (_.isFunction(bindTo)) {
                /**
                 * Pass the filtered results directly to a function
                 */
                return bindTo;
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
            /**
             * Defer scope.$digest to an on demand call when data is changed
             */
            return function () {
                ScopeHelper.digestIfDigestNotInProgress(scope);
            };
        }

        /**
         * @class Observer
         * Observer object with filters
         */
        function Observer(observable) {
            this.observable = observable;
            this.filters = [];
            this.callback = null;
        }

        /**
         * Adds new filter function to the observer
         *
         * @param {Function} filterFunction - Filter function
         * @return {Observer}
         */
        Observer.prototype.addFilter = function (filterFunction) {
            if (!_.isFunction(filterFunction)) {
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
            if (_.isEmpty(propertyName)) {
                throw new Error("[Observer] Empty or malformed property name: " + propertyName);
            }

            this.filters.push(getProperty(String(propertyName)));

            return this;
        };

        /**
         * Adds a multi-property picker filter to the observer
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
         * Adds a default value that will be used in case when the observed data is undefined or null
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
         * Detaches the observer from the observable
         *
         * @return {Observer}
         */
        Observer.prototype.detach = function() {
			this.observable.removeObserver(this.callback);
            this.observable.callback = null;

            return this;
        };

        /**
         * The base binding strategy for connecting observables to different call contexts
         *
         * @param {Object} scope AngularJS scope object
         * @param {string|Number|Function} bindTo a setter function or a name of the scope
         *                                 variable to bind the observable data to
         * @param {Function} digestFunc use a custom digest function - see interface in createDigester above
         * @return {Function} the handle to detach the observer
         */
        Observer.prototype.baseBind = function(scope, bindTo, digestFunc) {
            var set = createSetter(scope, bindTo),
                digest = _.isFunction(digestFunc) ? digestFunc(scope) : createDigester(scope),
                callback = pipe([this.filters, set, digest]),
                handler = this.observable.addObserver(callback);

            this.observable.callback = callback;
            callback(this.observable.data);

            if(scope && _.isFunction(scope.$on)) {
                scope.$on('$destroy', handler.detach);
            }

            return this;
        };

        /**
         * Attaches the observer to the given AngularJS scope object and binds
         * the scope's digest cycle to the observable's change event so that the digest is
         * automatically triggered whenever the observable data change is committed. Note that
         * you must set all required filters before calling this function.
         *
         * !!! Functionality deprecation notice: The bindTo parameter accepts both strings and functions leading to confusing
         * !!! callback behavior on scope. Please use onUpdate when using callbacks as the dual functionality of this call will
         * !!! be removed once the current integrations are fixed.
         *
         * @param {Object} scope AngularJS scope object
         * @param {string|Number} bindTo the name of the scope variable to bind the observable data to (eg. 'heading')
         * @param {Function} digestFunc use a custom digest function - see interface in createDigester above
         * @return {Observer}
         */
        Observer.prototype.attachToScope = function(scope, bindTo, digestFunc) {
            return this.baseBind(scope, bindTo, digestFunc);
        };

        /**
         * Calls a callback function when observer updates. Automatically detaches the observer on scope.$destroy.
         * Intended for use in Controller and Link Functions.
         *
         * @param {Object} scope AngularJS scope object
         * @param {Function} callbackFunc a setter function (myFunc)
         * @param {Function} digestFunc use a custom digest function - see interface in createDigester above
         * @return {Observer}
         */
        Observer.prototype.onUpdate = function(scope, callbackFunc, digestFunc) {
            return this.baseBind(scope, callbackFunc, digestFunc);
        };

        /**
         * Creates a new observer for the observable object
         *
         * @param {Observable} observable - Object to observe
         * @return {Observer} the observer instance
         */
        function create(observable) {
            return new Observer(observable);
        }

        return {
            create: create,
            noDigest: _.noop
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ObserverFactory
     * @description
     *
     * Observer: adds dynamic data filtering and scope binding to observable data
     */
    angular.module('origin-components')
        .factory('ObserverFactory', ObserverFactory);
}());