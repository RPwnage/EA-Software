/**
 * @file ownership.js
 */
(function() {
    'use strict';

    function OwnershipFactory(ObjectHelperFactory, FunctionHelperFactory) {
        var pipe = FunctionHelperFactory.pipe,
            orderByField = ObjectHelperFactory.orderByField,
            takeHead = ObjectHelperFactory.takeHead,
            filterBy = ObjectHelperFactory.filterBy;

        /**
         * Determines whether user owns a particular edition
         * @param edition
         * @returns {isOwned|*}
         */
        function isOwned(edition) {
            return edition.isOwned;
        }

        /**
         * Determines whether subscription is available for a particular edition
         * @param edition
         * @returns {boolean|*}
         */
        function isSubscriptionAvailable(edition) {
            return edition.isSubscription;
        }

        /**
         * Gets the top edition user has purchased
         * @param editions to filter through
         * @returns {*}
         */
        function getTopOwnedEdition(editions) {
            return getTopEditionByFilter(isOwned)(editions);
        }

        /**
         * Get the top edition user has purchased
         * @param editions
         * @returns {*}
         */
        function getTopSubscriptionEdition(editions) {
            return getTopEditionByFilter(isSubscriptionAvailable)(editions);
        }

        /**
         * Filters editions by given filter and orders by weight
         * @param filterField
         * @returns {Function}
         */
        function getTopEditionByFilter(filterField) {
            return pipe([filterBy(filterField), orderByField('weight'), takeHead]);
        }

        /**
         * Has at least one subscribe-able edition
         * @param editions
         * @returns {*}
         */
        function hasSubscriptionEdition(editions) {
            return getTopSubscriptionEdition(editions);
        }

        /**
         * Whether user owns a better edition
         * @param editions
         * @param model
         * @returns {*|boolean}
         */
        function userOwnsBetterEdition(editions, model) {
            var topOwnedEdition = getTopOwnedEdition(editions);
            return (topOwnedEdition && topOwnedEdition.weight > model.weight);
        }

        /**
         *
         * @param editions
         * @param model
         * @returns {*}
         */
        function getPrimaryEdition(editions, model) {
            if (userOwnsBetterEdition(editions, model)) {
                return getTopOwnedEdition(editions);
            } else if (model.isOwned) {
                return model;
            } else if (hasSubscriptionEdition(editions)) {
                return getTopSubscriptionEdition(editions);
            } else {
                return model;
            }
        }

        return {
            getPrimaryEdition: getPrimaryEdition,
            getTopOwnedEdition: getTopOwnedEdition,
            getTopSubscriptionEdition: getTopSubscriptionEdition,
            hasSubscriptionEdition: hasSubscriptionEdition,
            userOwnsBetterEdition: userOwnsBetterEdition
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function OwnershipFactorySingleton(ObjectHelperFactory, FunctionHelperFactory, $rootScope, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('OwnershipFactory', OwnershipFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OwnershipFactory
     * @description
     *
     * Ownership: adds dynamic filtering and scope binding to observable data
     */
    angular.module('origin-components')
        .factory('OwnershipFactory', OwnershipFactorySingleton);
}());
