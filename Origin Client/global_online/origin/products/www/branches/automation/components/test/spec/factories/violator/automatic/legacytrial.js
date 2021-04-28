/**
 * Jasmine functional test
 */

'use strict';

describe('Automatic Violator: Legacy GameTime violators with termiation dates', function() {
    var legacyTrialViolator, violatorModel, futureDate, pastDate;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            utils: {
                os: function(){}
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
                    getCatalogAndEntitlementData: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_ViolatorLegacyTrialFactory_, _ViolatorModelFactory_) {
        legacyTrialViolator = _ViolatorLegacyTrialFactory_;
        violatorModel = _ViolatorModelFactory_;
    }));

    describe('trialNotActivated', function() {
        it('The game is a non started limited trial', function(done) {
            spyOn(Origin.utils, 'os').and.returnValue('PCWIN');

            spyOn(violatorModel, 'getCatalogAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    gameDistributionSubType: 'Limited Trial'
                }, {
                    terminationDate: null
                }
            ]));

            legacyTrialViolator.trialNotActivated()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('The game is a non started limited trial on mac', function(done) {
            spyOn(Origin.utils, 'os').and.returnValue('MAC');

            spyOn(violatorModel, 'getCatalogAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    gameDistributionSubType: 'Limited Trial'
                }, {
                    terminationDate: null
                }
            ]));

            legacyTrialViolator.trialNotActivated()
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

    describe('trialNotExpired', function() {
        it('The game is a non started limited trial', function(done) {
            spyOn(Origin.utils, 'os').and.returnValue('PCWIN');

            spyOn(violatorModel, 'getCatalogAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    gameDistributionSubType: 'Limited Trial'
                }, {
                    terminationDate: futureDate
                }
            ]));

            legacyTrialViolator.trialNotExpired()
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
        it('The game is a non started limited trial', function(done) {
            spyOn(Origin.utils, 'os').and.returnValue('PCWIN');

            spyOn(violatorModel, 'getCatalogAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    gameDistributionSubType: 'Limited Trial'
                }, {
                    terminationDate: pastDate
                }
            ]));

            legacyTrialViolator.trialExpired()
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