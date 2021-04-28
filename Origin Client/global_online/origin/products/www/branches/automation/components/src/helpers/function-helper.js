/**
 * @file function-helper.js
 */
/* global Promise */
(function() {
    'use strict';

    function FunctionHelperFactory() {
        /**
         * Convert function arguments to a real array
         * @param  {mixed} args a collection of function arguments
         * @return {Array} sanitized arguments list
         */
        function sanitizeArguments(args) {
            return Array.prototype.slice.call(args);
        }

        /**
         * Executes the function/array of functions/value with the given array of parameters.
         * Returns execution result for functions and pipes or the value for other input types.
         *
         * @param {mixed} func - function or array of functions to run. If any other data type
         *                       is given as this parameter, execute will just return it intact
         * @param {Array} arguments - arguments to execute the function with
         * @return {mixed}
         */
        function execute(func, args) {
            if (typeof func === 'function') {
                return func.apply(undefined, args);
            } else if (Array.isArray(func)) {
                return pipe(func).apply(undefined, args);
            } else {
                return func;
            }
        }

        /**
         * Creates a deferred function execution context.
         * Use this helper when you need a command object that you planning on
         * executing at a point in the future
         *
         * @param {Array|Function} func - function or array of functions to run. If array
         *                                is supplied it will automatically form a pipe
         * @param {mixed} args - arguments to execute the function with
         * @return {Function}
         */
        function deferred(func, args) {
            return function () {
                return execute(func, args);
            };
        }

        /**
         * Creates a function that will execute incoming functions with given arguments.
         * Use this helper when you have a list of functions you need to run on the same
         * arguments.
         *
         * @param {mixed} arguments - arguments to execute the function with
         * @return {Function}
         */
        function executeWith() {
            return _.partial(execute, _, sanitizeArguments(arguments));
        }

        /**
         * Creates a pipeline of functions left to right that when executed
         * will pass the result of the previous function as an argument
         * to the next function.
         *
         * @param {Array|Object} funcs - pipeline filter functions
         * @return {Function}
         */
        function pipe(funcs) {
            return function (initialArgument) {
                var result = initialArgument;

                for (var i in funcs) {
                    if (funcs.hasOwnProperty(i) && funcs[i] !== undefined) {
                        result = execute(funcs[i], [result]);
                    }
                }

                return result;
            };
        }


        /**
         * Creates a promise chain that can be executed at later time.
         *
         * @param {Array|Object} funcs - pipeline filter functions
         * @return {Function}
         */
        function chain(funcs) {
            return function (initialArgument) {
                var result = Promise.resolve(initialArgument);

                for (var i in funcs) {
                    if (funcs.hasOwnProperty(i) && funcs[i]) {
                        result = result.then(funcs[i]);
                    }
                }

                return result;
            };
        }

        /**
         * Executes array of filter functions and performs the logical 'or' on the results.
         *
         * @param {Array|Object} funcs - filter functions
         * @return {Function}
         */
        function or(funcs) {
            return function () {
                var args = sanitizeArguments(arguments);

                for (var i in funcs) {
                    if (funcs.hasOwnProperty(i) && !!execute(funcs[i], args)) {
                        return true;
                    }
                }

                return false;
            };
        }

        /**
         * Executes array of filter functions and performs the logical 'and' on the results.
         *
         * @param {Array|Object} funcs - filter functions
         * @return {Function}
         */
        function and(funcs) {
            return function () {
                var args = sanitizeArguments(arguments);

                for (var i in funcs) {
                    if (funcs.hasOwnProperty(i) && !!!execute(funcs[i], args)) {
                        return false;
                    }
                }

                return true;
            };
        }

        /**
         * Negate the result of a function
         *
         * @param {Function} func the function to negate
         * @return {Function}
         */
        function not(func) {
            /**
             * Execute a function with arguments an negate it
             * @param {...string} var_args
             * @return {Boolean}
             */
            return function () {
                var args = sanitizeArguments(arguments);

                return !func.apply(undefined, args);
            };
        }

        return {
            executeWith: executeWith,
            deferred: deferred,
            pipe: pipe,
            chain: chain,
            or: or,
            and: and,
            not: not
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function FunctionHelperFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('FunctionHelperFactory', FunctionHelperFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.FunctionHelperFactory
     * @description
     *
     * Object manipulation functions
     */
    angular.module('origin-components')
        .factory('FunctionHelperFactory', FunctionHelperFactorySingleton);
}());
