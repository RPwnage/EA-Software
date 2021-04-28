/**
 * @todo remove this file when all the data is available through regular endpoints
 */
(function() {
    'use strict';

    var MOCK_DATA = {
        'Origin.OFR.50.0001094': {
            offerId: 'Origin.OFR.50.0001094',
            franchiseKey: 'command-and-conquer',
            gameKey: 'red-alert',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition (preload)'
        },
        'Origin.OFR.50.0001107': {
            offerId: 'Origin.OFR.50.0001107',
            franchiseKey: 'origin',
            gameKey: 'sdk-tool',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition (preload)'
        },
        'DR:102427200': {
            offerId: 'DR:102427200',
            franchiseKey: 'mass-effect',
            gameKey: 'mass-effect',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition'
        },
        'Origin.OFR.50.0000978': {
            offerId: 'Origin.OFR.50.0000978',
            franchiseKey: 'fifa',
            gameKey: 'fifa-16',
            editionKey: 'deluxe-edition',
            editionName: 'Deluxe Edition',
            weight: 2
        },
        'Origin.OFR.50.0000979': {
            offerId: 'Origin.OFR.50.0000979',
            franchiseKey: 'fifa',
            gameKey: 'fifa-16',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            price: 39.99,
            strikePrice: 59.99,
            savings: 25,
            weight: 1
        },
        'Origin.OFR.50.0000980': {
            offerId: 'Origin.OFR.50.0000980',
            franchiseKey: 'fifa',
            gameKey: 'fifa-16',
            editionKey: 'super-deluxe-edition',
            editionName: 'Super Deluxe Edition',
            weight: 3
        },
        'Origin.OFR.50.0000912': {
            offerId: 'Origin.OFR.50.0000912',
            franchiseKey: 'fifa',
            gameKey: 'fifa-16',
            editionKey: 'demo',
            editionName: 'Standard Edition (Demo)',
            weight: 0
        },
        'Origin.OFR.50.0000914': {
            offerId: 'Origin.OFR.50.0000914',
            franchiseKey: 'fifa',
            gameKey: 'fifa-16-points',
            editionKey: '100-fifa-16-points',
            editionName: '100 Points',
            weight: 3,
            isBaseGameRequired: true,
            baseGameOfferId: 'Origin.OFR.50.0000979'
        },
        'Origin.OFR.50.0000915': {
            offerId: 'Origin.OFR.50.0000915',
            franchiseKey: 'fifa',
            gameKey: 'fifa-16-points',
            editionKey: '250-fifa-16-points',
            editionName: '250 Points',
            weight: 2,
            isBaseGameRequired: true,
            baseGameOfferId: 'Origin.OFR.50.0000979'
        },
        'Origin.OFR.50.0000916': {
            offerId: 'Origin.OFR.50.0000916',
            franchiseKey: 'fifa',
            gameKey: 'fifa-16-points',
            editionKey: '500-fifa-16-points',
            editionName: '500 Points',
            weight: 1,
            isBaseGameRequired: true,
            baseGameOfferId: 'Origin.OFR.50.0000979'
        },
        'Origin.OFR.50.0000426': {
            offerId: 'Origin.OFR.50.0000426',
            franchiseKey: 'battlefield',
            gameKey: 'hardline',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            weight: 1,
            oth: 'oth',
            useEndDate: '2015-12-31'
        },
        'Origin.OFR.50.0000846': {
            offerId: 'Origin.OFR.50.0000846',
            franchiseKey: 'battlefield',
            gameKey: 'hardline',
            editionKey: 'ultimate-edition',
            editionName: 'Ultimate Edition',
            weight: 2
        },
        'Origin.OFR.50.0000708': {
            offerId: 'Origin.OFR.50.0000708',
            franchiseKey: 'battlefield',
            gameKey: 'hardline-battlepacks',
            editionKey: 'battlefield-hardline-5-x-gold-battlepacks',
            editionName: '5X Gold Battlepacks',
            weight: 1,
            isBaseGameRequired: true,
            baseGameOfferId: 'Origin.OFR.50.0000426'
        },
        'OFB-EAST:109549060': {
            offerId: 'OFB-EAST:109549060',
            franchiseKey: 'battlefield',
            gameKey: 'battlefield-4',
            editionKey: 'digital-deluxe-edition',
            editionName: 'Digital Deluxe Edition',
            weight: 2
        },
        'OFB-EAST:109546867': {
            offerId: 'OFB-EAST:109546867',
            franchiseKey: 'battlefield',
            gameKey: 'battlefield-4',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            weight: 1
        },
        'OFB-EAST:109550761': {
            offerId: 'OFB-EAST:109550761',
            franchiseKey: 'battlefield',
            gameKey: 'second-assault',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            weight: 1,
            oth: 'oth'
        },
        'Origin.OFR.50.0000500': {
            offerId: 'Origin.OFR.50.0000500',
            franchiseKey: 'theme-hospital',
            gameKey: 'theme-hospital',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            weight: 1,
            oth: 'oth'
        },
        'OFB-EAST:109552299': {
            offerId: 'OFB-EAST:109552299',
            franchiseKey: 'sims',
            gameKey: 'sims-4',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            weight: 1,
            oth: 'oth'
        },
        'OFB-EAST:55320': {
            offerId: 'OFB-EAST:55320',
            franchiseKey: 'monopoly',
            gameKey: 'monopoly',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            weight: 1,
            oth: 'oth'
        },
        'OFB-EAST:109552316': {
            offerId: 'OFB-EAST:109552316',
            franchiseKey: 'battlefield',
            gameKey: 'battlefield-4',
            editionKey: 'premium-edition',
            editionName: 'Premium Edition',
            weight: 3,
            media: [
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_03.jpg'
                }
            ]
        },
        'Origin.OFR.50.0000488': {
            offerId: 'Origin.OFR.50.0000488',
            franchiseKey: 'titanfall',
            gameKey: 'titanfall',
            editionKey: 'game-time',
            editionName: 'Game Time',
            weight: 0,
            media: [
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_03.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_04.jpg'
                }
            ]
        },
        'OFB-EAST:109545763': {
            offerId: 'OFB-EAST:109545763',
            franchiseKey: 'kingdoms-of-amalur',
            gameKey: 'reckoning',
            editionKey: 'game-time',
            editionName: 'Game Time',
            weight: 0,
            media: [
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_03.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_04.jpg'
                }
            ]
        },
        'OFB-EAST:109552153': {
            offerId: 'OFB-EAST:109552153',
            franchiseKey: 'battlefield',
            gameKey: 'battlefield-4',
            editionKey: 'game-time',
            editionName: 'Game Time',
            weight: 0,
            media: [
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_03.jpg'
                }
            ]
        },
        'DR:106479900': {
            offerId: 'DR:106479900',
            franchiseKey: 'burnout',
            gameKey: 'burnout-paradise',
            editionKey: 'ultimate-edition',
            editionName: 'Ultimate Edition'
        },
        'Origin.OFR.50.0000595' : {
            offerId: 'Origin.OFR.50.0000595',
            weight: 1
        },
        'Origin.OFR.50.0000871': {
            offerId: 'Origin.OFR.50.0000871',
            franchiseKey: 'star-wars',
            gameKey: 'battlefront',
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            media: [
                {
                    type: 'video',
                    videoId: 'grwZs9zp5wA'
                },
                {
                    type: 'video',
                    videoId: 'hlEKa6tUPlU'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_03.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_04.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_03.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/kingdoms_04.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_03.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/titanfall_04.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_01.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_02.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_03.jpg'
                },
                {
                    type: 'image',
                    src: 'http://docs.x.origin.com/origin-x-design-prototype/images/screenshots/bf4_04.jpg'
                }
            ],
            weight: 1
        },
        'Origin.OFR.50.0000907': {
            offerId: 'Origin.OFR.50.0000907',
            franchiseKey: 'star-wars',
            gameKey: 'battlefront',
            editionKey: 'deluxe-edition',
            editionName: 'Deluxe Edition',
            weight: 2
        },
        'OFB-EAST:52735': {
            offerId: 'OFB-EAST:52735'
        },
        'Origin.OFR.50.0000675': {
            offerId: 'Origin.OFR.50.0000675',
            franchiseKey: 'dragon-age',
            gameKey: 'inquisition',
            weight: 1
        },
        'OFB-EAST:60531': {
            offerId: 'OFB-EAST:60531',
            franchiseKey: 'syndicate',
            gameKey: 'syndicate',
            weight: 1
        },
        'OFB-EAST:39471': {
            offerId: 'OFB-EAST:39471',
            franchiseKey: 'ultima',
            gameKey: 'ultima',
            editionKey: 'gold-edition',
            editionName: 'Gold Edition',
            weight: 1
        },
        'Origin.OFR.50.0000462': {
            offerId: 'Origin.OFR.50.0000462',
            franchiseKey: 'fifa',
            gameKey: 'fifa-15',
            weight: 1
        },
        'default': {
            editionKey: 'standard-edition',
            editionName: 'Standard Edition',
            systemRequirements: '<b>Minimum System Requirements</b><br>• OS: Windows Vista SP2 32-bit<br>• Processor (AMD): Athlon X2 2.8 GHz<br>• Processor (Intel): Core 2 Duo 2.4 GHz<br>• Memory: 4 GB<br>• Hard Drive: 30 GB<br>• Graphics card (AMD): AMD Radeon HD 3870<br>• Graphics card (NVIDIA): NVIDIA GeForce 8800 GT<br>• Graphics memory: 512 MB<br><br><b>Recommended System Requirements</b><br>• OS: Windows 8 64-bit<br>• Processor (AMD): Six-core CPU<br>• Processor (Intel): Quad-core CPU<br>• Memory: 8 GB<br>• Hard Drive: 30 GB<br>• Graphics card (AMD): AMD Radeon HD 7870<br>• Graphics card (NVIDIA): NVIDIA GeForce GTX 660<br>• Graphics memory: 3 GB',
            genre: '<a class="otka" href="#">Shooter</a>, <a class="otka" href="#">Action</a>',
            publisher: '<a class="otka" href="#">Electronic Arts</a>',
            // developer: '<a class="otka" href="#">DICE</a>',
            supportedLanguages: 'Deutsch (DE), English (US), Espanol (ES), Francais (FR), Italiano (IT)',
            links: '<a class="otka" href="#">Official Site</a><br><a class="otka" href="#">Forums</a>',
            termsAndConditions: "INTERNET CONNECTION, EA ACCOUNT, ACCEPTANCE OF PRODUCT END USER LICENSE AGREEMENTS (EULAS), INSTALLATION OF THE ORIGIN CLIENT SOFTWARE (www.origin.com/about) AND REGISTRATION WITH ENCLOSED SINGLE-USE SERIAL CODE REQUIRED TO PLAY AND ACCESS ONLINE FEATURES. SERIAL CODE REGISTRATION IS LIMITED TO ONE EA ACCOUNT PER SERIAL CODE. SERIAL CODES ARE NON-TRANSFERABLE ONCE USED AND SHALL BE VALID, AT A MINIMUM, SO LONG AS ONLINE FEATURES ARE AVAILABLE. YOU MUST BE 13+ TO ACCESS ONLINE FEATURES. ADDITIONAL IN-GAME CONTENT MAY BE PURCHASED. EA ONLINE PRIVACY AND COOKIE POLICY AND TERMS OF SERVICE CAN BE FOUND AT www.ea.com. EULAS AND ADDITIONAL DISCLOSURES CAN BE FOUND AT www.ea.com/1/product-eulas. EA MAY RETIRE ONLINE FEATURES 30 DAYS NOTICE POSTED ON www.ea.com/1/service-updates.",
            weight: 1,
            isBaseGameRequired: false
        }
    };

    function StoreMockDataFactory($stateParams, ObjectHelperFactory, $filter) {
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

        return {
            getMock: getMock,
            getEditionOfferIds: getEditionOfferIds
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function StoreMockDataFactorySingleton($stateParams, ObjectHelperFactory, $filter, SingletonRegistryFactory) {
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
