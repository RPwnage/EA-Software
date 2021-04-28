/**
 * Jasmine functional test
 */

'use strict';

describe('Platform Refiner', function() {
    var platformRefiner, futureDate, pastDate;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');

        futureDate = new Date();
        futureDate.setHours(futureDate.getHours() + 2);

        pastDate = new Date();
        pastDate.setHours(pastDate.getHours() - 2);
    });

    beforeEach(inject(function(_PlatformRefiner_) {
        platformRefiner = _PlatformRefiner_;
    }));

    describe('hasMatchingPlatformReleaseDates', function() {
        it('Check if two release dates are the same', function() {
            var matchingDate = _.clone(futureDate),
                catalogInfo = {
                'platforms' : {
                    'PCWIN': {
                        releaseDate: matchingDate
                    },
                    'MAC': {
                        releaseDate: matchingDate
                    },
                    'MAC/PCWIN': {
                        releaseDate: matchingDate
                    },
                    'Browser': {
                        releaseDate: pastDate
                    }
                }
            };

            expect(platformRefiner.hasMatchingPlatformReleaseDates(['PCWIN', 'MAC', 'MAC/PCWIN'])(catalogInfo)).toBe(true);
            expect(platformRefiner.hasMatchingPlatformReleaseDates(['PCWIN', 'Browser'])(catalogInfo)).toBe(false);
        });

        it('If the catalog dates are malformed, handle the edge case', function() {
            var catalogInfo = {
                'PCWIN': {
                    releaseDate: undefined
                },
                'MAC': {
                }
            };

            expect(platformRefiner.hasMatchingPlatformReleaseDates(['PCWIN', 'MAC'])(catalogInfo)).toBe(false);
            expect(platformRefiner.hasMatchingPlatformReleaseDates(['MAC'])(catalogInfo)).toBe(false);
        });
    });

    describe('isReleaseDateImminent', function() {
        it('Will ensure the release date is in the future and applicable to the specified platform', function() {
            var catalogInfo = {
                'platforms' : {
                    'PCWIN': {
                        releaseDate: futureDate
                    }
                }
            };

            expect(platformRefiner.isReleaseDateImminentInDays('PCWIN', 7)(catalogInfo)).toBe(true);
            expect(platformRefiner.isReleaseDateImminentInDays('MAC', 7)(catalogInfo)).toBe(false);
        });
    });

    describe('isRecentlyReleasedInDays', function() {
        it('Will return false if the release date is in the future', function() {
            var catalogInfo = {
                platforms:  {
                    'PCWIN':  {
                        'releaseDate': futureDate
                    }
                }
            };

            expect(platformRefiner.isRecentlyReleasedInDays('PCWIN', 7)(catalogInfo)).toBe(false);
        });

        it('Will return true if a catalog item has been released in the specified time', function() {
            var catalogInfo = {
                platforms:  {
                    'PCWIN':  {
                        'releaseDate': pastDate
                    }
                }
            };

            expect(platformRefiner.isRecentlyReleasedInDays('PCWIN', 7)(catalogInfo)).toBe(true);
        });

        it('Will return false if the recently released threshold date is passed', function() {
            var sevenDaysAgo = new Date();
            sevenDaysAgo.setHours(sevenDaysAgo.getHours() - 20 - (24 * 7));

            var catalogInfo = {
                platforms:  {
                    'PCWIN':  {
                        'releaseDate': sevenDaysAgo
                    }
                }
            };

            expect(platformRefiner.isRecentlyReleasedInDays('PCWIN', 7)(catalogInfo)).toBe(false);
        });
    });

    describe('isReleased', function() {
        it('Will return false if the release date is in the future', function() {
            var catalogInfo = {
                platforms:  {
                    'PCWIN':  {
                        'releaseDate': futureDate
                    }
                }
            };

            expect(platformRefiner.isReleased('PCWIN')(catalogInfo)).toBe(false);
        });

        it('Will return true if the release date is in the past', function() {
            var catalogInfo = {
                platforms:  {
                    'PCWIN':  {
                        'releaseDate': pastDate
                    }
                }
            };

            expect(platformRefiner.isReleased('PCWIN')(catalogInfo)).toBe(true);
        });
    });

    describe('isPreloadImminentInDays', function() {
        it('Will return true if a base game preload event is upcoming', function() {
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': futureDate
                    }
                },
                downloadable: true,
                originDisplayType: 'Full Game'
            };

            expect(platformRefiner.isPreloadImminentInDays('PCWIN', 7)(catalogInfo)).toBe(true);
            expect(platformRefiner.isPreloadImminentInDays('MAC', 7)(catalogInfo)).toBe(false);
        });
    });

    describe('isPreloadable', function() {
        it('Will return true if the catalog data is in a preloadable state, the entitlement is not ORIGIN_PREORDER, and the game is not yet installed', function() {
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            };

            var clientInfo = {
                installed: false
            };

            expect(platformRefiner.isPreloadable('PCWIN')(catalogInfo, clientInfo, false)).toBe(true);
            expect(platformRefiner.isPreloadable('PCWIN')(catalogInfo, clientInfo, true)).toBe(false);
            expect(platformRefiner.isPreloadable('MAC')(catalogInfo, clientInfo, false)).toBe(false);
        });

        it('Will return false if the catalog data is in a preloadable state and the game is installed', function() {
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            };

            var clientInfo = {
                installed: true
            };

            expect(platformRefiner.isPreloadable('PCWIN')(catalogInfo, clientInfo)).toBe(false);
            expect(platformRefiner.isPreloadable('MAC')(catalogInfo, clientInfo)).toBe(false);
        });

        it('Will also work in an external browser when the client state is unavailable', function() {
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            };

            var clientInfo = {
                downloading: false
            };

            expect(platformRefiner.isPreloadable('PCWIN')(catalogInfo, clientInfo)).toBe(true);
        });
    });

    describe('isAvailableOnPlatform', function() {
        var macOnlyGame = {
                platformFacetKey: 'Mac Download'
            },
            pcOnlyGame = {
                platformFacetKey: 'PC Download'
            },
            hybridGame = {
                platformFacetKey: 'Mac/PC Download'
            },
            legacyPcGame = {
                platforms: {
                    'PCWIN': {
                        platform: 'PCWIN'
                    }
                }
            },
            legacyMacGame = {
                platforms: {
                    'MAC': {
                        platform: 'MAC'
                    }
                }
            },
            legacyBothPlatforms = {
                platforms: {
                    'MAC': {
                        platform: 'MAC'
                    },
                    'PCWIN': {
                        platform: 'PCWIN'
                    }
                }
            };

        it('will return true if it\'s a mac only game on a mac local platform', function() {
            expect(platformRefiner.isAvailableOnPlatform('MAC')(macOnlyGame)).toBe(true);
            expect(platformRefiner.isAvailableOnPlatform('PCWIN')(macOnlyGame)).toBe(false);
        });

        it('will return true if it\'s a pc only game on a pc local platform', function() {
            expect(platformRefiner.isAvailableOnPlatform('PCWIN')(pcOnlyGame)).toBe(true);
            expect(platformRefiner.isAvailableOnPlatform('MAC')(pcOnlyGame)).toBe(false);
        });

        it('will return true if it\'s a hybrid game on a either platform', function() {
            expect(platformRefiner.isAvailableOnPlatform('PCWIN')(hybridGame)).toBe(true);
            expect(platformRefiner.isAvailableOnPlatform('MAC')(hybridGame)).toBe(true);
        });

        it('will fall back to checking platform base attributes if the platform facet key is not defined (legacy Mac game)', function(){
            expect(platformRefiner.isAvailableOnPlatform('MAC')(legacyMacGame)).toBe(true);
            expect(platformRefiner.isAvailableOnPlatform('PCWIN')(legacyMacGame)).toBe(false);
        });

        it('will fall back to checking platform base attributes if the platform facet key is not defined (legacy PC game)', function(){
            expect(platformRefiner.isAvailableOnPlatform('PCWIN')(legacyPcGame)).toBe(true);
            expect(platformRefiner.isAvailableOnPlatform('MAC')(legacyPcGame)).toBe(false);
        });

        it('will fall back to checking platform base attributes if the platform facet key is not defined (available on both platforms)', function() {
            expect(platformRefiner.isAvailableOnPlatform('PCWIN')(legacyBothPlatforms)).toBe(true);
            expect(platformRefiner.isAvailableOnPlatform('MAC')(legacyBothPlatforms)).toBe(true);
        });
    });

    describe('isExpiryImminentInDays', function() {
        it('will return true if the game is coming up to its use end date', function() {
            var sunsetGame = {
                platforms: {
                    'PCWIN': {
                        'useEndDate': futureDate
                    }
                }
            };

            expect(platformRefiner.isExpiryImminentInDays('PCWIN', 90)(sunsetGame)).toBe(true);
        });
    });


    describe('isExpired', function() {
        it('will return true if the game has passed its use by date', function() {
            var expiredgame = {
                platforms: {
                    'PCWIN': {
                        'useEndDate': pastDate
                    }
                }
            };

            expect(platformRefiner.isExpired('PCWIN')(expiredgame)).toBe(true);
        });
    });

    describe('isPreloaded', function() {
        it('Will return false if the catalog data is in a preloadable state and the user has not yet installed the game', function() {
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            };

            var clientInfo = {
                installed: false
            };

            expect(platformRefiner.isPreloaded('PCWIN')(catalogInfo, clientInfo)).toBe(false);
            expect(platformRefiner.isPreloaded('MAC')(catalogInfo, clientInfo)).toBe(false);
        });

        it('Will return true if the catalog data is in a preloadable state and the user has installed the game', function() {
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'downloadStartDate': pastDate,
                        'releaseDate': futureDate
                    }
                }
            };

            var clientInfo = {
                installed: true
            };

            expect(platformRefiner.isPreloaded('PCWIN')(catalogInfo, clientInfo)).toBe(true);
            expect(platformRefiner.isPreloaded('MAC')(catalogInfo, clientInfo)).toBe(false);
        });
    });

    describe('isVaultGameExpiryImminentInDays', function(){
        it('will return true if a game\'s subscription use end date is approaching within the specified number of days', function(){
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'originSubscriptionUseEndDate': futureDate
                    }
                }
            };

            expect(platformRefiner.isVaultGameExpiryImminentInDays('PCWIN', 7)(catalogInfo)).toBe(true);
            expect(platformRefiner.isVaultGameExpiryImminentInDays('MAC', 7)(catalogInfo)).toBe(false);
        });
    });

    describe('isVaultGameExpired', function(){
        it('will return true if a game\'s subscription use end date is in the past', function(){
            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'originSubscriptionUseEndDate': pastDate
                    }
                }
            };

            expect(platformRefiner.isVaultGameExpired('PCWIN')(catalogInfo)).toBe(true);
            expect(platformRefiner.isVaultGameExpired('MAC')(catalogInfo)).toBe(false);
        });
    });

    describe('hasVaultMacAlternative', function() {
        it('will return true if a game has a mac alternative game for subscribers to purchase', function(){
            var catalogInfo = {
                    isPurchasable: true,
                    platformFacetKey: 'Mac Download'
                },
                isInSubscriptionAndActive = true,
                isSubscriptionActive = true;

            expect(platformRefiner.hasVaultMacAlternative('PCWIN')(catalogInfo, isInSubscriptionAndActive, isSubscriptionActive)).toBe(false);
            expect(platformRefiner.hasVaultMacAlternative('MAC')(catalogInfo, isInSubscriptionAndActive, isSubscriptionActive)).toBe(true);
        });
    });
});