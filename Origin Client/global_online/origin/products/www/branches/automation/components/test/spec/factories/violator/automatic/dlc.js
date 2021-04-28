/**
 * Jasmine functional test
 */

'use strict';

describe('Automatic Violator: Addon and Expansions (DLC)', function() {
    var dlcViolator, violatorModel, futureDate, pastDate;

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
                    getExtraContent: function() {},
                    getClientAndEntitlementData: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_ViolatorDlcFactory_, _ViolatorModelFactory_) {
        dlcViolator = _ViolatorDlcFactory_;
        violatorModel = _ViolatorModelFactory_;
    }));

    describe('newDlcAvailable', function() {
        it('Will determine if a user\'s entitlement has new DLC (Addon) that has become available for up to 7 days', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'downloadable': true,
                'originDisplayType': 'Full Game',
            }));

            spyOn(violatorModel, 'getExtraContent').and.returnValue(Promise.resolve({
                'offerid': {
                    'downloadable': true,
                    'isPurchasable': true,
                    'originDisplayType': 'Addon',
                    'platforms': {
                        'PCWIN': {
                            'releaseDate': pastDate
                        }
                    }
                }
            }));

            dlcViolator.newDlcAvailable('offerid')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will stop advertising new dlc after 7 days', function(done) {
            var sevenDaysAgo = new Date();
            sevenDaysAgo.setHours(sevenDaysAgo.getHours() - 20 - (24 * 7));

            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'downloadable': true,
                'originDisplayType': 'Full Game',
            }));

            spyOn(violatorModel, 'getExtraContent').and.returnValue(Promise.resolve({
                'offerid': {
                    'downloadable': true,
                    'isPurchasable': true,
                    'originDisplayType': 'Addon',
                    'platforms': {
                        'PCWIN': {
                            'releaseDate': sevenDaysAgo
                        }
                    }
                }
            }));

            dlcViolator.newDlcAvailable('offerid')
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

    describe('newDlcExpansionAvailable', function() {
        it('Will determine if a user\'s entitlement has new Expansion that has become available for up to 7 days', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'downloadable': true,
                'originDisplayType': 'Full Game',
            }));

            spyOn(violatorModel, 'getExtraContent').and.returnValue(Promise.resolve({
                'offerid': {
                    'downloadable': true,
                    'isPurchasable': true,
                    'originDisplayType': 'Expansion',
                    'platforms': {
                        'PCWIN': {
                            'releaseDate': pastDate
                        }
                    }
                }
            }));

            dlcViolator.newDlcExpansionAvailable('offerid')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will stop advertising new Expansions after 7 days', function(done) {
            var sevenDaysAgo = new Date();
            sevenDaysAgo.setHours(sevenDaysAgo.getHours() - 20 - (24 * 7));

            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'downloadable': true,
                'originDisplayType': 'Full Game',
            }));

            spyOn(violatorModel, 'getExtraContent').and.returnValue(Promise.resolve({
                'offerid': {
                    'downloadable': true,
                    'isPurchasable': true,
                    'originDisplayType': 'Expansion',
                    'platforms': {
                        'PCWIN': {
                            'releaseDate': sevenDaysAgo
                        }
                    }
                }
            }));

            dlcViolator.newDlcExpansionAvailable('offerid')
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

    describe('newDlcReadyForInstall', function() {
        it('will let users know they have an Addon or expansion outstanding for up to 28 days', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'downloadable': true,
                'originDisplayType': 'Full Game',
            }));

            spyOn(violatorModel, 'getExtraContent').and.returnValue(Promise.resolve({
                'offerid': {
                    'downloadable': true,
                    'originDisplayType': 'Addon',
                }
            }));

            spyOn(violatorModel, 'getClientAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    installable: true
                }, {
                    grantDate: pastDate
                }
            ]));

            dlcViolator.newDlcReadyForInstall('offerid')
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

    describe('newDlcExpansionReadyForInstall', function() {
        it('will let users know they have an Expansion outstanding for up to 28 days', function(done) {
            spyOn(violatorModel, 'getCatalog').and.returnValue(Promise.resolve({
                'downloadable': true,
                'originDisplayType': 'Full Game',
            }));

            spyOn(violatorModel, 'getExtraContent').and.returnValue(Promise.resolve({
                'offerid': {
                    'downloadable': true,
                    'originDisplayType': 'Expansion',
                }
            }));

            spyOn(violatorModel, 'getClientAndEntitlementData').and.returnValue(Promise.resolve([
                {
                    installable: true
                }, {
                    grantDate: pastDate
                }
            ]));

            dlcViolator.newDlcExpansionReadyForInstall('offerid')
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
