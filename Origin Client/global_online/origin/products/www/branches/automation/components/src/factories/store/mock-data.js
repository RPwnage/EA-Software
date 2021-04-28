/**
 * @todo remove this file when all the data is available through regular endpoints
 */
(function() {
    'use strict';

    var MOCK_DATA = {
        // criminal activity
        'Origin.OFR.50.0000824': {
            offerId: 'Origin.OFR.50.0000824',
            isUpgradeable: true,
            vault: false,
            vaultOcdPath: '/battlefield/battlefield-hardline/ultimate-edition',
            showSubsSaveGameWarning: true
        },
        // getaway
        'Origin.OFR.50.0000826': {
            offerId: 'Origin.OFR.50.0000826',
            isUpgradeable: true,
            vault: false,
            vaultOcdPath: '/battlefield/battlefield-hardline/ultimate-edition',
            showSubsSaveGameWarning: true
        },
        // robbery
        'Origin.OFR.50.0000825': {
            offerId: 'Origin.OFR.50.0000825',
            isUpgradeable: true,
            vault: false,
            vaultOcdPath: '/battlefield/battlefield-hardline/ultimate-edition',
            showSubsSaveGameWarning: true
        },
        // betrayal
        'Origin.OFR.50.0000827': {
            offerId: 'Origin.OFR.50.0000827',
            isUpgradeable: true,
            vault: false,
            vaultOcdPath: '/battlefield/battlefield-hardline/ultimate-edition',
            showSubsSaveGameWarning: false
        },
        'OFB-EAST:109550763': {
            offerId: 'OFB-EAST:109550763',
            isUpgradeable: true,
            vault: false,
            vaultOcdPath: '/battlefield/battlefield-4/premium-edition',
            showSubsSaveGameWarning: false
        },
        default: {}
    };

    function StoreMockDataFactory(ObjectHelperFactory, $filter) {
        var map = ObjectHelperFactory.map,
            getProperty = ObjectHelperFactory.getProperty,
            merge = ObjectHelperFactory.merge,
            copy = ObjectHelperFactory.copy,
            toArray = ObjectHelperFactory.toArray;

        function getMock(offerId) {
            if (!MOCK_DATA.hasOwnProperty(offerId)) {
                return MOCK_DATA.default;
            }

            return MOCK_DATA[offerId];
        }

        function getEditionOfferIds(conditions) {
            var mocks = $filter('filter')(toArray(MOCK_DATA), conditions),
                offerIds = map(getProperty('offerId'), mocks);

            return offerIds;
        }

        // Merge defaults into each mock so we don't need to do that on every request
        MOCK_DATA = map(function (item) {
            return merge(copy(MOCK_DATA.default), item);
        }, MOCK_DATA);


        function insertMockData(data) {
            var mock;

            ObjectHelperFactory.forEach(function(value, key) {
                mock = getMock(value.offerId);
                data[key] = _.merge(data[key], mock);
            }, data);

            return Promise.resolve(data);
        }

        return {
            getMock: getMock,
            getEditionOfferIds: getEditionOfferIds,
            insertMockData: insertMockData
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function StoreMockDataFactorySingleton(ObjectHelperFactory, $filter, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('StoreMockDataFactory', StoreMockDataFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.StoreMockDataFactory

     * @description
     *
     * Mock data for missing store services
     */
    angular.module('origin-components')
        .factory('StoreMockDataFactory', StoreMockDataFactorySingleton);
}());
