/**
 * Jasmine functional test
 */

'use strict';

describe('Game Violator Factory', function() {
    var gameViolatorFactory,
        stubTrue = function(){
            return Promise.resolve(true);
        },
        stubTrueWithOptions = function() {
            return stubTrue;
        };

    window.store = {};

    window.localStorage = {
        getItem: function() {},
        setItem: function() {}
    };

    beforeEach(function() {
        function Communicator() {}
        Communicator.prototype.fire = function() {};

        window.Origin = {
            log: {
                'message': function(data) {console.log(data);}
            },
            utils: {
                os: function() { return 'PCWIN'; },
                Communicator: Communicator
            }
        };

        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ViolatorBaseGameFactory', function() {
                return {
                    updateAvailable: stubTrue,
                    gameExpires: stubTrue,
                    downloadOverride: stubTrue
                };
            });

            $provide.factory('ViolatorDlcFactory', function() {
                return {
                    newDlcAvailable: stubTrue,
                    newDlcExpansionAvailable: stubTrue,
                    newDlcReadyForInstall: stubTrue,
                    newDlcInstalled: stubTrue,
                };
            });

            $provide.factory('ViolatorLegacyTrialFactory', function() {
                return {
                    trialNotActivated: stubTrue,
                    trialNotExpired: stubTrue,
                    trialExpired: stubTrue
                };
            });

            $provide.factory('ViolatorPlatformFactory', function() {
                return {
                    releasesOn: stubTrueWithOptions,
                    preloadOn: stubTrueWithOptions,
                    preloadAvailable: stubTrueWithOptions,
                    justReleased: stubTrueWithOptions,
                    compatibleSinglePlatform: stubTrueWithOptions,
                    incompatibleHybridPlatform: stubTrue,
                    preAnnouncementDisplayDate: stubTrueWithOptions,
                    expired: stubTrueWithOptions,
                    expiresOn: stubTrueWithOptions,
                    preorderFailed: stubTrueWithOptions,
                    preloaded: stubTrueWithOptions,
                    wrongArchitecture: stubTrueWithOptions,
                    vaultGameExpiresOn: stubTrueWithOptions,
                    vaultGameExpired: stubTrueWithOptions,
                    hasVaultMacAlternative: stubTrueWithOptions,
                    vaultGameUpgrade: stubTrue
                };
            });

            $provide.factory('ViolatorVaultFactory', function() {
                return {
                    offlinePlayExpiring: stubTrue,
                    offlinePlayExpired: stubTrue,
                    isTrialAwaitingActivation: stubTrue,
                    isTrialExpired: stubTrue,
                    isTrialInProgress: stubTrue,
                    subscriptionExpired: stubTrue
                };
            });

            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){},
                    error: function(){}
                };
            });

            $provide.factory('ViolatorProgrammedFactory', function() {
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
                            }
                        ]);
                    }
                };
            });

            $provide.factory('ViolatorModelFactory', function() {
                return {
                    getCatalog: function(offerid) {
                        return {
                            platforms: {
                                'PCWIN': {
                                    'releaseDate': new Date(),
                                    'preloadDate': new Date()
                                }
                            }
                        };
                    },
                    getEntitlement: function(offerId) {
                        return {
                            'terminationDate': new Date()
                        };
                    },
                    getClient: function(offerId) {
                        return {
                            'offlinePlayExpirationDate': new Date()
                        };
                    },
                    getTrialTime: function(offerid) {
                        return {
                            hasTimeLeft: false
                        };
                    }
                };
            });

            $provide.factory('GamesTrialFactory', function() {
                return {
                    getTrialTime: function() {}
                };
            });

            $provide.factory('GamesDataFactory', function() {
                return {
                    getContentId: function() {
                        return Promise.resolve({});
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_GameViolatorFactory_) {
        gameViolatorFactory = _GameViolatorFactory_;
    }));

    describe('Values derived from game data', function() {
        it('Will prioritize critical programmed violators in their natural order', function(done) {
            gameViolatorFactory.getViolators('OFB-EAST:123456', 'gametile')
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

    describe('isWithinViewableDateRange', function() {
        var item;

        beforeEach(function() {
            item = {};
        });

        it('Will be immediately visible if a start time and end time is not provided', function() {
            expect(gameViolatorFactory.isWithinViewableDateRange(item)).toEqual(true);
        });

        it('Will be visible after the campaign begins', function() {
            var now = new Date();
            var campaignStartTime = new Date(now.getTime());
            campaignStartTime.setMinutes(campaignStartTime.getMinutes() - 1);

            item.startdate = campaignStartTime.toISOString();
            expect(gameViolatorFactory.isWithinViewableDateRange(item)).toEqual(true);
        });

        it('Will not be visible before the campaign begins', function() {
            var now = new Date();
            var campaignStartTime = new Date(now.getTime());
            campaignStartTime.setMinutes(campaignStartTime.getMinutes() + 1);

            item.startdate = campaignStartTime.toISOString();
            expect(gameViolatorFactory.isWithinViewableDateRange(item)).toEqual(false);
        });

        it('Will be deactivated when the campaign ends', function() {
            var now = new Date();
            var campaignEndTime = new Date(now.getTime());
            campaignEndTime.setMinutes(campaignEndTime.getMinutes() - 1);

            item.enddate = campaignEndTime.toISOString();
            expect(gameViolatorFactory.isWithinViewableDateRange(item)).toEqual(false);
        });
    });

    describe('dismiss and isDismissed', function() {
        beforeEach(function() {
            spyOn(window.localStorage, 'getItem').and.callFake(function(key) {
                return window.store[key];
            });

            spyOn(window.localStorage, 'setItem').and.callFake(function(key, value) {
                return window.store[key] = value + '';
            });

            spyOn(gameViolatorFactory.events, 'fire').and.callThrough();
        });

        it('Will dismiss programmed violators', function() {
            gameViolatorFactory.dismiss(
                "OFBEAST:12423",
                "critical",
                "This is a title",
                "2015-01-20T20:20:10Z",
                "2015-02-20T20:20:10Z"
            );

            expect(window.localStorage.setItem).toHaveBeenCalled();
            expect(gameViolatorFactory.events.fire).toHaveBeenCalledWith('violator:dismiss:OFBEAST:12423');

            expect(gameViolatorFactory.isDismissed(
                "OFBEAST:12423",
                "critical",
                "This is a title",
                "2015-01-20T20:20:10Z",
                "2015-02-20T20:20:10Z"
            )).toBe(true);

            expect(window.localStorage.getItem).toHaveBeenCalled();
        });

        it('will dismiss automatic violators', function() {
            gameViolatorFactory.dismiss(
                "OFBEAST:12423",
                undefined,
                "This is a title",
                undefined,
                undefined,
                "justreleased"
            );

            expect(window.localStorage.setItem).toHaveBeenCalled();
            expect(gameViolatorFactory.events.fire).toHaveBeenCalledWith('violator:dismiss:OFBEAST:12423');

            expect(gameViolatorFactory.isDismissed(
                "OFBEAST:12423",
                undefined,
                "This is a title",
                undefined,
                undefined,
                "justreleased"
            )).toBe(true);

            expect(window.localStorage.getItem).toHaveBeenCalled();
        });
    });

    describe('enddateReached', function() {
        beforeEach(function() {
            spyOn(gameViolatorFactory.events, 'fire').and.callThrough();
        });

        it('will allow the UI to fire an event when the enddate\'s live countdown is reached', function() {
            gameViolatorFactory.enddateReached('OFBEAST:12423');

            expect(gameViolatorFactory.events.fire).toHaveBeenCalledWith('violator:enddateReached:OFBEAST:12423');
        });
    });

    describe('isValidViolator', function() {
        it('will return true given a valid violator list', function() {
            var data = [
                {
                    'violatortype': 'programmed',
                    'priority': 'critical',
                    'offerid': 'OFB-EAST:1234',
                    'enddate': new Date(),
                    'timeremaining': 30,
                    'timerformat': 'limited'
                }
            ];

            expect(gameViolatorFactory.isValidViolatorData(data)).toBe(true);
        });

        it('will return false if the data is malformed', function() {
            var data;

            expect(gameViolatorFactory.isValidViolatorData(data)).toBe(false);
        });
    });
});