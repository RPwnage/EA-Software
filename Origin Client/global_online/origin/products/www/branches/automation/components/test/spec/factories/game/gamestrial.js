/**
 * Jasmine functional test
 */

'use strict';

describe('Games Trial Factory', function() {
    var gamesTrialFactory,
        gamesEntitlementFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        window.Origin = {
            trial: {
                getTime: function(contentId) {
                    return Promise.resolve({
                        hasTimeLeft: false,
                        leftTrialSec:0,
                        totalTrialSec:0,
                        totalGrantedSec:0
                    }); 
                }
            },
            utils: {
                os: function() {
                    return 'PCWIN';
                }
            }
        };

        module(function($provide) {
            $provide.factory('GamesCatalogFactory', function(){
                return {
                    getCatalogInfo: function(offerId) {
                        return Promise.resolve({
                                'Mr. Offer Id': {
                                    gameDistributionSubType: 'Limited Trial',
                                    contentId: '12345',
                                    terminationDate: null
                            }
                        });
                    }
                };
            });
            $provide.factory('AuthFactory', function(){
                return {
                    isClientOnline: function() {
                        return true;
                    },
                    isAppLoggedIn: function () {
                        return true;
                    }
                };
            });
            $provide.factory('GamesEntitlementFactory', function(){
                return {
                    getEntitlement: function(offerId) {
                        return Promise.resolve({
                                terminationDate: new Date('2012-01-01')
                        });
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_GamesTrialFactory_,_GamesEntitlementFactory_) {
        gamesTrialFactory = _GamesTrialFactory_;
        gamesEntitlementFactory = _GamesEntitlementFactory_;
    }));

    describe('trialIsExpired', function() {
        it('Will be true if vault trial is expired', function(done) {
            spyOn(gamesEntitlementFactory,'getEntitlement').and.returnValue(Promise.resolve({terminationDate:new Date('2040-01-01')}));
            gamesTrialFactory.isTrialExpired('Mr. Offer Id')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
        it('Will be true if legacy trial is expired', function(done) {
            spyOn(window.Origin.trial,'getTime').and.returnValue(Promise.resolve({hasTimeLeft: true}));
            gamesTrialFactory.isTrialExpired('Mr. Offer Id')
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
    describe('trialIsAwaitingOnlineActivation', function() {
        it('Will be true if vault trial is awaiting activation', function(done) {
            spyOn(window.Origin.trial,'getTime').and.returnValue(Promise.resolve({hasTimeLeft:true,leftTrialSec:5,totalTrialSec:5}));
            gamesTrialFactory.isTrialAwaitingActivation('Mr. Offer Id')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
        it('Will be true if legacy trial is awaiting activation', function(done) {
            spyOn(gamesEntitlementFactory,'getEntitlement').and.returnValue(Promise.resolve({terminationDate: null}));
            gamesTrialFactory.isTrialAwaitingActivation('Mr. Offer Id')
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
    describe('trialIsInProgress', function() {
        it('Will be true if vault trial is in progress', function(done) {
            spyOn(window.Origin.trial,'getTime').and.returnValue(Promise.resolve({hasTimeLeft:true,leftTrialSec:5,totalTrialSec:10}));
            gamesTrialFactory.isTrialInProgress('Mr. Offer Id')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
        it('Will be true if legacy trial is in progress', function(done) {
            spyOn(gamesEntitlementFactory,'getEntitlement').and.returnValue(Promise.resolve({terminationDate: new Date('2040-01-01')}));
            gamesTrialFactory.isTrialInProgress('Mr. Offer Id')
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