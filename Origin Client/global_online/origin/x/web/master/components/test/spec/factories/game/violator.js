/**
 * Jasmine functional test
 */

'use strict';

describe('Game Violator Factory', function() {

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('GameAutomaticViolatorTestsFactory', function() {
                var stubTrue = function(){return Promise.resolve(true);};

                return {
                    preloadOn: stubTrue,
                    preloadAvailable: stubTrue,
                    releasesOn: stubTrue,
                    updateAvailable: stubTrue,
                    gameExpires: stubTrue,
                    trialNotActivated: stubTrue,
                    trialNotExpired: stubTrue,
                    trialExpired: stubTrue,
                    newDlcAvailable: stubTrue,
                    newDlcExpansionAvailable: stubTrue,
                    newDlcReadyForInstall: stubTrue,
                    newDlcInstalled: stubTrue
                };
            });

            $provide.factory('GameProgrammedViolatorFactory', function() {
                return {
                    getContent: function(){
                        return Promise.resolve([
                            {
                                'title': 'a critical priority programmed message',
                                'priority': 'critical'
                            }, {
                                'title': 'another critical priority programmed message',
                                'priority': 'critical'
                            }, {
                                'title': 'a high priority programmed message',
                                'priority': 'high'
                            }, {
                                'title': 'a normal priority programmed message',
                                'priority': 'normal'
                            }, {
                                'title': 'a low priority programmed message',
                                'priority': 'low'
                            },
                        ]);
                    }
                };
            });

            $provide.factory('GameViolatorModelFactory', function() {
                return {
                    getCatalog: function(offerid) {
                        return {
                            'releaseDate': new Date(),
                            'preloadDate': new Date()
                        };
                    },
                    getEntitlement: function(offerId) {
                        return {
                            'terminationDate': new Date()
                        };
                    }
                };

            });
        });
    });

    describe('Values derived from game data', function() {
        var GameViolatorFactory;

        beforeEach(inject(function(_GameViolatorFactory_) {
            GameViolatorFactory = _GameViolatorFactory_;
        }));

        it('Will prioritize critical programmed violators in their natural order', function(done) {
            GameViolatorFactory.getViolators('OFB-EAST:123456', 'gametile')
            .then(function(data) {
                expect(data.length > 6).toBeTruthy();
                var lastItem = data.pop();
                expect(data[0].title).toBe('a critical priority programmed message');
                expect(data[1].title).toBe('another critical priority programmed message');
                expect(lastItem.title).toBe('a low priority programmed message');
                done();
            })
            .catch(function(err){
                fail(err);
                done();
            });
        });


    });
});