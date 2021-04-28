/**
 * @file array-helper.js
 */
(function() {
    'use strict';

    function ArrayHelperFactory() {

        /**
         * Array shuffle function built on Fisher Yates
         * @param  {[type]} array    array to shuffle
         * @param  {[type]} randomFn random function to use
         */
        function shuffle(array, randomFn) {
            var i = 0,
                j = 0,
                temp = null;


            for (i = array.length - 1; i > 0; i--) {
                //pick a random index
                j = Math.floor(randomFn() * (i + 1));

                //do the swap
                temp = array[i];
                array[i] = array[j];
                array[j] = temp;
            }
        }

        /**
         * Concats all array parameters
         *
         */
        function concat(){
            if (arguments.length === 0){
                return [];
            }
            var results = [];
            _.forEach(arguments, function(argument){
                if (_.isArray(argument)) {
                    results.push.apply(results, argument);
                }
            });
            return results;
        }

        return {
            shuffle: shuffle,
            concat: concat
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ArrayHelperFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ArrayHelperFactory', ArrayHelperFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ArrayHelperFactory
     * @description
     *
     * Array manipulation functions
     */
    angular.module('origin-components')
        .factory('ArrayHelperFactory', ArrayHelperFactorySingleton);
}());