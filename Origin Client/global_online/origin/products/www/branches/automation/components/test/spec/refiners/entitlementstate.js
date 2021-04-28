/**
 * Jasmine functional test
 */

'use strict';

describe('Entitlement State Refiner', function() {
    var entitlementStateRefiner, futureDate, pastDate;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        window.Origin = {
            utils: {
                os: function(){return 'PCWIN';}
            }
        };

        futureDate = new Date();
        futureDate.setHours(futureDate.getHours() + 2);

        pastDate = new Date();
        pastDate.setHours(pastDate.getHours() - 2);
    });

    beforeEach(inject(function(_EntitlementStateRefiner_) {
        entitlementStateRefiner = _EntitlementStateRefiner_;
    }));

    describe('isUpdateAvailable', function() {
        it('Will return true if the client is in an update available state', function() {
            var clientInfo = {
                updateAvailable: true
            };

            expect(entitlementStateRefiner.isUpdateAvailable(clientInfo)).toBe(true);
        });
    });

    describe('isWrongArchitecture', function() {
        it('Will return true if the client says we are not on a supported architecture', function() {
            var clientInfo = {
                isWrongArchitecture: true
            };

            expect(entitlementStateRefiner.isWrongArchitecture(clientInfo)).toBe(true);
        });
    });

    describe('isBaseGameExpiryImminentInDays', function() {
        it('Will return true if the user is approaching a termination date for a base game in their collection', function() {
            var catalogInfo = {
                originDisplayType: 'Full Game'
            };

            var entitlementInfo = {
                terminationDate: futureDate
            };

            expect(entitlementStateRefiner.isBaseGameExpiryImminentInDays(7)(catalogInfo, entitlementInfo)).toBe(true);
        });
    });

    describe('isAddonOrExpansionAwatingDownloadInDays', function() {
        it('Will return true if an addon or expansion is ready to download', function() {
            var clientInfo = {
                installable: true
            };

            var entitlementInfo = {
                grantDate: pastDate
            };

            expect(entitlementStateRefiner.isAddonOrExpansionAwatingDownloadInDays(28)(clientInfo, entitlementInfo)).toBe(true);
        });
    });

    describe('isAddonOrExpansionRecentlyInstalledInDays', function(){
        it('Will return true if an addon or expansion is recently installed', function() {
            var clientInfo = {
                installed: true,
                initialInstalledTimestamp: futureDate
            };

            var baseGameLastPlayed = new Date();
            baseGameLastPlayed.setHours(baseGameLastPlayed.getHours() - 20);

            expect(entitlementStateRefiner.isAddonOrExpansionRecentlyInstalledInDays(28)(clientInfo, baseGameLastPlayed)).toBe(true);
        });
    });

    describe('isPreorder', function() {
        it('Will return true if the entitlement is a preorder', function() {
            var entitlementInfo = {
                entitlementTag: 'ORIGIN_PREORDER'
            };

            expect(entitlementStateRefiner.isPreorder(entitlementInfo)).toBe(true);
        });
    });

    describe('isFailedPreorder', function() {
        it('will return true if the user\'s preorder is still outstanding after the street release date', function(){
            var catalogInfo = {
                    platforms: {
                        'PCWIN': {
                            releaseDate: pastDate
                        }
                    }
                },
                entitlementInfo = {
                    entitlementTag: 'ORIGIN_PREORDER'
                };

            expect(entitlementStateRefiner.isFailedPreorder('PCWIN')(catalogInfo, entitlementInfo)).toBe(true);
        });

        it('Normal preorders will have no entitlementTag after release', function(){
            var catalogInfo = {
                    platforms: {
                        'PCWIN': {
                            releaseDate: pastDate
                        }
                    }
                },
                entitlementInfo = {
                    entitlementTag: 'ORIGIN_DOWNLOAD'
                };

            expect(entitlementStateRefiner.isFailedPreorder('PCWIN')(catalogInfo, entitlementInfo)).toBe(false);
        });
    });

    describe('isInSubscription', function() {
        it('will return true if the entitlement is a subscription', function() {
            var entitlementInfo = {
                externalType: 'SUBSCRIPTION'
            };

            expect(entitlementStateRefiner.isInSubscription(entitlementInfo)).toBe(true);
        });
    });

    describe('isExpired', function() {
        it('will be true if the game has expired', function() {
            var entitlementInfoPast = {
                terminationDate: pastDate
            };
            var entitlementInfoFuture = {
                terminationDate: futureDate
            };

            expect(entitlementStateRefiner.isExpired(entitlementInfoPast)).toBe(true);
            expect(entitlementStateRefiner.isExpired(entitlementInfoFuture)).toBe(false);
        });

        it('will handle bad values for date', function(){
            var entitlementInfoNull = {
                terminationDate: null
            };
            var entitlementInfoUndefined = {
            };

            expect(entitlementStateRefiner.isExpired(entitlementInfoNull)).toBe(false);
            expect(entitlementStateRefiner.isExpired(entitlementInfoUndefined)).toBe(false);
        });
    });

    describe('isActive', function() {
        it('will return true if the game is active', function() {
            var entitlmentInfo = {
                status: 'ACTIVE'
            };

            expect(entitlementStateRefiner.isActive(entitlmentInfo)).toBe(true);
        });
    });

    describe('isDisabled - this function is primarily used to enable and disable game library tiles it is also core to the ogd cta buttons', function(){
        var catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription;

        beforeEach(function(){
            catalogInfo = {};
            entitlementInfo = {};
            clientInfo = {};
            isValidPlatform = false;
            isActiveSubscription = false;
        });

        it('will be disabled if the entitlement is expired', function() {
            entitlementInfo = {
                terminationDate: pastDate
            };

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });


        it('will be disabled if the entitlement is part of subscriptions but the user is not an active subscriber', function() {
            isActiveSubscription = false;

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the entitlement is part of subscriptions, the user is a subscriber and the originSubscriptionUseEndDate is in the past', function() {
            catalogInfo = {
                platforms: {
                    'PCWIN': {
                        'originSubscriptionUseEndDate': pastDate
                    }
                }
            };

            entitlementInfo = {
                externalType: 'SUBSCRIPTION'
            };

            isActiveSubscription = true;

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the entitlement is a preorder but not yet preloadable - EADP has not granted the entitlement in time to preload with everyone else', function() {
            catalogInfo = {
                'platforms' : {
                    'PCWIN': {
                        releaseDate: futureDate
                    }
                }
            };

            entitlementInfo = {
                status: 'ACTIVE',
                terminationDate: pastDate,
                externalType: 'SUBSCRIPTION',
                entitlementTag: 'ORIGIN_PREORDER'
            };

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the game is being retired and past its use end date', function() {
            catalogInfo = {
                'platforms' : {
                    'PCWIN': {
                        useEndDate: pastDate
                    }
                }
            };

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the entitlement is inactive', function(){
            entitlementInfo = {
                status: 'INACTIVE'
            };

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the client says it\'s not playable', function() {
            entitlementInfo = {
                status: 'ACTIVE'
            };
            clientInfo = {
                playable: false
            };

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the client says it\'s not downloadable', function() {
            entitlementInfo = {
                status: 'ACTIVE'
            };
            clientInfo = {
                playable: true,
                downloadable: false
            };

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the client says it\'s not installable', function() {
            entitlementInfo = {
                status: 'ACTIVE'
            };
            clientInfo = {
                playable: true,
                downloadable: true,
                installable: false
            };

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be disabled if the entitlement is not available on the current platform', function() {
            catalogInfo = {
                'platforms' : {
                    'MAC': {
                        releaseDate: pastDate,
                        platform: 'MAC'
                    }
                }
            };

            entitlementInfo = {
                status: 'ACTIVE',
            };

            clientInfo = {
                playable: true,
                downloadable: true,
                installable: true
            };

            isValidPlatform = true;

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(true);
        });

        it('will be enabled for happy cases', function() {
            catalogInfo = {
                'platforms' : {
                    'PCWIN': {
                        releaseDate: pastDate,
                        platform: 'PCWIN'
                    }
                }
            };

            entitlementInfo = {
                status: 'ACTIVE',
            };

            clientInfo = {
                playable: true
            };

            isValidPlatform = true;

            expect(entitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription))
                .toBe(false);
        });
    });

    describe('isDownloadOverride', function(){
        it('Will return true if the game has OverrideDownloadPath::Origin.OFR.50.0000823=file://C:\\foo.zip override set in EACore.ini under the QA tab', function() {
            var clientInfo = {
                hasDownloadOverride: true
            };

            expect(entitlementStateRefiner.isDownloadOverride(clientInfo)).toBe(true);
        });
    });

    describe('isVaultGameExpired', function() {
        it('Will return true if the user has a vault entitlement, but isn\'t in the subscription program any longer', function(){
            var entitlementInfo = {
                externalType: 'SUBSCRIPTION'
            };
            var isActiveSubscription = false;

            expect(entitlementStateRefiner.isVaultGameExpired(entitlementInfo, isActiveSubscription)).toBe(true);
        });

        it('Will return false if the user is a valid subscriber', function(){
            var entitlementInfo = {
                externalType: 'SUBSCRIPTION'
            };
            var isActiveSubscription = true;

            expect(entitlementStateRefiner.isVaultGameExpired(entitlementInfo, isActiveSubscription)).toBe(false);
        });

        it('Will return false for normal base games', function(){
            var entitlementInfo = {
            };
            var isActiveSubscription = false;

            expect(entitlementStateRefiner.isVaultGameExpired(entitlementInfo, isActiveSubscription)).toBe(false);
        });
    });

    describe('isOTHGranted - determines if an entitlement was granted under the "On The House" Program', function() {
        it('will return true if the game is currently on the house entitlement - note that the atributes are avalable on the entitlement endpoint', function() {
            var entitlementInfo = {
                entitlementSource: 'Ebisu-Platform',
                gameDistributionSubType: 'On the House'
            };

            expect(entitlementStateRefiner.isOTHGranted(entitlementInfo)).toBe(true);
        });

        it('will return true if the game is a normal game and free eg. (\'entitlementSource\') === \'Ebisu-Platform\' - note that the atributes are avalable on the entitlement endpoint', function(){
            var entitlementInfo = {
                entitlementSource: 'Ebisu-Platform',
                gameDistributionSubType: 'Normal Game'
            };

            expect(entitlementStateRefiner.isOTHGranted(entitlementInfo)).toBe(true);
        });

        it('will return false for a normal game with a different entitlementSource', function() {
            var entitlementInfo = {
                entitlementSource: 'ORIGIN-STORE-CLIENT-CA',
                gameDistributionSubType: 'Normal Game'
            };

            expect(entitlementStateRefiner.isOTHGranted(entitlementInfo)).toBe(false);
        });

        it('will behave with malformed input', function() {
            var entitlementInfo;

            expect(entitlementStateRefiner.isOTHGranted(entitlementInfo)).toBe(false);
        });
    });

    describe('isGameActionable - adds some overrides to the base game disabled filter for use with the OGD Call to actions', function() {
        it('Will always show a CTA for non origin games', function() {
            expect(entitlementStateRefiner.isGameActionable(undefined, undefined, undefined, false, false, false, true, false)).toBe(true);
        });

        it('Will always show a CTA for a game that can be uninstalled', function() {
            var clientInfo = {
                uninstallable: true
            };

            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        releaseDate: pastDate
                    }
                }
            };

            expect(entitlementStateRefiner.isGameActionable(catalogInfo, undefined, clientInfo, false, false, false, false, false)).toBe(true);
        });

        it('Will always show a CTA for a game that is being actively uninstalled', function() {
            var clientInfo = {
                uninstalling: true
            };

            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        releaseDate: pastDate
                    }
                }
            };

            expect(entitlementStateRefiner.isGameActionable(catalogInfo, undefined, clientInfo, false, false, false, false, false)).toBe(true);
        });

        it('Will always show a CTA for expired trials - We want to redirect the user to an offer in the store', function() {
            var catalogInfo = {
                gameDistributionSubType: 'Weekend Trial',
                platforms: {
                    'PCWIN': {
                        releaseDate: pastDate
                    }
                }
            };

            expect(entitlementStateRefiner.isGameActionable(catalogInfo, undefined, undefined, false, false, true, false, false)).toBe(true);
        });

        it('Will never show a CTA for preordered games before their preload date', function() {
            var entitlementInfo = {
                entitlementTag: 'ORIGIN_PREORDER'
            };

            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        releaseDate: futureDate,
                        downloadStartDate: futureDate
                    }
                }
            };

            expect(entitlementStateRefiner.isGameActionable(catalogInfo, entitlementInfo, undefined, false, false, false, false, false)).toBe(false);
        });

        it('will never show a CTA for preload games that have already been successfully installed', function() {
            var entitlementInfo = {
                entitlementTag: 'ORIGIN_DOWNLOAD'
            };

            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        releaseDate: futureDate,
                        downloadStartDate: pastDate
                    }
                }
            };

            var clientInfo = {
                installed: true
            };

            expect(entitlementStateRefiner.isGameActionable(catalogInfo, entitlementInfo, clientInfo, false, false, false, false, false)).toBe(false);
        });

        it('will show a CTA for preordered games that have are marked as "private" 1102/1103 offers', function() {
            var entitlementInfo = {
                status: 'ACTIVE'
            };

            var catalogInfo = {
                platforms: {
                    'PCWIN': {
                        releaseDate: futureDate,
                        platform: 'PCWIN'                        
                    }
                }
            };

            expect(entitlementStateRefiner.isGameActionable(catalogInfo, entitlementInfo, undefined, true, false, false, false, true)).toBe(true);
        });

    });

});
