/**
 * Jasmine functional test
 */

'use strict';

describe('Ogd Header Observer factory', function() {
    var ogdHeaderObserverFactory,
        observerFactory,
        entitlementStateRefiner,
        gamesDataFactory,
        mockScope,
        CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-header';

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
            }
        };

        angular.mock.module('origin-components');

        module(function($provide){
            $provide.factory('GamesDataFactory', function() {
                return {
                    getOcdByOfferId: function() {},
                    getCatalogInfo: function() {
                        return Promise.resolve({
                            'OFB-EAST:12345': {
                                offerId: 'OFB-EAST:12345',
                                i18n: {
                                    displayName: 'game foo'
                                }
                            }
                        });
                    },
                    getEntitlement: function() {},
                    getClientGamePromise: function() {},
                    isTrialExpired: function() {},
                    isSubscriptionEntitlement: function() {},
                    isNonOriginGame: function() {},
                    events: {
                        on: function(){}
                    },
                    isPrivate: function() {}
                };
            });
            $provide.factory('SubscriptionFactory', function() {
                return {
                    isActive: function() {}
                };
            });
            $provide.factory('UtilFactory', function() {
                return {
                    getLocalizedStr: function() {}
                };
            });
            $provide.factory('BeaconFactory', function() {
                return {
                    isInstallable: function() {}
                };
            });
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    error: function() {}
                };
            });
            $provide.factory('CQTargetingFactory', function() {
                return {};
            });
            $provide.factory('ObservableFactory', function() {
                return {
                    observe: function() {
                        return {
                            commit: function() {}
                        };
                    }
                };
            });
            $provide.factory('EntitlementStateRefiner', function() {
                return {
                    isGameActionable: function() {}
                };
            });
            $provide.factory('ObserverFactory', function() {
                return {
                    create: function() {
                        return {
                            addFilter: function() {
                                return this;
                            },
                            attachToScope: function() {
                                return this;
                            }
                        };
                    },
                    noDigest: function() {}
                };
            });
        });

        mockScope = {
            $on: function() {},
            $destroy: function() {}
        };
    });

    beforeEach(inject(function(_OgdHeaderObserverFactory_, _ObserverFactory_, _GamesDataFactory_, _EntitlementStateRefiner_) {
        ogdHeaderObserverFactory = _OgdHeaderObserverFactory_;
        observerFactory = _ObserverFactory_;
        gamesDataFactory = _GamesDataFactory_;
        entitlementStateRefiner = _EntitlementStateRefiner_;
    }));

    describe('getObserver', function() {
        it('will provide an observer handle', function() {
            var offerId = 'OFB-EAST:12345',
                bindTo = 'header';

            var observer = ogdHeaderObserverFactory.getObserver(offerId, CONTEXT_NAMESPACE, mockScope, bindTo);

            expect(typeof(observer)).toEqual('object');
        });
    });

    describe('getModel', function() {
        it('will build a view model for the controller', function(done) {
            spyOn(gamesDataFactory, 'getOcdByOfferId').and.returnValue(Promise.resolve({
                'gamehub': {
                    'components': {
                        'items': [{
                            'origin-gamelibrary-ogd-header': {
                                'backgroundvideoid': '4pY3hlQEOc0',
                                'backgroundvideoid-age-gate': 'no',
                                'gamelogo': 'https://dev.data3.origin.com/preview/content/dam/originx/web/app/games/battlefield/battlefield-3/battlefield-3-standard-edition_logo_800x344_en_WW.png',
                                'gamepremiumlogo': 'https://dev.data4.origin.com/preview/content/dam/originx/web/app/games/battlefield/battlefield-3/Premium_Badge.png',
                                'items': []
                            }
                        }]
                    }
                }
            }));

            spyOn(entitlementStateRefiner, 'isGameActionable').and.returnValue(true);

            spyOn(gamesDataFactory, 'isPrivate').and.returnValue(false);

            spyOn(gamesDataFactory, 'isSubscriptionEntitlement').and.returnValue(true);

            spyOn(gamesDataFactory, 'isNonOriginGame').and.returnValue(false);

            ogdHeaderObserverFactory.getModel('OFB-EAST:12345', CONTEXT_NAMESPACE)
                .then(function(model) {
                    expect(model.backgroundVideoId).toBe('4pY3hlQEOc0');
                    expect(model.backgroundImage).toBe(undefined);
                    expect(model.gameLogo).toBe('https://dev.data3.origin.com/preview/content/dam/originx/web/app/games/battlefield/battlefield-3/battlefield-3-standard-edition_logo_800x344_en_WW.png');
                    expect(model.gamePremiumLogo).toBe('https://dev.data4.origin.com/preview/content/dam/originx/web/app/games/battlefield/battlefield-3/Premium_Badge.png');
                    expect(model.gameName).toBe('game foo');
                    expect(model.packartLarge).toBe(undefined);
                    expect(model.isNog).toBe(false);
                    expect(model.accessLogo).toBe(true);
                    expect(model.isGameActionable).toBe(true);
                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });

        });
    });
});