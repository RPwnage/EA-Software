/**
 * Jasmine functional test
 */

'use strict';

describe('Programmed Introduction Factory', function() {
    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            var previousDate = new Date();
            previousDate.setHours(previousDate.getHours() - 24)

            $provide.factory('GamesDataFactory', function() {
                return {
                    baseGameEntitlements: function() {
                        return [
                            {
                                'offerId': 'OFB-EAST:50500',
                                'grantDate': previousDate
                            }, {
                                'offerId': 'OFB-EAST:10606',
                                'grantDate': previousDate
                            }
                        ];
                    },
                    getOcdByOfferId: function(offerId) {
                        switch (offerId) {
                            case 'OFB-EAST:50500':
                                return Promise.resolve({
                                    'gamehub': {
                                        'components': {
                                            "origin-home-discovery-tile-introduction-config": {
                                                "title": "Because You Played Battlefield 4",
                                                "showfordays": 3,
                                                "items": [{
                                                    "origin-home-discovery-tile-config-programmable": {
                                                      "image": "/content/dam/originx/web/app/home/discovery/programmed/tile_bf4_beginners_long.jpg",
                                                      "description": "<b>BF4 Guide & Tips for Beginners<\/b><br>Battlefield 4 'The Basics' Guide and tips for BF4 beginners.",
                                                      "placementtype": "priority",
                                                      "placementvalue": "1",
                                                      "items": [{
                                                          "origin-home-discovery-tile-config-cta": {
                                                            "actionid": "url-external",
                                                            "href": "http://www.google.com",
                                                            "description": "Read More",
                                                            "type": "primary"
                                                          }
                                                        }
                                                      ]
                                                    }
                                                  },{
                                                    "origin-home-discovery-tile-config-programmable": {
                                                      "image": "/content/dam/originx/tile_bf4_teamplayer_long.jpg",
                                                      "description": "<b>Guide Being A BF4 Team Player<\/b><br>New to BF4? This guide will show you how to be a better team player.",
                                                      "placementtype": "position",
                                                      "placementvalue": "2",
                                                      "items": [{
                                                          "origin-home-discovery-tile-config-cta": {
                                                            "actionid": "play-video",
                                                            "href": "https://www.youtube.com/watch?v=O2XE0wzzP-k",
                                                            "description": "Check it out",
                                                            "type": "primary"
                                                          }
                                                        }
                                                      ]
                                                    }
                                                  }
                                                ]
                                            }
                                        }
                                    }
                                });

                            case 'OFB-EAST:10606':
                                return Promise.resolve({});
                        }
                    }
                };
            });
        });
    });

    var programmedIntroductionFactory;

    beforeEach(inject(function(_ProgrammedIntroductionFactory_) {
            programmedIntroductionFactory = _ProgrammedIntroductionFactory_;
    }));

    describe('getTileData', function() {
        it('Gets tile data for a user\'s base game entitlements', function(done) {
            programmedIntroductionFactory.getTileData()
                .then(function(data) {
                    expect(data.length).toBe(1);
                    expect(data[0].showForDays).toBe(3);
                    expect(data[0].title).toBe('Because You Played Battlefield 4');
                    expect(data[0].items.length).toBe(2);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });
});