/**
 * Jasmine functional test
 */

'use strict';

describe('PIN Recommendation News Factory', function() {
    var PinRecommendationNewsFactory;

    beforeEach(function() {
        window.OriginComponents = {};

        function Communicator() {}
        Communicator.prototype.fire = function() {};

        window.Origin = {
            pin: {
                getNewsRecommendation: function() {},
                constants: {
                    NEWS: 'NEWS'
                }
            },
            log: {
                message: function() {}
            },
            utils: {
                Communicator: Communicator
            },
            events: {
                on: function() {}
            }
        };

        angular.mock.module('origin-components');

        module(function($provide){
            $provide.factory('GamesDataFactory', function() {
                return {};
            });

            $provide.factory('GamesEntitlementFactory', function() {
                return {};
            });

            $provide.factory('GamesCatalogFactory', function() {
                return {};
            });
        }); 
    });

    beforeEach(inject(function(_PinRecommendationNewsFactory_) {
        PinRecommendationNewsFactory = _PinRecommendationNewsFactory_;
    }));

    describe('filterRecommendedNews', function() {
        var numRecs = 4,
            bucketConfig = [{
                position: 1,
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_sims_gameplayvideo'
                }]
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_gamingculture_thinkpiecevideo'
                }]
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_battlefield_gameplaystream'
                }]
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_swbf_tipsvideo'
                }]
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_titanfall_theradbradwalkthroughvideo'
                }]
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_pcculture_graphicscardhardwarearticle'
                }]
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_titanfall_tips'
                }]
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_gamingculture_extracreditsframeratevideo'
                }]
            }],

            bucketConfigCtids = [
                'editorial_sims_gameplayvideo',
                'editorial_gamingculture_thinkpiecevideo',
                'editorial_battlefield_gameplaystream',
                'editorial_swbf_tipsvideo',
                'editorial_titanfall_theradbradwalkthroughvideo',
                'editorial_pcculture_graphicscardhardwarearticle',
                'editorial_titanfall_tips',
                'editorial_gamingculture_extracreditsframeratevideo'
            ],

            recoResponse = {
                govId: '2600956123462160236',
                recommendations: [
                    {
                        name: 'origin news',
                        result: {
                            'item_list': [
                                'editorial_titanfall_theradbradwalkthroughvideo',
                                'editorial_pcculture_graphicscardhardwarearticle',
                                'editorial_titanfall_tips',
                                'editorial_gamingculture_extracreditsframeratevideo'/*,
                                'editorial_pcculture_fanforum',
                                'editorial_battlefield_tipsvideo'*/
                            ]
                        },
                        track: {
                            trackingTag: 'R39Xbd0V2759Ppc'
                        }
                    }
                ]
            },

            bucketConfigReco = [{
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_titanfall_theradbradwalkthroughvideo',
                    recoid: 'editorial_titanfall_theradbradwalkthroughvideo',
                    recotag: recoResponse.recommendations[0].track.trackingTag,
                    recoindex: 0,
                    recotype: 'NEWS'
                }],
                limit: 1,
                diminish: 0,
                priority: 4
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_pcculture_graphicscardhardwarearticle',
                    recoid: 'editorial_pcculture_graphicscardhardwarearticle',
                    recotag: recoResponse.recommendations[0].track.trackingTag,
                    recoindex: 1,
                    recotype: 'NEWS'
                }],
                limit: 1,
                diminish: 0,
                priority: 3
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_titanfall_tips',
                    recoid: 'editorial_titanfall_tips',
                    recotag: recoResponse.recommendations[0].track.trackingTag,
                    recoindex: 2,
                    recotype: 'NEWS'
                }],
                limit: 1,
                diminish: 0,
                priority: 2
            }, {
                usePinRec: true,
                feedData: [{
                    ctid: 'editorial_gamingculture_extracreditsframeratevideo',
                    recoid: 'editorial_gamingculture_extracreditsframeratevideo',
                    recotag: recoResponse.recommendations[0].track.trackingTag,
                    recoindex: 3,
                    recotype: 'NEWS'
                }],
                limit: 1,
                diminish: 0,
                priority: 1
            }];

        it('will return an array of objects representing the recommended news articles', function(done) {

            spyOn(window.Origin.pin, 'getNewsRecommendation').and.returnValue(Promise.resolve(recoResponse));

            PinRecommendationNewsFactory.filterRecommendedNews(bucketConfig, numRecs)
                .then(function(recommendedNewsBucketConfig) {
                    for(var i = 0; i < numRecs; ++i) {
                        expect(window.Origin.pin.getNewsRecommendation).toHaveBeenCalledWith(bucketConfigCtids, numRecs);
                        expect(recommendedNewsBucketConfig[i].feedData[0].ctid).toEqual(bucketConfigReco[i].feedData[0].ctid);
                        expect(recommendedNewsBucketConfig[i].feedData[0].recoid).toEqual(bucketConfigReco[i].feedData[0].recoid);
                        expect(recommendedNewsBucketConfig[i].feedData[0].recotag).toEqual(bucketConfigReco[i].feedData[0].recotag);
                        expect(recommendedNewsBucketConfig[i].feedData[0].recoindex).toEqual(bucketConfigReco[i].feedData[0].recoindex);
                        expect(recommendedNewsBucketConfig[i].feedData[0].recotype).toEqual(bucketConfigReco[i].feedData[0].recotype);
                        expect(recommendedNewsBucketConfig[i].usePinRec).toEqual(bucketConfigReco[i].usePinRec);
                        expect(recommendedNewsBucketConfig[i].limit).toEqual(bucketConfigReco[i].limit);
                        expect(recommendedNewsBucketConfig[i].diminish).toEqual(bucketConfigReco[i].diminish);
                        expect(recommendedNewsBucketConfig[i].priority).toEqual(bucketConfigReco[i].priority);
                    }

                    done();
                });
        });

        it('will return no more than n (=\'numrecs\') recommended news articles', function(done) {

            spyOn(window.Origin.pin, 'getNewsRecommendation').and.returnValue(Promise.resolve(recoResponse));

            PinRecommendationNewsFactory.filterRecommendedNews(bucketConfig, numRecs)
                .then(function(recommendedNewsBucketConfig) {
                    expect(window.Origin.pin.getNewsRecommendation).toHaveBeenCalledWith(bucketConfigCtids, numRecs);
                    expect(recommendedNewsBucketConfig.length).toBeLessThan(numRecs + 1);

                    done();
                });
        });
    });
});