/**
 * Jasmine functional test
 */

'use strict';

describe('Automatic Violator: Platform Specific violators', function() {
    var platformViolator, beaconFactory, violatorModel, futureDate, pastDate, reallyFutureDate;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            utils: {
                os: function() {
                    return 'PCWIN';
                }
            }
        };

        angular.mock.module('origin-components');

        futureDate = new Date();
        futureDate.setHours(futureDate.getHours() + 2);

        reallyFutureDate = new Date();
        reallyFutureDate.setYear(reallyFutureDate.getYear() + 2);

        pastDate = new Date();
        pastDate.setHours(pastDate.getHours() - 2);

        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){}
                };
            });

            $provide.factory('ViolatorModelFactory', function() {
                return {
                    getCatalog: function() {},
                    getClient: function() {},
                    getCatalogAndEntitlementData: function() {},
                    getSubscriptionActive: function() {},
                    getClientOnline: function() {},
                    getEntitlement: function() {}
                };
            });

            $provide.factory('BeaconFactory', function() {
                return {
                    isInstallable: function() {
                        return Promise.resolve(true);
                    },
                    isInstallableOnPlatform: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_ViolatorPlatformFactory_, _ViolatorModelFactory_, _BeaconFactory_) {
        platformViolator = _ViolatorPlatformFactory_;
        violatorModel = _ViolatorModelFactory_;
        beaconFactory = _BeaconFactory_;
    }));

    describe('releasesOn', function() {
        it('Will determine if a user\'s entitlement is in a release date imminent state on their current platform', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'downloadable': true,
                    'originDisplayType': 'Full Game',
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            }));

            platformViolator.releasesOn('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('preloadOn', function() {
        it('Will determine if a user\'s entitlement is in a preload imminent state', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'downloadable': true,
                'originDisplayType': 'Full Game',
                'platforms': {
                    'PCWIN': {
                        'downloadStartDate': futureDate
                    }
                }
            }));

            platformViolator.preloadOn('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('preloadAvailable', function() {
        it('Will determine if a user\'s entitlement is in a preload availablestate', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            }));

            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                installed: false
            }));

            spyOn(violatorModel, 'getEntitlement').and.returnValue(Promise.resolve({
                entitlementTag: 'ORIGIN_DOWNLOAD'
            }));

            platformViolator.preloadAvailable('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('will be suppressed if the user\'s billing has failed', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            }));

            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                installed: false
            }));

            spyOn(violatorModel, 'getEntitlement').and.returnValue(Promise.resolve({
                entitlementTag: 'ORIGIN_PREORDER'
            }));

            platformViolator.preloadAvailable('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });


    describe('justReleased', function() {
        it('Will determine if a user\'s entitlement is released in the last 5 days', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'PCWIN': {
                        'releaseDate': pastDate
                    }
                }
            }));

            platformViolator.justReleased('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
        it('Will determine if a user\'s entitlement is released in the future', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'PCWIN': {
                        'releaseDate': futureDate
                    }
                }
            }));

            platformViolator.justReleased('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('compatibleSinglePlatform', function() {
        it('will show a violator if the game is pc only on a mac client', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platformFacetKey: 'PC Download'
            }));

            spyOn(beaconFactory, 'isInstallableOnPlatform').and.returnValue(Promise.resolve(false));

            platformViolator.compatibleSinglePlatform('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('will show a violator if the game is mac only on a pc client', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platformFacetKey: 'Mac Download'
            }));

            spyOn(beaconFactory, 'isInstallableOnPlatform').and.returnValue(Promise.resolve(false));

            platformViolator.compatibleSinglePlatform('MAC')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will suppress violators if viewing a hybrid game on a PC', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platformFacetKey: 'Mac/PC Download'
            }));

            spyOn(beaconFactory, 'isInstallableOnPlatform').and.returnValue(Promise.resolve(true));

            platformViolator.compatibleSinglePlatform('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('incompatibleHybridPlatform', function() {
        it('Will show violator if viewing a hybrid game on an unsupported platform', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'platformFacetKey': 'Mac/PC Download'
            }));

            spyOn(beaconFactory, 'isInstallable').and.returnValue(Promise.resolve(false));

            platformViolator.incompatibleHybridPlatform()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('expireson', function() {
        it('Will show a violator if the use end date is within 90 days', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'platforms': {
                    'PCWIN': {
                        'useEndDate': futureDate
                    }
                }
            }));

            platformViolator.expiresOn('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('expired', function() {
        it('Will show violator if the game is expired', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'platforms': {
                    'PCWIN': {
                        'useEndDate': pastDate
                    }
                }
            }));

            platformViolator.expired('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('preorderFailed', function() {
        it('Will show a violator for a failed preorder', function(done) {
            spyOn(violatorModel, 'getCatalogAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    'platforms': {
                        'PCWIN': {
                            'releaseDate': pastDate
                        }
                    }
                }, {
                    entitlementTag: 'ORIGIN_PREORDER'
                }
            ]));

            platformViolator.preorderFailed('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });

        });
    });

    describe('preloaded', function() {
        it('Will show a violator when the user has completed installation of their preload', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            }));

            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                installed: true
            }));

            platformViolator.preloaded('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('vaultGameExpiresOn', function() {
        it('Will show a violator when a game is approaching its vault sunset date', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'PCWIN': {
                        'originSubscriptionUseEndDate': futureDate,
                    }
                }
            }));

            spyOn(violatorModel, 'getSubscriptionActive').and.returnValue(Promise.resolve(true));

            spyOn(violatorModel, 'getClientOnline').and.returnValue(Promise.resolve(true));

            spyOn(violatorModel, 'getEntitlement').and.returnValue(Promise.resolve({
                externalType: 'SUBSCRIPTION'
            }));

            platformViolator.vaultGameExpiresOn('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });


    describe('vaultGameExpired', function() {
        it('Will show a violator when a game is approaching its vault sunset date', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platforms: {
                    'PCWIN': {
                        'originSubscriptionUseEndDate': pastDate,
                    }
                }
            }));

            spyOn(violatorModel, 'getSubscriptionActive').and.returnValue(Promise.resolve(true));

            spyOn(violatorModel, 'getClientOnline').and.returnValue(Promise.resolve(true));

            spyOn(violatorModel, 'getEntitlement').and.returnValue(Promise.resolve({
                externalType: 'SUBSCRIPTION'
            }));

            platformViolator.vaultGameExpired('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });
    describe('preAnnouncementDisplayDate', function() {
        it('Will show a violator when the preAnnouncementDisplayDate is set', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                i18n: {
                    preAnnouncementDisplayDate: 'Coming Soon'
                },
                PCWIN: {
                    'releaseDate': reallyFutureDate
                }
            }));

            platformViolator.preAnnouncementDisplayDate('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will handle the NOG schema where the attributes are completely missing', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({}));

            platformViolator.preAnnouncementDisplayDate('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });
    describe('wrongArchitecture', function() {
        it('Will determine if a user\'s entitlement is for the wrong architecture', function(done) {
            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                isWrongArchitecture: true
            }));

            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platformFacetKey: 'PC Download'
            }));

            platformViolator.wrongArchitecture('PCWIN')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });


    describe('wrongArchitecture', function() {
        it('Will determine if wrongArchitecture will not show if wrong platform', function(done) {
            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                isWrongArchitecture: true
            }));

            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                platformFacetKey: 'PC Download'
            }));


            platformViolator.wrongArchitecture('MAC')()
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('hasVaultMacAlternative', function() {
        it('Will show a violator when the user can purchase a viable mac alternative to an OA game', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                isPurchasable: true,
                platformFacetKey: 'Mac Download'
            }));

            spyOn(violatorModel, 'getSubscriptionActive').and.returnValue(Promise.resolve(true));

            spyOn(violatorModel, 'getEntitlement').and.returnValue(Promise.resolve({
                externalType: 'SUBSCRIPTION',
                status: 'ACTIVE'
            }));

            platformViolator.hasVaultMacAlternative('MAC')()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });
});