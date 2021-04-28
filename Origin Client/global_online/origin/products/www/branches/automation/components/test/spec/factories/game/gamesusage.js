/**
 * Jasmine functional test
 */

'use strict';

describe('Games Usage Factory', function() {
    var gamesUsageFactory, gamesCatalogFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        window.Origin = {
            atom: {
                atomGameLastPlayed: angular.noop
            },
            utils: {
                Communicator: angular.noop
            }
        };

        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: angular.noop
                };
            });

            $provide.factory('GamesCatalogFactory', function(){
                return {
                    getCatalogInfo: angular.noop
                };
            });

            $provide.factory('AppCommFactory', function () {
                return {
                    events: {
                        on: angular.noop,
                        off: angular.noop
                    }
                };
            });

            $provide.factory('GamesClientFactory', function() {
                return {
                    events: {
                        on: angular.noop,
                        fire: angular.noop
                    }
                }
            })
        });
    });

    beforeEach(inject(function(_GamesUsageFactory_, _GamesCatalogFactory_) {
        gamesUsageFactory = _GamesUsageFactory_;
        gamesCatalogFactory = _GamesCatalogFactory_;
    }));

    describe('getLastPlayedTimeByOfferId', function() {
        var testData;

        beforeEach(function() {
            testData = [{
                    masterTitleId: '50182',
                    timestamp: '2014-12-12T19:23:06.133Z'
                }, {
                    // sometimes there are duplicates in the list because of multiplayerIds
                    masterTitleId: '50182',
                    timestamp: '2013-10-03T01:28:55.327Z'
                }, {
                    masterTitleId: '70146',
                    timestamp: '2016-07-12T19:23:06.133Z'
                }];
        });

        it('will return a play time given an offerid', function(done) {
            spyOn(gamesCatalogFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({
                'DR:225064100': {
                    masterTitleId: '50182'
                }
            }));

            spyOn(window.Origin.atom, 'atomGameLastPlayed').and.returnValue(Promise.resolve(testData));

            gamesUsageFactory.getLastPlayedTimeByOfferId('DR:225064100')
                .then(function(data) {
                    expect(data).toEqual(new Date(testData[0].timestamp));
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('will return undefined if the offer can\'t be resolved in catalog', function(done) {
            spyOn(gamesCatalogFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({}));
            spyOn(window.Origin.atom, 'atomGameLastPlayed').and.returnValue(Promise.resolve(testData));

            gamesUsageFactory.getLastPlayedTimeByOfferId('OFB:EAST209012.0202')
                .then(function(data) {
                    expect(data).toBeUndefined();
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('will handle wire errors gracefully on the atom service', function(done) {
            spyOn(gamesCatalogFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({}));
            spyOn(window.Origin.atom, 'atomGameLastPlayed').and.returnValue(Promise.reject());

            gamesUsageFactory.getLastPlayedTimeByOfferId('OFB:EAST-12345')
                .then(function(data) {
                    expect(data).toBeUndefined();
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('return undefined in cases where there\'s no matching game', function(done) {
            spyOn(gamesCatalogFactory, 'getCatalogInfo').and.returnValue(Promise.resolve({
                'OFB-EAST:12345': {
                    masterTitleId: '61233'
                }
            }));

            spyOn(window.Origin.atom, 'atomGameLastPlayed').and.returnValue(Promise.resolve(testData));

            gamesUsageFactory.getLastPlayedTimeByOfferId('OFB-EAST:12345')
                .then(function(data) {
                    expect(data).toBeUndefined();
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('getAllLastPlayedTimes', function() {
        var testData;

        beforeEach(function() {
            testData = [{
                    masterTitleId: '50182',
                    timestamp: '2014-12-12T19:23:06.133Z'
                }];
        });

        it('will return all play times from the atom service', function(done){
            spyOn(window.Origin.atom, 'atomGameLastPlayed').and.returnValue(Promise.resolve(testData));

            gamesUsageFactory.getAllLastPlayedTimes()
                .then(function(data) {
                    expect(data).toEqual(testData);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('will handle wire errors gracefully on the atom service', function(done) {
            spyOn(window.Origin.atom, 'atomGameLastPlayed').and.returnValue(Promise.reject());

            gamesUsageFactory.getAllLastPlayedTimes()
                .then(function(data) {
                    expect(data).toEqual([]);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });
});