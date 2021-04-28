/**
 * Jasmine functional test
 */

'use strict';

describe('Automatic Violator: Base Game', function() {
    var baseGameViolator, violatorModel, futureDate, pastDate;

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
                    getCatalogAndEntitlementData: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_ViolatorBaseGameFactory_, _ViolatorModelFactory_) {
        baseGameViolator = _ViolatorBaseGameFactory_;
        violatorModel = _ViolatorModelFactory_;
    }));

    describe('updateAvailable', function() {
        it('Will determine if a user\'s entitlement has outstanding updates', function(done) {
            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                updateAvailable: true
            }));

            baseGameViolator.updateAvailable()
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


    describe('gameExpires', function() {
        it('Will determine if a user\'s entitlement is a game that is expiring', function(done) {
            spyOn(violatorModel, 'getCatalogAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    originDisplayType: 'Full Game'
                }, {
                    terminationDate: futureDate
                }
            ]));

            baseGameViolator.gameExpires()
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

    describe('downloadOverride', function () {
        it('Will show a violator if a user has provided a OverrideDownloadPath in their EACore.ini', function(done) {
            spyOn(violatorModel, 'getClient').and.returnValue(Promise.resolve({
                hasDownloadOverride: true
            }));

            baseGameViolator.downloadOverride()
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
