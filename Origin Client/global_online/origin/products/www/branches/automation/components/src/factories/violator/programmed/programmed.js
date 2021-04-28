/**
 * @file game/violatorprogrammed.js
 */
(function(){

    'use strict';

    function ViolatorProgrammedFactory(ObjectHelperFactory, ViolatorModelFactory, OcdHelperFactory) {
        var coalesceChildDirectives = OcdHelperFactory.coalesceChildDirectives,
            flattenDirectives = OcdHelperFactory.flattenDirectives;

        /**
         * Parse the OCD response
         * @param {string} offerId the offerid to use
         * @param {string} parentDirectiveName the name of the parent directive name
         * @param {string} directiveName the directive name containing the programmed information
         * @return {Object[]} the violator collection for game tiles
         */
        function getContent(offerId, parentDirectiveName, directiveName) {
            return ViolatorModelFactory.getOcd(offerId)
                .then(coalesceChildDirectives(parentDirectiveName, directiveName))
                .then(flattenDirectives(directiveName));
        }

        return {
            getContent: getContent
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ViolatorProgrammedFactorySingleton(ObjectHelperFactory, ViolatorModelFactory, OcdHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ViolatorProgrammedFactory', ViolatorProgrammedFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorProgrammedFactory
     *
     * @description
     *
     * ViolatorProgrammedFactory
     */
    angular.module('origin-components')
        .factory('ViolatorProgrammedFactory', ViolatorProgrammedFactorySingleton);
})();