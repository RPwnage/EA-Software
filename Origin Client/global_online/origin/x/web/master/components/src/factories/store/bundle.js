/**
 * @file store/bundle.js
 */
(function() {
    'use strict';

    var STORAGE_KEY = 'origin-store-bundle:';

    function StoreBundleFactory(LocalStorageFactory, ObservableFactory, ObserverFactory, AuthFactory) {
        var bundles = ObservableFactory.observe({}),
            currentBundleId;

        /**
         * Gets bundle ID
         *
         * @return {String}
         */
        function getCurrentBundleId() {
            return currentBundleId;
        }

        /**
         * Saves bundle ID
         *
         * @return {void}
         */
        function setCurrentBundleId(bundleId) {
            currentBundleId = bundleId;
        }

        /**
         * Saves current state of the bundle
         *
         * @return {void}
         */
        function saveAll() {
            bundles.commit();
            LocalStorageFactory.set(STORAGE_KEY, bundles.data);
        }

        /**
         * Returns all the offer ID's from the current bundle
         *
         * @return {Array}
         */
        function getAll() {
            var observer = ObserverFactory
                .create(bundles)
                .getProperty(currentBundleId)
                .defaultTo([]);

            return observer;
        }

        /**
         * Clears the bundle
         *
         * @return {void}
         */
        function clear() {
            bundles.data[currentBundleId] = [];
            saveAll();
        }

        /**
         * Checks if the offer ID is in the bundle or not
         *
         * @param {string} offerId offer ID to check
         * @return {boolean}
         */
        function has(offerId) {
            return !!bundles.data[currentBundleId] && bundles.data[currentBundleId].indexOf(offerId) > -1;
        }

        /**
         * Adds offer ID to the bundle
         *
         * @param {string} offerId offer ID to add
         * @return {void}
         */
        function add(offerId) {
            if (!Array.isArray(bundles.data[currentBundleId])) {
                bundles.data[currentBundleId] = [];
            }

            bundles.data[currentBundleId].unshift(offerId);
            saveAll();
        }

        /**
         * Removes the offer ID from the bundle
         *
         * @param {string} offerId offer ID to remove
         * @return {void}
         */
        function remove(offerId) {
            if (!!!bundles.data[currentBundleId]) {
                return;
            }

            var index = bundles.data[currentBundleId].indexOf(offerId);

            if (index > -1) {
                bundles.data[currentBundleId].splice(index, 1);
            }

            saveAll();
        }

        /**
         * 
         */
        function load() {
            bundles.data = LocalStorageFactory.get(STORAGE_KEY, {});
            bundles.commit();
        }

        AuthFactory.events.on('auth:change', load); 
        AuthFactory.waitForAuthReady().then(load);

        return {
            getAll: getAll,
            has: has,
            clear: clear,
            add: add,
            remove: remove,
            getCurrentBundleId: getCurrentBundleId,
            setCurrentBundleId: setCurrentBundleId
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function StoreBundleFactorySingleton(LocalStorageFactory, ObservableFactory, ObserverFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('StoreBundleFactory', StoreBundleFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.StoreBundleFactory

     * @description
     *
     * Store deals bundle
     */
    angular.module('origin-components')
        .factory('StoreBundleFactory', StoreBundleFactorySingleton);
}());