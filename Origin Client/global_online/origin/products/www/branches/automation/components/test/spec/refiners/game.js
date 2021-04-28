/**
 * Jasmine functional test
 */

'use strict';

describe('Game Refiner', function() {
    var gameRefiner, futureDate, pastDate;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');

        futureDate = new Date();
        futureDate.setHours(futureDate.getHours() + 2);

        pastDate = new Date();
        pastDate.setHours(pastDate.getHours() - 2);
    });

    beforeEach(inject(function(_GameRefiner_) {
        gameRefiner = _GameRefiner_;
    }));

    describe('isTrial', function() {
        it('will return true if the game is a trial as mapped in Subtypes', function() {
            var catalogInfo = {
                gameDistributionSubType: 'Weekend Trial'
            };

            expect(gameRefiner.isTrial(catalogInfo)).toBe(true);
        });
    });

    describe('isLimitedTrial', function() {
        it('will return true if the game is a limited trial as mapped in Subtypes', function() {
            var catalogInfo = {
                gameDistributionSubType: 'Limited Trial'
            };

            expect(gameRefiner.isLimitedTrial(catalogInfo)).toBe(true);
        });

        it('will handle unexpected lookups on the insubtype call', function() {
            expect(gameRefiner.isLimitedTrial()).toBe(false);
        });
    });

    describe('isOnTheHouse', function() {
        it('will return true if the game is a On The House - a free game givaway program', function() {
            var catalogInfo = {
                gameDistributionSubType: 'On the House'
            };

            expect(gameRefiner.isOnTheHouse(catalogInfo)).toBe(true);
        });
    });

    describe('isDemo', function() {
        it('will return true if the game is a Demo - for demonstration games', function() {
            var catalogInfo = {
                gameDistributionSubType: 'Demo'
            };

            expect(gameRefiner.isDemo(catalogInfo)).toBe(true);
        });
    });

    describe('isAlpha', function() {
        it('will return true if the game is in Alpha - the game is in active development', function() {
            var catalogInfo = {
                gameDistributionSubType: 'Alpha'
            };

            expect(gameRefiner.isAlpha(catalogInfo)).toBe(true);
        });
    });

    describe('isBeta', function() {
        it('will return true if the game is in Beta - the game is in active development', function() {
            var catalogInfo = {
                gameDistributionSubType: 'Beta'
            };

            expect(gameRefiner.isBeta(catalogInfo)).toBe(true);
        });
    });

    describe('isEarlyAccess', function() {
        it('will return true if the game is an Early Access Trial - Subscribed users get an early trial', function() {
            var catalogInfo = {
                gameDistributionSubType: 'Gameplay Timer Trial - Access'
            };

            expect(gameRefiner.isEarlyAccess(catalogInfo)).toBe(true);
        });
    });

    describe('isPurchasable', function() {
        it('will return true if the game is available for purchase in the store', function() {
            var catalogInfo = {
                isPurchasable: true
            };

            expect(gameRefiner.isPurchasable(catalogInfo)).toBe(true);
        });
    });

    describe('isDownloadable', function() {
        it('will return true if the game is a downloadable game', function() {
            var catalogInfo = {
                downloadable: true
            };

            expect(gameRefiner.isDownloadable(catalogInfo)).toBe(true);
        });
    });

    describe('isFullGame', function() {
        it('will return true if the game is a full game type base game', function() {
            var catalogInfo = {
                originDisplayType: 'Full Game'
            };

            expect(gameRefiner.isFullGame(catalogInfo)).toBe(true);
        });
    });

    describe('isFullGamePlusExpansion', function() {
        it('will return true if the game is a full game that features expansion content as a bundle', function() {
            var catalogInfo = {
                originDisplayType: 'Full Game Plus Expansion'
            };

            expect(gameRefiner.isFullGamePlusExpansion(catalogInfo)).toBe(true);
        });
    });

    describe('isAddon', function() {
        it('will return true if the game is an Addon', function() {
            var catalogInfo = {
                originDisplayType: 'Addon'
            };

            expect(gameRefiner.isAddon(catalogInfo)).toBe(true);
        });
    });

    describe('isExpansion', function() {
        it('will return true if the game is an Expansion', function() {
            var catalogInfo = {
                originDisplayType: 'Expansion'
            };

            expect(gameRefiner.isExpansion(catalogInfo)).toBe(true);
        });
    });

    describe('isBaseGame', function() {
        it('will return true if the game is a full game type base game', function() {
            var catalogInfo = {
                originDisplayType: 'Full Game'
            };

            expect(gameRefiner.isBaseGame(catalogInfo)).toBe(true);
        });
    });

    describe('isHybridGame', function() {
        it('will return true if the game is a hbrid univeral mac pc binary', function() {
            var catalogInfo = {
                platformFacetKey: 'Mac/PC Download'
            };

            expect(gameRefiner.isHybridGame(catalogInfo)).toBe(true);
        });

        it('will return true if the game is a hbrid univeral mac pc binary (legacy games without a platform facet key)', function() {
            var catalogInfo = {
                platforms: {
                    'MAC': {
                        platform: 'MAC'
                    },
                    'PCWIN': {
                        platform: 'PCWIN'
                    }
                }
            };

            expect(gameRefiner.isHybridGame(catalogInfo)).toBe(true);
        });
    });
});
