/**
 * @file factories/common/featureintro.js
 */
(function() {

    'use strict';

    function FeatureIntroFactory() {
        var myEvents = new Origin.utils.Communicator();
        /**
         * Create and set true session variable
         */
        function updateHiddenGameSessionInfo() {
            if(!sessionStorage.hiddenGameViewed) {
                sessionStorage.setItem('hiddenGameViewed', true);
                myEvents.fire('featureintro:showhidegame');
            }
        }

        return {
            events: myEvents,
            updateHiddenGameSessionInfo: updateHiddenGameSessionInfo
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */

    function FeatureIntroFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('FeatureIntroFactory', FeatureIntroFactory, this, arguments);
    }

    /**
     * @ngdoc service
     *
     * @name origin-components.factories.FeatureIntroFactory
     *
     * @description
     *
     * Stores and processes actions to handle callouts
     * used to introduce users to new features in the SPA
     *
     */
    angular.module('origin-components')
        .factory('FeatureIntroFactory', FeatureIntroFactorySingleton);
}());
