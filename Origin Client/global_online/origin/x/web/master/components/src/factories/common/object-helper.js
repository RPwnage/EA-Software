/**
 * @file object-helper.js
 */
(function() {

    'use strict';

    function ObjectHelperFactory($filter) {
        /**
         * Converts native object/primitives into native array by dropping property names and keeping property values.
         * Helps when we need to apply Array's map/filter functions to an object
         *
         * @param {Mixed} object - Javascript object or primitive to be converted
         * @return {Array}
         */
        function toArray(object) {
            if (!object) {
                return object;
            }

            if (_.isNumber(object) || _.isString(object) || _.isBoolean(object)) {
                return [object];
            }

            return _.toArray(object);
        }

        /**
         * Applies a function to each property of the given object.
         * Returns the same object with no modification to allow for method chaining.
         *
         * @param {function} func - function to apply to each property
         * @param {Object} object - Javascript object to be iterated over
         * @return {Object}
         */
        function forEach(func, object) {
            return _.forEach(object, func);
        }

        /**
         * Partially apply forEach with the given function
         *
         * @param {function} func - function to apply to each property
         * @return {Function} partially applied forEach
         */
        function forEachWith(func) {
            return _.partial(forEach, func, _);
        }

        /**
         * Maps a function over the object's properties preserving the keys.
         *
         * @param {function} func - function to apply to each property
         * @param {Array|Object} object - Javascript source object mapped
         * @return {Array|Object}
         */
        function map(func, object) {
            return _.mapValues(object, func);
        }

        /**
         * Generates a function that maps the given function over incoming objects
         *
         * @param {Function} func - function to apply to each property
         * @param {Object} object - Javascript source object mapped
         * @return {Function}
         */
        function mapWith(func) {
            return _.partial(map, func, _);
        }

        /**
         * Generates getter function that given an object yields its property value.
         * Used for creating filter/promise piplines
         *
         * @param {string|array} propertyName - property name
         * @return {function}
         */
        function getProperty(propertyName) {
            return _.partial(_.get, _, propertyName);
        }

        /**
         * Transforms the object according to the given transformation map
         *
         * @param {Object} propertyMap - transformed object's property names associated
         *                               with corresponding locations in the original object
         * @param {Object} source - Javascript object to be transformed
         * @return {Object}
         */
        function transform(propertyMap, source) {
            var target = {};

            forEach(function (sourceProperty, targetProperty) {
                target[targetProperty] = getProperty(sourceProperty)(source);
            }, propertyMap);

            return target;
        }

        /**
         * Generates a function that transforms incoming objects according to the given transformation map
         *
         * @param {Object} propertyMap - transformed object's property names associated
         *                               with corresponding locations in the original object
         * @return {Function}
         */
        function transformWith(propertyMap) {
            return _.partial(transform, propertyMap, _);
        }

        /**
         * Merges properties of the source into the target
         *
         * @param {Object} target - merge target
         * @param {Object} source - data source
         * @return {Object}
         */
        function merge(target, source) {
            return _.merge(target, source);
        }

        /**
         * Partially applied merge
         *
         * @param {Object} target - merge target
         * @return {Function}
         */
        function mergeWith(target) {
            return _.partial(merge, target, _);
        }

        /**
         * Filters object's properties preserving the keys. Removes any property that doesn't pass the filtering function.
         *
         * @param {function} filterFunction - function to apply to each property
         * @param {Object} object - Javascript object to be converted
         * @return {Array}
         */
        function filter(filterFunction, object) {
            var result = Array.isArray(object) ? [] : {};

            forEach(function (value, name) {
                if (filterFunction(value, name)) {
                    result[name] = value;
                }
            }, object);

            return result;
        }

        /**
         * Generates a function that filters incoming objects by certain criteria
         *
         * @param {function} filterFunction - function to apply to each property
         * @return {Function}
         */
        function filterBy(filterFunction) {
            return _.partial(filter, filterFunction, _);
        }

        /**
         * Finds and returns the first property that has the value satisfying the given filter function.
         *
         * @param {function} filterFunction - function to apply to each property
         * @param {Object} object - Javascript object to be converted
         * @return {Array}
         */
        function find(filterFunction, object) {
            return _.find(object, filterFunction);
        }

        /**
         * a partially applied lodash "where" collection filter
         *
         * @param {Object} filterConditions - object with filtering conditions
         * @param {Object} object - Javascript object to be converted
         * @return {Object[]}
         */
        function where(filterConditions) {
            return _.partial(_.where, _, filterConditions);
        }

        /**
         * Folds the object to a single value by incrementally applying the given reducer function
         *
         * @param {function} reducer - function that performs incremental reduction
         * @param {mixed} initialValue - initial value
         * @param {Object} object - Javascript object to be reduced
         * @return {mixed}
         */
        function reduce(reducer, initialValue, object) {
            return _.reduce(object, reducer, initialValue);
        }

        /**
         * Generates a function that reduces incoming object according to the given parameters
         *
         * @param {function} reducer - function that performs incremental reduction
         * @param {mixed} initialValue - initial value
         * @return {Function}
         */
        function reduceWith(reducer, initialValue) {
            return _.partial(reduce, reducer, initialValue, _);
        }

        /**
         * Uses Angular's 'orderBy' filter to sort the incoming list by a given field
         *
         * @param {string} field - field name to sort by
         * @param {Array|Object} list - the list to sort
         * @return {Function}
         */
        function orderByField(field) {
            return function (list) {
                return $filter('orderBy')(toArray(list), field, true);
            };
        }

        /**
         * Returns the first item from a collection
         *
         * @param {Array} list - Javascript array or iterable
         * @return {mixed}
         */
        function takeHead(list) {
            return _.first(list);
        }

        /**
         * Creates value substitution filter
         *
         * @param {mixed} source - Value to watch for
         * @param {mixed} target - Value to return instead
         * @return {Function}
         */
        function substitute(source, target) {
            return function (value) {
                if (value === source) {
                    return target;
                } else {
                    return value;
                }
            };
        }

        /**
         * Generates a function thet returns a partial copy of the incoming object containing
         * only the keys specified. If the key does not exist, the property is ignored.
         *
         * @param {Array} keys - names of the properties to pick from the object
         * @return {Object}
         */
        function pick(keys) {
            return _.partial(_.pick, _, keys);
        }

        /**
         * Copies an object or collection
         *
         * @param {Object|Array} object - Javascript object or collection to copy
         * @return {Object}
         */
        function copy(object) {
            return _.cloneDeep(object);
        }

        /**
         * Calculates the length of a Javascript object
         *
         * @param {Object} object - Javascript object
         * @return {integer}
         */
        function length(object) {
            return Object.keys(object).length;
        }

        /**
         * Creates a pipeline filter that substitutes empty values with a given default
         *
         * @param {mixed} defaultValue value to use as the default
         * @return {Function}
         */
        function defaultTo(defaultValue) {
            return function (data) {
                if (angular.isUndefined(data) || data === null) {
                    return defaultValue;
                } else {
                    return data;
                }
            };
        }

        return {
            getProperty: getProperty,
            toArray: toArray,
            map: map,
            mapWith: mapWith,
            copy: copy,
            pick: pick,
            find: find,
            filter: filter,
            filterBy: filterBy,
            where: where,
            length: length,
            forEach: forEach,
            forEachWith: forEachWith,
            transform: transform,
            transformWith: transformWith,
            merge: merge,
            mergeWith: mergeWith,
            takeHead: takeHead,
            reduce: reduce,
            reduceWith: reduceWith,
            orderByField: orderByField,
            substitute: substitute,
            defaultTo: defaultTo
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ObjectHelperFactorySingleton($filter, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ObjectHelperFactory', ObjectHelperFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ObjectHelperFactory
     * @description
     *
     * Some common lodash object and array manipulation patterns
     * The By/With Applies some auto curried, function first syntax sugar for use in Promise contexts
     */
    angular.module('origin-components')
        .factory('ObjectHelperFactory', ObjectHelperFactorySingleton);
}());