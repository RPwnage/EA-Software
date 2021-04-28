/**
 * @file factories/common/nucleusidobfuscate.js
 */
(function() {
    'use strict';
    var CACHE_TIMEOUT = 60 * 60 * 24;

    function NucleusIdObfuscateFactory(CacheFactory) {

       
        return {
            encode: CacheFactory.decorate(Origin.obfuscate.encode, CACHE_TIMEOUT),
            decode: CacheFactory.decorate(Origin.obfuscate.decode, CACHE_TIMEOUT)
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function NucleusIdObfuscateFactorySingleton(CacheFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('NucleusIdObfuscateFactory', NucleusIdObfuscateFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.NucleusIdObfuscateFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('NucleusIdObfuscateFactory', NucleusIdObfuscateFactorySingleton);
}());