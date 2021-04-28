/**
 * Jasmine functional test
 */

'use strict';

describe('PIN Recommendation Games Factory', function() {
    var PinRecommendationGamesFactory, GamesDataFactory, GamesEntitlementFactory, GamesCatalogFactory, StoreAgeGateFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            pin: {
                getGamesRecommendation: function() {},
                constants: {
                    GAMES: 'GAMES'
                }
            }
        };

        angular.mock.module('origin-components');

        module(function($provide){
            $provide.factory('GamesDataFactory', function() {
                return {
                    waitForInitialRefreshCompleted: function() {},
                    getCatalogInfo: function() {},
                    getPurchasableOfferByProjectId: function() {},
                    lookUpOfferIdIfNeeded: function() {}
                };
            });

            $provide.factory('GamesEntitlementFactory', function() {
                return {
                    getAllEntitlements: function() {}
                };
            });

            $provide.factory('GamesCatalogFactory', function() {
                return {
                    getOfferIdsFromMasterId: function() {}
                };
            });

            $provide.factory('StoreAgeGateFactory', function() {
                return {
                    isUserUnderGameAgeRequirement: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_PinRecommendationGamesFactory_, _GamesDataFactory_, _GamesEntitlementFactory_, _GamesCatalogFactory_, _StoreAgeGateFactory_) {
        PinRecommendationGamesFactory = _PinRecommendationGamesFactory_;
        GamesDataFactory = _GamesDataFactory_;
        GamesEntitlementFactory = _GamesEntitlementFactory_;
        GamesCatalogFactory = _GamesCatalogFactory_;
        StoreAgeGateFactory = _StoreAgeGateFactory_;
    }));

    describe('retrieveRecommendedGamesToBuy', function() {
            var config = {
                    numrecs: 1,
                    controlGames: [{
                        attributes: {
                            gameName: 'Test Control 1',
                            description: {
                                value: 'Run, leap and fight for freedom in the city of Glass.'
                            },
                            showPrice: 'true',
                            'ocd-path': '/mirrors-edge/mirrors-edge-catalyst/standard-edition'
                        }
                    }]
                },
                recoResponse = {
                    govId: '2600956123462160236',
                    recommendations: [
                        {
                            name: 'origin games',
                            result: {
                                'item_list': [{
                                    masterTitleId: '55482',
                                    projectId: '310270'
                                }],
                                experimentData: {
                                    control: 'no'
                                }
                            },
                            track: {
                                trackingTag: 'R3aPpc'
                            }
                        }
                    ]
                },
                recoControlResponse = {
                    govId: '2600956123462160236',
                    recommendations: [
                        {
                            name: 'origin games',
                            result: {
                                'item_list': [],
                                experimentData: {
                                    control: 'yes'
                                }
                            },
                            track: {
                                trackingTag: 'R3aPpc'
                            }
                        }
                    ]
                },
                catalogInfo = {
                    projectNumber: '310270',
                    offerId: 'SIMS4.OFF.SOLP.0x000000000001D5ED',
                    i18n: {
                        packArtLarge: 'large_pack_art.png',
                        displayName: 'Sims 4 City Living'
                    }
                },
                recGameJsonArray = [{
                    recotype: 'GAMES',
                    recoid: catalogInfo.projectNumber,
                    recoindex: 0,
                    recotag: 'R3aPpc',
                    offerid: catalogInfo.offerId,
                    description: '',
                    showprice: true,
                    items: [{
                        'origin-cta-downloadinstallplay': {
                            offerid: catalogInfo.offerId,
                            type: 'primary'
                        }
                    }]
                }],
                projectNumber = catalogInfo.projectNumber;

        it('will return an array of objects representing the recommended games', function(done) {
            spyOn(GamesDataFactory, 'waitForInitialRefreshCompleted').and.returnValue(Promise.resolve());
            spyOn(window.Origin.pin, 'getGamesRecommendation').and.returnValue(recoResponse);
            spyOn(GamesDataFactory, 'getPurchasableOfferByProjectId').and.returnValue(Promise.resolve(catalogInfo));
            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({'SIMS4.OFF.SOLP.0x000000000001D5ED': catalogInfo}));
            spyOn(StoreAgeGateFactory, 'isUserUnderGameAgeRequirement').and.returnValue(false);

            PinRecommendationGamesFactory.retrieveRecommendedGamesToBuy(config)
                .then(function(recommendedGames) {
                    expect(window.Origin.pin.getGamesRecommendation).toHaveBeenCalledWith([projectNumber], config.numrecs);
                    expect(recommendedGames).toEqual(recGameJsonArray);

                    done();
                });
        });

        it('will return no more than n (=\'numrecs\') recommended games', function(done) {
            spyOn(GamesDataFactory, 'waitForInitialRefreshCompleted').and.returnValue(Promise.resolve());
            spyOn(window.Origin.pin, 'getGamesRecommendation').and.returnValue(recoResponse);
            spyOn(GamesDataFactory, 'getPurchasableOfferByProjectId').and.returnValue(Promise.resolve(catalogInfo));
            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({'SIMS4.OFF.SOLP.0x000000000001D5ED': catalogInfo}));
            spyOn(StoreAgeGateFactory, 'isUserUnderGameAgeRequirement').and.returnValue(false);

            PinRecommendationGamesFactory.retrieveRecommendedGamesToBuy(config)
                .then(function(recommendedGames) {
                    expect(window.Origin.pin.getGamesRecommendation).toHaveBeenCalledWith([projectNumber], config.numrecs);
                    expect(recommendedGames.length).toBeLessThan(config.numrecs + 1);

                    done();
                });
        });

        it('will return an array of objects representing control games', function(done) {
            var bucketConfig = [{
                controlGames: [{
                    attributes: {
                        gameName: 'Test Control 1',
                        description: {
                            value: 'Run, leap and fight for freedom in the city of Glass.'
                        },
                        showPrice: 'true',
                        'ocd-path': '/mirrors-edge/mirrors-edge-catalyst/standard-edition'
                    }
                }],
                directiveName: 'origin-home-discovery-tile-programmable-game',
                feedFunctionName: 'getRecommendedGamesToBuy',
                limit: 1,
                numrecs: 1,
                priority: 5000
            }],
            mirrorsEdgeCatalogInfo = {
                projectNumber: '308903',
                offerId: 'Origin.OFR.50.0001000',
                i18n: {
                    packArtLarge: 'large_pack_art.png',
                    displayName: 'Sims 4 City Living'
                }
            },
            controlGameJsonArray = [{
                recotype: 'GAMES',
                recoid: mirrorsEdgeCatalogInfo.projectNumber,
                recoindex: 0,
                recotag: 'R3aPpc',
                offerid: mirrorsEdgeCatalogInfo.offerId,
                description: bucketConfig[0].controlGames[0].attributes.description.value,
                showprice: true,
                items: [{
                    'origin-cta-downloadinstallplay': {
                        offerid: mirrorsEdgeCatalogInfo.offerId,
                        type: 'primary'
                    }
                }]
            }];

            spyOn(GamesDataFactory, 'waitForInitialRefreshCompleted').and.returnValue(Promise.resolve());
            spyOn(window.Origin.pin, 'getGamesRecommendation').and.returnValue(recoControlResponse);
            spyOn(GamesDataFactory, 'getPurchasableOfferByProjectId').and.returnValue(Promise.resolve(catalogInfo));
            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({'Origin.OFR.50.0001000': mirrorsEdgeCatalogInfo}));
            spyOn(GamesDataFactory, 'lookUpOfferIdIfNeeded').and.returnValue(Promise.resolve('Origin.OFR.50.0001000'));
            spyOn(StoreAgeGateFactory, 'isUserUnderGameAgeRequirement').and.returnValue(false);
            spyOn(PinRecommendationGamesFactory, 'filterControlGames').and.returnValue(Promise.resolve(bucketConfig));

            PinRecommendationGamesFactory.retrieveRecommendedGamesToBuy(config)
                .then(function(recommendedGames) {
                    expect(window.Origin.pin.getGamesRecommendation).toHaveBeenCalledWith([mirrorsEdgeCatalogInfo.projectNumber], config.numrecs);
                    expect(recommendedGames).toEqual(controlGameJsonArray);

                    done();
                });
        });
    });

    describe('filterControlGames', function() {
        var bucketConfig = [{
            usePinRec: true,
            directiveName: 'origin-home-discovery-tile-programmable-game',
            feedData: [{}]
        }, {
            controlGames: [],
            directiveName: 'origin-home-discovery-tile-programmable-game',
            feedFunctionName: 'getRecommendedGamesToBuy'
        }];

        it('will filter out control games marked by \'usePinRec\'', function(done) {
            var expected = [{
                controlGames: [],
                directiveName: 'origin-home-discovery-tile-programmable-game',
                feedFunctionName: 'getRecommendedGamesToBuy'
            }];

            PinRecommendationGamesFactory.filterControlGames(bucketConfig)
                .then(function(result) {
                    expect(result).toEqual(expected);

                    done();
                });
        });
    });

});