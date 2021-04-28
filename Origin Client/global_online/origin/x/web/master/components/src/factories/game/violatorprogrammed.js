/**
 * @file game/violatorprogrammed.js
 */
(function(){

    'use strict';

    function GameProgrammedViolatorFactory(ObjectHelperFactory, GameViolatorModelFactory) {
        var getProperty = ObjectHelperFactory.getProperty;

        /**
         * Flatten the directive property list into an array of objects
         * @param {Object[]} data [ocdData, directiveName]
         * @return {Object[]} a list of directive property sets [{title: foo,...},...]
         */
        function flattenPropertyList(data) {
            var ocdData = data[0],
                directiveName = data[1],
                propertyList = [];

            if(ocdData && ocdData.length > 0) {
                for(var i = 0; i < ocdData.length; i++) {
                    if(typeof(ocdData[i][directiveName]) === 'object') {
                        propertyList.push(ocdData[i][directiveName]);
                    }
                }
            }

            return propertyList;
        }


        /**
         * Parse the OCD response
         * @param {string} offerId the offerid to use
         * @param {string} parentDirectiveName the name of the parent directive name
         * @param {string} directiveName the directive name containing the programmed information
         * @return {Object[]} the violator collection for game tiles
         */
        function getContent(offerId, parentDirectiveName, directiveName) {
            return Promise.all([
                    GameViolatorModelFactory.getOcd(offerId)
                        .then(getProperty(['gamehub', 'components', parentDirectiveName, 'items'])),
                    directiveName
                ])
                .then(flattenPropertyList);
        }

        return {
            getContent: getContent
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GameProgrammedViolatorFactorySingleton(ObjectHelperFactory, GameViolatorModelFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameProgrammedViolatorFactory', GameProgrammedViolatorFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameProgrammedViolatorFactory
     *
     * @description
     *
     * GameProgrammedViolatorFactory
     */
    angular.module('origin-components')
        .factory('GameProgrammedViolatorFactory', GameProgrammedViolatorFactorySingleton);
})();