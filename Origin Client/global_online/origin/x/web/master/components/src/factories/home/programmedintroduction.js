/**
 * @file factories/home/programmedintroduction.js
 */
(function() {

    'use strict';

    function ProgrammedIntroductionFactory(GamesDataFactory, ObjectHelperFactory, DateHelperFactory) {
        var toArray = ObjectHelperFactory.toArray,
            map = ObjectHelperFactory.map,
            mapWith = ObjectHelperFactory.mapWith,
            transformWith = ObjectHelperFactory.transformWith,
            isInTheFuture = DateHelperFactory.isInTheFuture,
            addDays = DateHelperFactory.addDays;

        /**
         * Get the base game entitlement list - this function is blocked by the games:baseGameEntitlementsUpdate event
         * @return {object{}} an object of objects
         */
        function getBaseGameEntitlements() {
            return GamesDataFactory.baseGameEntitlements();
        }

        /**
         * For each owned base game, add a promise to the request chain
         * @return {Promise<Object, Error>[]} An array of promises to resolve
         */
        function getOcdRequestChain() {
            return map(function(baseGameEntitlement) {
                return GamesDataFactory.getOcdByOfferId(baseGameEntitlement.offerId);
            }, getBaseGameEntitlements());
        }

        /**
         * Compare the showForDays Data from OCD to the entitlement grantdate and add the marketing data to to list if valid
         * @param  {Object[]} data a list of gamehub responses from OCD
         * @return {Object[]} data OCD data fragment that matches the conditions
         */
        function getApplicableIntroductionTileData(data) {
            var applicableIntroductionTiles = [],
                baseGameEntitlements = getBaseGameEntitlements();

            for (var i = 0; i < baseGameEntitlements.length; i++) {
                var grantDate = new Date(baseGameEntitlements[i].grantDate),
                    showForDays = data[i].showForDays;
                if (isInTheFuture(addDays(grantDate, showForDays))) {
                    applicableIntroductionTiles.push(data[i]);
                }
            }

            return applicableIntroductionTiles;
        }

        /**
         * Prepare the tile data for view
         * @return {Object[]} A list of applicable tile data
         */
        function getTileData() {
            return Promise.all(toArray(getOcdRequestChain()))
                .then(mapWith(transformWith({
                    showForDays: ['gamehub', 'components', 'origin-home-discovery-tile-introduction-config', 'showfordays'],
                    title: ['gamehub', 'components', 'origin-home-discovery-tile-introduction-config', 'title'],
                    items: ['gamehub', 'components', 'origin-home-discovery-tile-introduction-config', 'items']
                })))
                .then(getApplicableIntroductionTileData);
        }

        return {
            getTileData: getTileData
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ProgrammedIntroductionFactorySingleton(GamesDataFactory, ObjectHelperFactory, DateHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ProgrammedIntroductionFactory', ProgrammedIntroductionFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ProgrammedIntroductionFactory

     * @description
     *
     * ProgrammedIntroductionFactory
     */
    angular.module('origin-components')
        .factory('ProgrammedIntroductionFactory', ProgrammedIntroductionFactorySingleton);
}());