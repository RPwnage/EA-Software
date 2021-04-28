/**
 * Jasmine functional test
 */

'use strict';

describe('Vault Refiner', function() {
    var vaultRefiner, futureDate, pastDate;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');

        futureDate = new Date();
        futureDate.setHours(futureDate.getHours() + 2);

        pastDate = new Date();
        pastDate.setHours(pastDate.getHours() - 2);
    });

    beforeEach(inject(function(_VaultRefiner_) {
        vaultRefiner = _VaultRefiner_;
    }));

    describe('isOfflinePlayExpiryImminent', function() {
        it('will return true if the offlineplay is expiring in the future', function() {
            var clientInfo = {
                offlinePlayExpirationDate: futureDate,
            };

            expect(vaultRefiner.isOfflinePlayExpiryImminent(5)(clientInfo)).toBe(true);
        });

        it('will return false for mangled dates', function() {
            var clientInfo = {};

            expect(vaultRefiner.isOfflinePlayExpiryImminent(5)(clientInfo)).toBe(false);
        });

        it('will return false if outside the date window', function() {
            var outsideDate = new Date();
            outsideDate.setHours(outsideDate.getHours() + 30);

            var clientInfo = {
                offlinePlayExpirationDate: outsideDate,
            };

            expect(vaultRefiner.isOfflinePlayExpiryImminent(1)(clientInfo)).toBe(false);
        });
    });

    describe('isOfflinePlayExpired', function() {
        it('will return true if the offlineplay is expiring in the future', function() {
            var clientInfo = {
                offlinePlayExpirationDate: pastDate,
            };

            expect(vaultRefiner.isOfflinePlayExpired(clientInfo)).toBe(true);
        });

        it('will return false for mangled dates', function() {
            var clientInfo = {};

            expect(vaultRefiner.isOfflinePlayExpired(clientInfo)).toBe(false);
        });
    });

    describe('isTrialExpired', function() {
        it('will return true if the OA trial has reached the time limit', function(){
            var trialTimeInfo = {
                hasTimeLeft: false
            };

            expect(vaultRefiner.isTrialExpired(trialTimeInfo)).toBe(true);
        });

        it('will return true if the OA trial has reached the time limit', function(){
            var trialTimeInfo = {
                hasTimeLeft: true
            };

            expect(vaultRefiner.isTrialExpired(trialTimeInfo)).toBe(false);
        });
    });

    describe('isTrialAwaitingActivation', function() {
        it('will return true if the OA trial is awaiting activation', function(){
            var trialTimeInfo = {
                hasTimeLeft: true,
                leftTrialSec: 80,
                totalTrialSec: 80
            };

            expect(vaultRefiner.isTrialAwaitingActivation(trialTimeInfo)).toBe(true);
        });

        it('will return false if the OA trial is expired', function(){
            var trialTimeInfo = {
                hasTimeLeft: false,
                leftTrialSec: 0,
                totalTrialSec: 80
            };

            expect(vaultRefiner.isTrialAwaitingActivation(trialTimeInfo)).toBe(false);
        });

        it('will return false if the OA trial is in progress', function(){
            var trialTimeInfo = {
                hasTimeLeft: true,
                leftTrialSec: 30,
                totalTrialSec: 80
            };

            expect(vaultRefiner.isTrialAwaitingActivation(trialTimeInfo)).toBe(false);
        });
    });

    describe('isTrialInProgress', function() {
        it('will return true if the OA trial is in progress', function(){
            var trialTimeInfo = {
                hasTimeLeft: true,
                leftTrialSec: 30,
                totalTrialSec: 80
            };

            expect(vaultRefiner.isTrialInProgress(trialTimeInfo)).toBe(true);
        });

        it('will return false id the OA trial has expired', function(){
            var trialTimeInfo = {
                hasTimeLeft: false,
                leftTrialSec: 0,
                totalTrialSec: 80
            };

            expect(vaultRefiner.isTrialInProgress(trialTimeInfo)).toBe(false);
        });

        it('will return false if the OA trial is awaiting activation', function(){
            var trialTimeInfo = {
                hasTimeLeft: true,
                leftTrialSec: 80,
                totalTrialSec: 80
            };

            expect(vaultRefiner.isTrialInProgress(trialTimeInfo)).toBe(false);
        });
    });

    describe('isGameEligibleForUpgrade', function() {
        it('Will return true if the game is in an upgradable state, i.e., is missing vault edition or any available DLCs', function() {
            var isActiveSubscription = true,
                isVaultEditionOwned = false,
                isMissingBasegameVaultEditionOrDLC = true,
                isUpgradeableGame = true;

            expect(vaultRefiner.isGameEligibleForUpgrade(isActiveSubscription, isVaultEditionOwned, isMissingBasegameVaultEditionOrDLC, isUpgradeableGame)).toBe(true);
        });

        it('Will return false if the vault edition and all available DLCs are already owned', function() {
            var isActiveSubscription = true,
                isVaultEditionOwned = true,
                isMissingBasegameVaultEditionOrDLC = false,
                isUpgradeableGame = true;

            expect(vaultRefiner.isGameEligibleForUpgrade(isActiveSubscription, isVaultEditionOwned, isMissingBasegameVaultEditionOrDLC, isUpgradeableGame)).toBe(false);
        });

        it('Will return false if the user\'s subscription is expired', function() {
            var isActiveSubscription = false,
                isVaultEditionOwned = false,
                isMissingBasegameVaultEditionOrDLC = true,
                isUpgradeableGame = true;

            expect(vaultRefiner.isGameEligibleForUpgrade(isActiveSubscription, isVaultEditionOwned, isMissingBasegameVaultEditionOrDLC, isUpgradeableGame)).toBe(false);
        });

        it('Will return false if the game is not upgradeable', function() {
            var isActiveSubscription = true,
                isVaultEditionOwned = false,
                isMissingBasegameVaultEditionOrDLC = true,
                isUpgradeableGame = false;

            expect(vaultRefiner.isGameEligibleForUpgrade(isActiveSubscription, isVaultEditionOwned, isMissingBasegameVaultEditionOrDLC, isUpgradeableGame)).toBe(false);
        });
    });

    describe('isSubscriptionFreeTrial', function() {
        it('Will return true if the game is an origin access trial: OfferTransformer verison', function() {
            var catalogData = {
                ocdPath: '/origin-access/origin-access/game-time/origin-access-7-day-free-trial',
                gameKey: 'origin-access'
            };

            expect(vaultRefiner.isSubscriptionFreeTrial(catalogData)).toBe(true);
        });

        it('Will return false if the game is not an origin access trial: OfferTransformer verison', function() {
            var catalogData = {
                ocdPath: '/battlefield/battlefield-4/standard-edition',
                gameKey: 'battlefield-4'
            };

            expect(vaultRefiner.isSubscriptionFreeTrial(catalogData)).toBe(false);
        });

        it('Will return true if the game is an origin access trial: non OfferTransformer verison', function() {
            var catalogData = {
                offerPath: '/origin-access/origin-access/game-time/origin-access-7-day-free-trial',
                gameNameFacetKey: 'origin-access'
            };

            expect(vaultRefiner.isSubscriptionFreeTrial(catalogData)).toBe(true);
        });

        it('Will return false if the game is not an origin access trial: non OfferTransformer verison', function() {
            var catalogData = {
                offerPath: '/battlefield/battlefield-4/standard-edition',
                gameNameFacetKey: 'battlefield-4'
            };

            expect(vaultRefiner.isSubscriptionFreeTrial(catalogData)).toBe(false);
        });

        it('Will return false if there is no data', function() {
            var catalogData = {};

            expect(vaultRefiner.isSubscriptionFreeTrial(catalogData)).toBe(false);
        });
    });
});