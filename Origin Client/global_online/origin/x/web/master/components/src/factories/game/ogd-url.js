/**
 * @file store/ogd-url.js
 */
(function() {
    'use strict';

    function OgdUrlFactory(AppCommFactory) {

        /**
         * Fire an event to navigate to the OGD (Owned Game Details) page
         * @param  {Object} offerData an instance of
         */
        function goToOgd(offerData) {
            AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd', {
                offerid: offerData.offerId
            });
        }

        return {
            goToOgd: goToOgd
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OgdUrlFactorySingleton(AppCommFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OgdUrlFactory', OgdUrlFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OgdUrlFactory

     * @description
     *
     * Owned game details URL handler
     */
    angular.module('origin-components')
        .factory('OgdUrlFactory', OgdUrlFactorySingleton);
}());
