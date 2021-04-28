describe('Origin PIN API', function() {

    it('will get game recommendation', function(done) {
        Origin.pin.getGamesRecommendation(['310270'], 1).then(function(result) {

            var expected = {
                govId: '2600956123462160236',
                recommendations: [
                    {
                        name: 'origin games',
                        result: {
                            'item_list': [{
                                masterTitleId: '55482',
                                projectId: '310270'
                            }]
                        },
                        track: {
                            trackingTag: 'R3aPpc'
                        }
                    }
                ]
            };

            expect(result).toEqual(expected);
            done();

        }).catch(function(error) {
            fail(error);
            done(error);
        });
    });

    it('will get news recommendations', function(done) {
        var numRecs = 4,
            ctids = [
                'editorial_sims_gameplayvideo',
                'editorial_gamingculture_thinkpiecevideo',
                'editorial_battlefield_gameplaystream',
                'editorial_swbf_tipsvideo',
                'editorial_titanfall_theradbradwalkthroughvideo',
                'editorial_pcculture_graphicscardhardwarearticle',
                'editorial_titanfall_tips',
                'editorial_gamingculture_extracreditsframeratevideo'
            ];

        Origin.pin.getNewsRecommendation(ctids, numRecs).then(function(result) {

            var expected = {
                    govId: '2600956123462160236',
                    recommendations: [{
                            name: 'origin news',
                            result: {
                                'item_list': [
                                    'editorial_titanfall_theradbradwalkthroughvideo',
                                    'editorial_pcculture_graphicscardhardwarearticle',
                                    'editorial_titanfall_tips',
                                    'editorial_gamingculture_extracreditsframeratevideo'
                                ]
                            },
                            track: {
                                trackingTag: 'R39Xbd0V2759Ppc'
                            }
                    }]
            };

            expect(result).toEqual(expected);
            done();

        }).catch(function(error) {
            fail(error);
            done(error);
        });
    });

    it('will track recommendation clicks', function(done) {
        var impressionType = "clicks",
            trackingTags = ["trackingTag1", "trackingTag2"],
            trackingIdsAndIndices = [
                {"recoid": "id1", "recoindex": "index1"},
                {"recoid": "id2", "recoindex": "index2"}
            ];

        Origin.pin.trackRecommendations(impressionType, trackingTags, trackingIdsAndIndices).then(function(result) {

            var expected = '';

            expect(result).toEqual(expected);
            done();

        }).catch(function(error) {
            fail(error);
            done(error);
        });
    });

    it('will track recommendation impressions', function(done) {
        var impressionType = "impressions",
            trackingTags = ["trackingTag1", "trackingTag2"],
            trackingIdsAndIndices = [
                {"recoid": "id1", "recoindex": "index1"},
                {"recoid": "id2", "recoindex": "index2"}
            ];

        Origin.pin.trackRecommendations(impressionType, trackingTags, trackingIdsAndIndices).then(function(result) {

            var expected = '';

            expect(result).toEqual(expected);
            done();

        }).catch(function(error) {
            fail(error);
            done(error);
        });
    });

});
