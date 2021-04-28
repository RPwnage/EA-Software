/**
 * Jasmine functional test
 */

'use strict';

describe('Automatic Violator: Vault and OA Trial Game information', function() {
    var vaultViolator, violatorModel, futureDate, pastDate;

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
                    getTrialTime: function() {},
                    getSubscriptionActive: function() {},
                    getEntitlement: function() {},
                    isVaultEditionOwned: function() {},
                    isMissingBasegameVaultEditionOrDLC: function() {},
                    isUpgradeableVaultGame: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_ViolatorVaultFactory_, _ViolatorModelFactory_) {
        vaultViolator = _ViolatorVaultFactory_;
        violatorModel = _ViolatorModelFactory_;
    }));

    describe('offlinePlayExpiring', function() {
        it('will return true if the value user has been offline up to 5 days before the expiry date', function(done) {
            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                offlinePlayExpirationDate: futureDate
            }));

            vaultViolator.offlinePlayExpiring()
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

    describe('offlinePlayExpired', function() {
        it('will return true if the value user has been offline too long', function(done) {
            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                offlinePlayExpirationDate: pastDate
            }));

            vaultViolator.offlinePlayExpired()
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

    describe('trialNotActivated', function() {
        it('will return true if the user has not started their trial', function(done) {
            spyOn(violatorModel, 'getTrialTime').and.returnValue(Promise.resolve({
                hasTimeLeft: true,
                leftTrialSec: 80,
                totalTrialSec: 80
            }));

            vaultViolator.trialNotActivated()
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

    describe('trialNotExpired', function() {
        it('will return true if trial is in progress', function(done) {
            spyOn(violatorModel, 'getTrialTime').and.returnValue(Promise.resolve({
                hasTimeLeft: true,
                leftTrialSec: 30,
                totalTrialSec: 80
            }));

            vaultViolator.trialNotExpired()
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

    describe('trialExpired', function() {
        it('will return true if trial is expired', function(done) {
            spyOn(violatorModel, 'getTrialTime').and.returnValue(Promise.resolve({
                hasTimeLeft: false,
                leftTrialSec: 0,
                totalTrialSec: 80
            }));

            vaultViolator.trialExpired()
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

   describe('subscriptionExpired', function() {
        it('will return true if the entitlement if this entitlement\'s subscribtion plan has lapsed', function(done) {
            spyOn(violatorModel, 'getEntitlement').and.returnValue(Promise.resolve({
                externalType: 'SUBSCRIPTION'
            }));

            spyOn(violatorModel, 'getSubscriptionActive').and.returnValue(Promise.resolve(false));

            vaultViolator.subscriptionExpired()
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

   describe('vaultGameUpgrade', function() {
        it('will return true if the game is eligible for an upgrade to the vault edition', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                vaultInfo: {
                    isUpgradeable: 'Y'
                }
            }));

            spyOn(violatorModel, 'getEntitlement').and.returnValue(Promise.resolve({
                status: 'ACTIVE'
            }));

            spyOn(violatorModel, 'getSubscriptionActive').and.returnValue(Promise.resolve(true));

            spyOn(violatorModel, 'isVaultEditionOwned').and.returnValue(Promise.resolve(false));

            spyOn(violatorModel, 'isMissingBasegameVaultEditionOrDLC').and.returnValue(Promise.resolve(true));

            spyOn(violatorModel, 'isUpgradeableVaultGame').and.returnValue(Promise.resolve(true));

            vaultViolator.vaultGameUpgrade()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
   })
});