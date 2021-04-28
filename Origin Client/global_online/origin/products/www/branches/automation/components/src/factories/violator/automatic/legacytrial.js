/**
 * @file violator/automatic/trial.js
 */
(function(){

    'use strict';

    function ViolatorLegacyTrialFactory(ViolatorModelFactory, LegacyTrialRefiner) {
        /**
         * Test criteria for trial not activated
         * -no entitlement termination date set
         * -game is a trial
         * @param  {string} offerId  the offerid to test
         * @return {Promise.<Boolean, Error>}  resolves to a boolean
         */
        function trialNotActivated(offerId) {
            return ViolatorModelFactory.getCatalogAndEntitlementData(offerId)
                .then(_.spread(LegacyTrialRefiner.isTrialAwaitingActivation));
        }

        /**
         * Check if trial is not expired
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialNotExpired(offerId) {
            return ViolatorModelFactory.getCatalogAndEntitlementData(offerId)
                .then(_.spread(LegacyTrialRefiner.isTrialInProgress));
        }

        /**
         * Test if a trial is expired
         * @param {string} offerId the offerid to test
         * @return {Promise.<Boolean, Error>} resolves to a boolean
         */
        function trialExpired(offerId) {
            return ViolatorModelFactory.getCatalogAndEntitlementData(offerId)
                .then(_.spread(LegacyTrialRefiner.isTrialExpired));
        }

        return {
            trialNotActivated: trialNotActivated,
            trialNotExpired: trialNotExpired,
            trialExpired: trialExpired,
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function ViolatorLegacyTrialFactorySingleton(ViolatorModelFactory, LegacyTrialRefiner, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ViolatorLegacyTrialFactory', ViolatorLegacyTrialFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ViolatorLegacyTrialFactory

     * @description
     *
     * ViolatorLegacyTrialFactory
     * This Factory supports the legacy Game time trials with a hard termination date field. The refiners in this factory should not be reused.
     */
    angular.module('origin-components')
        .factory('ViolatorLegacyTrialFactory', ViolatorLegacyTrialFactorySingleton);
})();