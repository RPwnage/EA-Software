/**
 * Jasmine functional test
 */

'use strict';

describe('feed models Factory', function () {

    var FeedModelsFactory;

    beforeEach(function () {
        function Communicator() {
        }

        Communicator.prototype.fire = function () {
        };

        window.Origin = {
            'utils': {
                'Communicator': Communicator
            },
            'client': {
                games: {
                    isGamePlaying: function () {
                    }
                }
            },
            'locale': {
                locale: function () {
                    return 'en_US';
                }
            },
            'events': {
                'on': function () {
                }
            }
        };

        module('origin-components');

        module(function ($provide) {
            $provide.factory('ComponentsLogFactory', function () {
                return {
                    error: function () {
                    },
                    log: function () {
                    }
                };
            });

            $provide.factory('GamesEntitlementFactory', function () {
                return {
                    ownsEntitlement: function () {
                    },
                    isSubscriptionEntitlement: function () {
                    },
                    events: {
                        'on': function () {
                        }
                    }
                };
            });
            $provide.factory('AuthFactory', function () {
                return {
                    waitForAuthReady: function () {
                        return Promise.resolve({});
                    },
                    events: {
                        'on': function () {
                        }
                    }
                };
            });
            $provide.factory('GamesDataFactory', function () {
                return {
                    getCatalogInfo: function () {
                    },
                    events: {
                        'on': function () {
                        }
                    }
                };
            });
            $provide.factory('StoreAgeGateFactory', function() {
                return {};
            });
        });
    });

    beforeEach(inject(function (_FeedModelsFactory_) {
        FeedModelsFactory = _FeedModelsFactory_;
    }));


    describe('method getFriendRecommendations' ,function () {
        it('return empty array when server error happens', function (done) {
            Origin.friends = {
                friendRecommendations: function () {
                    return Promise.reject('server error');
                }
            };
            var friendRecommendations = FeedModelsFactory.getFriendRecommendations();

            friendRecommendations.then(function (result) {
                expect(result.length).toBe(0);
                done();
            });
        });

        it('should return one friend recommendation', function (done) {
            Origin.friends = {
                friendRecommendations: function () {
                    return Promise.resolve({
                        recs: [
                            {
                                id: 12305670577,
                                wt: 0.98,
                                reasons: {
                                    mf: ['1000124347818', '1000124347819', 'Anonymous'],
                                    sg: ['1ecb527c-43ed-4874-9d45-40428b7960b0'],
                                    'play_history': [
                                        {
                                            'game': 'Battelfield1',
                                            'no_of_matches': 5,
                                            'most_recent_ts': 836429387489
                                        },
                                        {
                                            'game': 'FIFA17',
                                            'no_of_matches': 2,
                                            'most_recent_ts': 836429792874

                                        }]
                                }
                            }]
                    });
                }
            };
            var friendRecommendations = FeedModelsFactory.getFriendRecommendations();

            friendRecommendations.then(function (result) {
                expect(result.length).toBe(1);
                expect(result[0].userid).toBe(12305670577);
                expect(result[0].friendsInCommonStr).toEqual(['1000124347818', '1000124347819', 'Anonymous2']);
                done();
            });
        });


        it('should return one friend recommendation and ignore common play friend recommendation', function (done) {
            Origin.friends = {
                friendRecommendations: function () {
                    return Promise.resolve({
                        recs: [
                            {
                                id: 12305670577,
                                wt: 0.98,
                                reasons: {
                                    mf: ['Anonymous','1000124347818', '1000124347819'],
                                    sg: ['1ecb527c-43ed-4874-9d45-40428b7960b0'],
                                    'play_history': [
                                        {
                                            'game': 'Battelfield1',
                                            'no_of_matches': 5,
                                            'most_recent_ts': 836429387489
                                        },
                                        {
                                            'game': 'FIFA17',
                                            'no_of_matches': 2,
                                            'most_recent_ts': 836429792874

                                        }]
                                }
                            },
                            {
                                id: '12289446480',
                                wt: 0.45,
                                reasons: {
                                    mf: [],
                                    sg: ['1ecb527c-43ed-4874-9d45-40428b7960b0'],
                                    'play_history': [
                                        {
                                            'game': 'Battelfield1',
                                            'no_of_matches': 5,
                                            'most_recent_ts': 836429387489

                                        }]
                                }
                            }]
                    });
                }
            };
            var friendRecommendations = FeedModelsFactory.getFriendRecommendations();

            friendRecommendations.then(function (result) {
                expect(result.length).toEqual(1);
                expect(result[0].userid).toEqual(12305670577);
                expect(result[0].friendsInCommonStr).toEqual(['1000124347818', '1000124347819', 'Anonymous2']);
                done();
            });
        });
    });
});
