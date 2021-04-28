/**
 * factory for routing helper service
 * @file factories/routinghelper.js
 */
(function() {
    'use strict';

    var OGD_TOPIC_ROUTE = 'app.game_gamelibrary.ogd.topic',
        OGD_EXPANSIONS_TOPIC = 'expansions';

    function RoutingHelperFactory($state, GamesDataFactory, GamesEntitlementFactory, GameRefiner, AppCommFactory) {
        /**
         * reload the current route
         */
        function reloadCurrentRoute() {
            var params = angular.copy($state.params);
            if ($state.current.name.length) {
                $state.go($state.current, params, {
                    reload: $state.current.name
                });
            }
        }

        // get OGD catalog data and check if it is a basegame,
        // redirect to owend base game if its not a base game OGD request
        function handleOgdRedirect(catalogData) {
            var model = _.first(_.values(catalogData)),
                ownedBasegameId = GamesEntitlementFactory.getBaseGameEntitlementOfferIdByMasterTitleId(_.get(model, 'masterTitleId'));

            if(!GameRefiner.isBaseGame(model) && !_.isNull(ownedBasegameId) && ownedBasegameId !== _.get(model, 'offerId')) {
                $state.go(OGD_TOPIC_ROUTE, {
                    offerid: ownedBasegameId,
                    topic: OGD_EXPANSIONS_TOPIC
                });
            }

            return Promise.resolve();
        }

        // load original ogd offer on error
        function handleOgdRedirectError() {
            return Promise.resolve();
        }

        // check if we should redirect OGD page request to owned basegame
        function checkForOgdRedirect(offerid) {
            return GamesDataFactory.getCatalogInfo([offerid])
                    .then(handleOgdRedirect)
                    .catch(handleOgdRedirectError);
        }
            
        AppCommFactory.events.on('origin-route:reloadcurrentRoute', reloadCurrentRoute);

        return {
            reloadCurrentRoute: reloadCurrentRoute,

            /**
             * Given an offerId, redirects OGD page request to owned basegame for DLC's and addons
             * @param {string} offerId offerId to check
             * @return {Promise}
             * @method checkForOgdRedirect
             */
            checkForOgdRedirect: checkForOgdRedirect
        };
    }



    /**
     * @ngdoc service
     * @name originApp.factories.RoutingHelperFactory
     * @description
     *
     * RoutingHelperFactory
     */
    angular.module('originApp')
        .factory('RoutingHelperFactory', RoutingHelperFactory);

}());