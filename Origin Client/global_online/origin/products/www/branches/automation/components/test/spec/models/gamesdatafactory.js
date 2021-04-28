/**
 * Jasmine functional test
 */

'use strict';

describe('Games Data Factory', function() {
    var GamesDataFactory,
        SettingsFactory,
        DialogFactory,
        GamesDirtybitsFactory,
        SubscriptionFactory,
        GamesPricingFactory,
        GamesClientFactory,
        GamesCatalogFactory,
        GamesEntitlementFactory,
        GamesOcdFactory,
        GamesNonOriginFactory,
        ComponentsLogFactory,
        ObjectHelperFactory,
        AuthFactory,
        AppCommFactory,
        PdpUrlFactory,
        GamePropertiesFactory,
        GamesCommerceFactory,
        EntitlementStateRefiner,
        GameRefiner,
        GamesTrialFactory,
        FeatureIntroFactory,
        mockScope;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            'utils': {
                'Communicator': function() {}
            },
            'client': {
                games: {
                    isGamePlaying: function() {}
                }
            },
            'locale': {
                locale: function() {
                    return 'en_US';
                }
            },
            'events': {
                'on': function() {}
            }
        };

        angular.mock.module('origin-components');

        module(function($provide){

            $provide.factory('SettingsFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('DialogFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesDirtybitsFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('SubscriptionFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesPricingFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesClientFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesCatalogFactory', function() {
                return {
                    getPath2OfferIdMap: function() {},
                    retrieveCriticalCatalogInfo: function() {
                        return Promise.resolve({});
                    },
                    getPurchasableOfferIdByMasterTitleId: function() {},
                    getCatalogInfo: function() {},
                    getCatalog: function() {},
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesEntitlementFactory', function() {
                return {
                    ownsEntitlement: function() {},
                    isSubscriptionEntitlement: function() {},
                    getBaseGameEntitlementByMasterTitleId: function() {},
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesOcdFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesNonOriginFactory', function() {
                return {
                    getCatalogInfo: function() {},
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('ComponentsLogFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('AuthFactory', function() {
                return {
                    waitForAuthReady: function() {
                        return Promise.resolve({});
                    },
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('AppCommFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('PdpUrlFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamePropertiesFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesCommerceFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('EntitlementStateRefiner', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GameRefiner', function() {
                return {
                    isPurchasable: function() {},
                    isBaseGame: function() {},
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesTrialFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('FeatureIntroFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });
        });

        mockScope = {
            $on: function() {},
            $destroy: function() {}
        };
    });

    beforeEach(inject(function(
        _GamesDataFactory_,
        _SettingsFactory_,
        _DialogFactory_,
        _GamesDirtybitsFactory_,
        _SubscriptionFactory_,
        _GamesPricingFactory_,
        _GamesClientFactory_,
        _GamesCatalogFactory_,
        _GamesEntitlementFactory_,
        _GamesOcdFactory_,
        _GamesNonOriginFactory_,
        _ComponentsLogFactory_,
        _ObjectHelperFactory_,
        _AuthFactory_,
        _AppCommFactory_,
        _PdpUrlFactory_,
        _GamePropertiesFactory_,
        _GamesCommerceFactory_,
        _EntitlementStateRefiner_,
        _GameRefiner_,
        _GamesTrialFactory_,
        _FeatureIntroFactory_
    ) {
        GamesDataFactory = _GamesDataFactory_;
        SettingsFactory = _SettingsFactory_;
        DialogFactory = _DialogFactory_;
        GamesDirtybitsFactory = _GamesDirtybitsFactory_;
        SubscriptionFactory = _SubscriptionFactory_;
        GamesPricingFactory = _GamesPricingFactory_;
        GamesClientFactory = _GamesClientFactory_;
        GamesCatalogFactory = _GamesCatalogFactory_;
        GamesEntitlementFactory = _GamesEntitlementFactory_;
        GamesOcdFactory = _GamesOcdFactory_;
        GamesNonOriginFactory = _GamesNonOriginFactory_;
        ComponentsLogFactory = _ComponentsLogFactory_;
        ObjectHelperFactory = _ObjectHelperFactory_;
        AuthFactory = _AuthFactory_;
        AppCommFactory = _AppCommFactory_;
        PdpUrlFactory = _PdpUrlFactory_;
        GamePropertiesFactory = _GamePropertiesFactory_;
        GamesCommerceFactory = _GamesCommerceFactory_;
        EntitlementStateRefiner = _EntitlementStateRefiner_;
        GameRefiner = _GameRefiner_;
        GamesTrialFactory = _GamesTrialFactory_;
        FeatureIntroFactory = _FeatureIntroFactory_;
    }));

    describe('getOtherOwnedVersion', function() {
        it('will return undefined if there are no other versions', function() {
            var ocdPath = '/battlefield/battlefield-4/standard-edition',
                versions = ['Origin.OFR.50.000123'];

            spyOn(GamesCatalogFactory, 'getPath2OfferIdMap').and.returnValue(versions);
            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(false);
            spyOn(GamesEntitlementFactory, 'isSubscriptionEntitlement').and.returnValue(false);

            expect(GamesDataFactory.getOtherOwnedVersion(ocdPath, false)).toEqual(undefined);
        });

        it('will return undefined if there are no other owned versions', function() {
            var ocdPath = '/battlefield/battlefield-4/standard-edition',
                versions = ['Origin.OFR.50.000123', 'Origin.OFR.50.000456'];

            spyOn(GamesCatalogFactory, 'getPath2OfferIdMap').and.returnValue(versions);
            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(false);
            spyOn(GamesEntitlementFactory, 'isSubscriptionEntitlement').and.returnValue(false);

            expect(GamesDataFactory.getOtherOwnedVersion(ocdPath, false)).toEqual(undefined);
        });

        it('will return an offer id if there is another owned versions', function() {
            var ocdPath = '/battlefield/battlefield-4/standard-edition',
                versions = ['Origin.OFR.50.000123', 'Origin.OFR.50.000456'];

            spyOn(GamesCatalogFactory, 'getPath2OfferIdMap').and.returnValue(versions);
            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(true);
            spyOn(GamesEntitlementFactory, 'isSubscriptionEntitlement').and.returnValue(false);

            expect(GamesDataFactory.getOtherOwnedVersion(ocdPath, false)).toEqual('Origin.OFR.50.000123');
        });

        it('will return an offer id if there is another vault owned versions', function() {
            var ocdPath = '/battlefield/battlefield-4/standard-edition',
                versions = ['Origin.OFR.50.000123', 'Origin.OFR.50.000456'];

            spyOn(GamesCatalogFactory, 'getPath2OfferIdMap').and.returnValue(versions);
            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(false);
            spyOn(GamesEntitlementFactory, 'isSubscriptionEntitlement').and.returnValue(true);

            expect(GamesDataFactory.getOtherOwnedVersion(ocdPath, true)).toEqual('Origin.OFR.50.000123');
        });

        it('will return undefined if there is not another vault owned versions', function() {
            var ocdPath = '/battlefield/battlefield-4/standard-edition',
                versions = ['Origin.OFR.50.000123', 'Origin.OFR.50.000456'];

            spyOn(GamesCatalogFactory, 'getPath2OfferIdMap').and.returnValue(versions);
            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(false);
            spyOn(GamesEntitlementFactory, 'isSubscriptionEntitlement').and.returnValue(false);

            expect(GamesDataFactory.getOtherOwnedVersion(ocdPath, true)).toEqual(undefined);
        });
    });

    describe('getPurchasableOfferByProjectId', function() {
        it('will return catalog info of purchasable basegame if project id is of basegame, and basegame is available for purchase by user', function(done) {
            var projectId = '310994',
                possibleOffers = [{
                    projectNumber: '310994',
                    offerId: 'Origin.OFR.50.0001665',
                    i18n: {
                        displayName: 'Battlefied 1 Ultimate Edition'
                    }
                }];

            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(false);
            spyOn(GameRefiner, 'isPurchasable').and.returnValue(true);
            spyOn(GameRefiner, 'isBaseGame').and.returnValue(true);
            spyOn(GamesEntitlementFactory, 'getBaseGameEntitlementByMasterTitleId').and.returnValue({'offerId': null});
            spyOn(GamesCatalogFactory, 'getPurchasableOfferIdByMasterTitleId').and.returnValue(Promise.resolve('Origin.OFR.50.0001665'));
            spyOn(GamesNonOriginFactory, 'getCatalogInfo').and.returnValue({});
            spyOn(GamesCatalogFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({'Origin.OFR.50.0001665': possibleOffers[0]}));

            GamesDataFactory.getPurchasableOfferByProjectId( projectId, possibleOffers )
                .then(function(recOffer) {
                    expect(recOffer.offerId).toEqual(possibleOffers[0].offerId);
                    done();
                })
                .catch(function(err) {
                    this.fail(err);
                    done();
                });
        });

        it('will return undefined if project id is of basegame, but user owns it and it is a normal game', function(done) {
            var projectId = '310994',
                possibleOffers = [{
                    projectNumber: '310994',
                    offerId: 'Origin.OFR.50.0001665',
                    i18n: {
                        displayName: 'Battlefied 1 Ultimate Edition'
                    }
                }];

            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(false);
            spyOn(GameRefiner, 'isPurchasable').and.returnValue(true);
            spyOn(GameRefiner, 'isBaseGame').and.returnValue(true);
            spyOn(GamesEntitlementFactory, 'getBaseGameEntitlementByMasterTitleId').and.returnValue({'offerId': 'Origin.OFR.50.0001665'});

            GamesDataFactory.getPurchasableOfferByProjectId( projectId, possibleOffers )
                .then(function() {
                    this.fail(Error('getPurchasableOfferByProjectId() should not be resolved'));
                    done();
                })
                .catch(function(err) {
                    expect(err).toEqual(Error('getPurchasableOfferByProjectId: cannot find base game by projectid'));
                    done();
                });
        });

        it('will return catalog info of DLC if project id is of DLC, and DLC is available for purchase by user', function(done) {
            var projectId = '310270',
                possibleOffers = [{
                    projectNumber: '310270',
                    offerId: 'SIMS4.OFF.SOLP.0x000000000001D5ED',
                    i18n: {
                        displayName: 'Sims 4 City Living'
                    },
                }];

            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(false);
            spyOn(GameRefiner, 'isPurchasable').and.returnValue(true);
            spyOn(GameRefiner, 'isBaseGame').and.returnValue(false);

            GamesDataFactory.getPurchasableOfferByProjectId( projectId, possibleOffers )
                .then(function(recOffer) {
                    expect(recOffer.offerId).toEqual(possibleOffers[0].offerId);
                    done();
                })
                .catch(function(err) {
                    this.fail(err);
                    done();
                });
        });

        it('will return undefined if project id is of DLC, and no DLC is available for purchase by user', function(done) {
            var projectId = '310270',
                possibleOffers = [{
                    projectNumber: '310270',
                    offerId: 'SIMS4.OFF.SOLP.0x000000000001D5ED',
                    i18n: {
                        displayName: 'Sims 4 City Living'
                    },
                }];

            spyOn(GamesEntitlementFactory, 'ownsEntitlement').and.returnValue(true);
            spyOn(GameRefiner, 'isPurchasable').and.returnValue(true);
            spyOn(GameRefiner, 'isBaseGame').and.returnValue(false);

            GamesDataFactory.getPurchasableOfferByProjectId( projectId, possibleOffers )
                .then(function() {
                    this.fail(Error('getPurchasableOfferByProjectId() should not be resolved'));
                    done();
                })
                .catch(function(err) {
                    expect(err).toEqual(Error('getPurchasableOfferByProjectId: cannot find dlc game by projectid'));
                    done();
                });
        });
    });
});