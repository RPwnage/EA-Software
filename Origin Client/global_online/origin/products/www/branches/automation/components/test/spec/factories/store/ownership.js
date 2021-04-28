/**
 * Jasmine functional test
 */

'use strict';

describe('OwnershipFactory', function () {
    var OwnershipFactory,
        ObjectHelperFactory,
        FunctionHelperFactory,
        GamesDataFactory,
        SubscriptionFactory,
        GamesTrialFactory,
        GameRefiner,
        VaultRefiner,
        AuthFactory;

    beforeEach(function () {
        window.Origin = {
            log: {
                message: function() {}
            },
            utils: {
                Communicator: function() {},
                os: function() {},
                OS_WINDOWS: 'PCWIN'
            },
            events: {
                on: function() {}
            }
        };
        window.OriginComponents = {};
        angular.mock.module('origin-components');

        module(function ($provide) {

            $provide.factory('ObjectHelperFactory', function () {
                return {};
            });

            $provide.factory('FunctionHelperFactory', function () {
                return {};
            });

            $provide.factory('GamesDataFactory', function () {

                var entitlements = [
                    { // currency
                        offerId: '0',
                        externalType : 'DEFAULT',
                        isConsumable: false
                    },
                    { // consumable
                        offerId: '1',
                        externalType : 'DEFAULT',
                        isConsumable: true
                    },
                    { // owned basegame
                        offerId: '2',
                        externalType : 'DEFAULT',
                        isConsumable: false
                    },
                    { // vault owned basegame
                        offerId: '3',
                        externalType : 'SUBSCRIPTIONS',
                        isConsumable: false
                    },
                    { // owned sub item game
                        offerId: '7',
                        externalType : 'DEFAULT',
                        isConsumable: false
                    },
                    { // owned sub item game
                        offerId: '8',
                        externalType : 'DEFAULT',
                        isConsumable: false
                    },
                    { // vault owned sub item game
                        offerId: '10',
                        externalType : 'SUBSCRIPTIONS',
                        isConsumable: false
                    },
                    { // vault owned sub item game
                        offerId: '11',
                        externalType : 'SUBSCRIPTIONS',
                        isConsumable: false
                    },
                    { // hidden shell bundle owned basegame
                        offerId: '13',
                        externalType : 'DEFAULT',
                        isConsumable: false
                    },
                    { // hidden shell bundle vault owned basegame
                        offerId: '14',
                        externalType : 'SUBSCRIPTIONS',
                        isConsumable: false
                    },
                    { // hidden shell bundle content owned basegame
                        offerId: '17',
                        externalType : 'DEFAULT',
                        isConsumable: false
                    },
                    { // hidden shell bundle content owned basegame
                        offerId: '18',
                        externalType : 'DEFAULT',
                        isConsumable: false
                    },
                    { // hidden shell bundle content vault owned basegame
                        offerId: '19',
                        externalType : 'SUBSCRIPTIONS',
                        isConsumable: false
                    },
                    { // hidden shell bundle content vault owned basegame
                        offerId: '20',
                        externalType : 'SUBSCRIPTIONS',
                        isConsumable: false
                    }
                ];

                return {
                    isConsumable: function(offerId) {
                        var ent = entitlements.filter(function(ent) {
                            return ent.offerId === offerId;
                        });

                        if(ent && ent[0] && ent[0].isConsumable === true) {
                            return true;
                        } else {
                            return false;
                        }
                    },
                    ownsEntitlement: function(offerId) {
                        var ent = entitlements.filter(function(ent) {
                            return ent.offerId === offerId;
                        });

                        return ent.length > 0;
                    },
                    isSubscriptionEntitlement: function(offerId) {
                        var ent = entitlements.filter(function(ent) {
                            return ent.offerId === offerId && ent.externalType === 'SUBSCRIPTIONS';
                        });

                        return ent.length > 0;
                    },
                    getOtherOwnedVersion: function() {
                        return undefined;
                    },
                    getCatalogInfo: function() {},
                    getCatalogByPath: function() {},
                    isInstalled: function() {}
                };
            });

            $provide.factory('SubscriptionFactory', function () {
                return {
                    userHasSubscription: function() {}
                };
            });

            $provide.factory('GamesTrialFactory', function () {
                return {
                    getTrialTimeByOfferId: function() {
                        return Promise.resolve();
                    }
                };
            });

            $provide.factory('GameRefiner', function () {
                return {
                    isCurrency: function(game) {
                        return game.type === 'currency';
                    },
                    isShellBundle: function(game) {
                        return game.isShellBundle === true;
                    },
                    isBaseGame: function(game) {
                        return game.type === 'basegame';
                    }
                };
            });

            $provide.factory('AuthFactory', function () {
                return {
                    isAppLoggedIn: function() {
                        return true;
                    }
                };
            });

            $provide.factory('VaultRefiner', function () {
                return {
                    isShellBundle: function(game) {
                        return game.isVaultShellBundle;
                    }
                };
            });

        });
    });

    beforeEach(inject(function (
        _OwnershipFactory_,
        _ObjectHelperFactory_,
        _FunctionHelperFactory_,
        _GamesDataFactory_,
        _SubscriptionFactory_,
        _GamesTrialFactory_,
        _GameRefiner_,
        _VaultRefiner_,
        _AuthFactory_
    ) {
        OwnershipFactory = _OwnershipFactory_;
        ObjectHelperFactory = _ObjectHelperFactory_;
        FunctionHelperFactory = _FunctionHelperFactory_;
        GamesDataFactory = _GamesDataFactory_;
        SubscriptionFactory = _SubscriptionFactory_;
        GamesTrialFactory = _GamesTrialFactory_;
        GameRefiner = _GameRefiner_;
        VaultRefiner = _VaultRefiner_;
        AuthFactory = _AuthFactory_;
    }));

    describe('loadEntitlementsInfo', function () {
        // isRepurchasableOrAnonymous
        // ===============================================================================
        it('Set ownership to false if logged out', function (done) {
            spyOn(AuthFactory, 'isAppLoggedIn').and.returnValue(false);
            OwnershipFactory.loadEntitlementsInfo({}).then(function(result) {
                expect(result.isOwned).toEqual(false);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set ownership to false if currency', function (done) {
            var game = {
                offerId: '0',
                type: 'currency',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
            };

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(false);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });
/*
        it('Set ownership to false if consumable', function (done) {
            var game = {
                offerId: '1',
                type: 'battleback',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
            };

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(false);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });
*/
        // isParentOwned
        // ===============================================================================
        it('Set ownership to true if parent is owned', function (done) {
            var game = {
                offerId: '2',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
            };

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set vault ownership to true if parent is vault owned', function (done) {
            var game = {
                offerId: '3',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
            };

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        function fakeGetOtherVersion(path, vault) {
            if(vault) {
                return undefined;
            } else {
                return '4a';
            }
        }

        // isAnotherVersionOfParentOwned
        // ===============================================================================
        it('Set ownership to true if another version of parent is owned', function (done) {
            var game = {
                offerId: '4',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
            };

            spyOn(GamesDataFactory, 'getOtherOwnedVersion').and.callFake(fakeGetOtherVersion);
            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set vault ownership to true if another version of parent is vault owned', function (done) {
            var game = {
                offerId: '5',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
            };

            spyOn(GamesDataFactory, 'getOtherOwnedVersion').and.returnValue('5a');
            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        // isBundleContentOwned
        // ===============================================================================
        it('Set ownership to true if all bundle content is owned', function (done) {
            var game = {
                offerId: '6',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '7',
                    '8'
                ]
            };

            var children = {
                '7': { // owned shell bundle sub item
                    offerId: '7',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                },
                '8': { // owned shell bundle sub item
                    offerId: '8',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set ownership to true if all bundle content is owned, but some catalog data is unavailable', function (done) {
            var game = {
                offerId: '6',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '7',
                    '8'
                ]
            };

            var children = {
                7: { // owned shell bundle sub item
                    offerId: '7',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set ownership to true if all bundle content is owned, but all catalog data is unavailable', function (done) {
            var game = {
                offerId: '6',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '7',
                    '8'
                ]
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({}));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set ownership to false if bundle content contains a shell bundle and is not owned', function (done) {
            var game = {
                offerId: '6',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '7a',
                    '8b'
                ]
            };

            var children = {
                '7a': { // owned shell bundle sub item
                    offerId: '7a',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: true,
                    bundleOffers: [
                        '7c',
                        '8c'
                    ]
                },
                '8a': { // owned shell bundle sub item
                    offerId: '8a',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(false);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set vault ownership to true if all bundle content is vault owned', function (done) {
            var game = {
                offerId: '9',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '10',
                    '11'
                ]
            };

            var children = {
                10: { // owned shell bundle sub item
                    offerId: '10',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                },
                11: { // owned shell bundle sub item
                    offerId: '11',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        it('Set vault ownership to true if all bundle content is vault owned, but some catalog data is unavailable', function (done) {
            var game = {
                offerId: '9',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '10',
                    '11'
                ]
            };

            var children = {
                10: { // owned shell bundle sub item
                    offerId: '10',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        it('Set vault ownership to true if all bundle content is vault owned, but all catalog data is unavailable', function (done) {
            var game = {
                offerId: '9',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '10',
                    '11'
                ]
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({}));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        it('Set ownership to true if bundle content contains a shell bundle and is owned', function (done) {
            var game = {
                offerId: '6',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '7a',
                    '8a'
                ]
            };

            var children = {
                '7a': { // owned shell bundle sub item
                    offerId: '7a',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: true,
                    bundleOffers: [
                        '7',
                        '8'
                    ]
                },
                '8a': { // owned shell bundle sub item
                    offerId: '8a',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: true,
                    bundleOffers: [
                        '7',
                        '8'
                    ]
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set vault ownership to true if bundle content contains a shell bundle and is vault owned', function (done) {
            var game = {
                offerId: '9',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '10a',
                    '11a'
                ]
            };

            var children = {
                '10a': { // owned shell bundle sub item
                    offerId: '10a',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: true,
                    bundleOffers: [
                        '10',
                        '11'
                    ]
                },
                '11a': { // owned shell bundle sub item
                    offerId: '11a',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: true,
                    bundleOffers: [
                        '10',
                        '11'
                    ]
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        it('Set ownership to false if bundle content contains a shell bundle and not owned', function (done) {
            var game = {
                offerId: '9',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '10a',
                    '11a'
                ]
            };

            var children = {
                '10a': { // owned shell bundle sub item
                    offerId: '10a',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: true,
                    bundleOffers: [
                        '10b',
                        '11b'
                    ]
                },
                '11a': { // owned shell bundle sub item
                    offerId: '11a',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: true,
                    bundleOffers: [
                        '10b',
                        '11b'
                    ]
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(false);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        // isShellBundleOwned
        // ===============================================================================
        it('Set ownership to true if hidden shell bundle is owned', function (done) {
            var game = {
                offerId: '12',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                isVaultShellBundle: true,
                bundleOffers: []
            };

            var hiddenShellBundle = {
                offerId: '13',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
                bundleOffers: []
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({}));
            spyOn(Origin.utils, 'os').and.returnValue('PCWIN');
            spyOn(GamesDataFactory, 'getCatalogByPath').and.returnValue(Promise.resolve(Promise.resolve(hiddenShellBundle)));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        it('Set vault ownership to true if hidden shell bundle is vault owned', function (done) {
            var game = {
                offerId: '12',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                isVaultShellBundle: true,
                bundleOffers: []
            };

            var hiddenShellBundle = {
                offerId: '14',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
                bundleOffers: []
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({}));
            spyOn(Origin.utils, 'os').and.returnValue('PCWIN');
            spyOn(GamesDataFactory, 'getCatalogByPath').and.returnValue(Promise.resolve(Promise.resolve(hiddenShellBundle)));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        it('Set ownership to false if hidden shell bundle is vault owned but on a mac', function (done) {
            var game = {
                offerId: '13a',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
                isVaultShellBundle: true,
                bundleOffers: []
            };

            var hiddenShellBundle = {
                offerId: '13',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
                bundleOffers: []
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({}));
            spyOn(Origin.utils, 'os').and.returnValue('OSX');
            spyOn(GamesDataFactory, 'getCatalogByPath').and.returnValue(Promise.resolve(hiddenShellBundle));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(false);
                expect(result.vaultEntitlement).toEqual(false);
                done();
            });
        });

        // isShellBundleContentOwned
        // ===============================================================================
        it('Set vault ownership to true if hidden shell bundle content is owned', function (done) {
            var game = {
                offerId: '15',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                isVaultShellBundle: true,
                bundleOffers: []
            };

            var hiddenShellBundle = {
                offerId: '16',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '17',
                    '18'
                ]
            };

            var children = {
                17: { // owned shell bundle sub item
                    offerId: '17',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                },
                18: { // owned shell bundle sub item
                    offerId: '18',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));
            spyOn(Origin.utils, 'os').and.returnValue('PCWIN');
            spyOn(GamesDataFactory, 'getCatalogByPath').and.returnValue(Promise.resolve(Promise.resolve(hiddenShellBundle)));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        it('Set vault ownership to true if hidden shell bundle content is vault owned', function (done) {
            var game = {
                offerId: '15',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                isVaultShellBundle: true,
                bundleOffers: []
            };

            var hiddenShellBundle = {
                offerId: '16',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: true,
                bundleOffers: [
                    '19',
                    '20'
                ]
            };

            var children = {
                19: { // owned shell bundle sub item
                    offerId: '19',
                    type: 'basegame',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                },
                20: { // owned shell bundle sub item
                    offerId: '20',
                    type: 'dlc',
                    offerPath: '/franchise/game/edition',
                    isShellBundle: false,
                    bundleOffers: []
                }
            };

            spyOn(GamesDataFactory, 'getCatalogInfo').and.returnValue(Promise.resolve(children));
            spyOn(Origin.utils, 'os').and.returnValue('PCWIN');
            spyOn(GamesDataFactory, 'getCatalogByPath').and.returnValue(Promise.resolve(Promise.resolve(hiddenShellBundle)));

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isOwned).toEqual(true);
                expect(result.vaultEntitlement).toEqual(true);
                done();
            });
        });

        // isInstalledOrSubscription
        // ===============================================================================
        it('Set isInstalled to true and isSubscription to true', function (done) {
            var game = {
                offerId: '21',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
                isVaultShellBundle: false,
                subscriptionAvailable: true,
                bundleOffers: []
            };

            spyOn(GamesDataFactory, 'isInstalled').and.returnValue(true);
            spyOn(SubscriptionFactory, 'userHasSubscription').and.returnValue(true);

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isInstalled).toEqual(true);
                expect(result.isSubscription).toEqual(true);
                done();
            });
        });

        it('Set isInstalled to false and isSubscription to false', function (done) {
            var game = {
                offerId: '21',
                type: 'basegame',
                offerPath: '/franchise/game/edition',
                isShellBundle: false,
                isVaultShellBundle: false,
                subscriptionAvailable: false,
                bundleOffers: []
            };

            spyOn(GamesDataFactory, 'isInstalled').and.returnValue(false);
            spyOn(SubscriptionFactory, 'userHasSubscription').and.returnValue(false);

            OwnershipFactory.loadEntitlementsInfo(game).then(function(result) {
                expect(result.isInstalled).toEqual(false);
                expect(result.isSubscription).toEqual(false);
                done();
            });
        });
    });
});
