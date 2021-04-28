describe('Origin Friend API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.friends.friendRecommendations()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.friends.friendRecommendations().then(function(result) {

            expected_id = 1000147395502;
            expected_recommendation_list = [{
                "id":1000123301435,
                "rs":[{
                    "r":1,
                    "rp":"12306939085,1000120382991",
                    "w":0.8
                },
                {
                    "r":2,
                    "rp":"2",
                    "w":0.2
                }],
                "s":1
            },
            {
                "id":12277919725,
                "rs":[{
                    "r":1,
                    "rp":"12306939085",
                    "w":0.8
                },
                {
                    "r":2,
                    "rp":"1",
                    "w":0.2
                }],
                "s":0
            },
            {
                "id":1000129332895,
                "rs":[{
                    "r":1,
                    "rp":"12306939085",
                    "w":0.8
                },
                {
                    "r":2,
                    "rp":"1",
                    "w":0.2
                }],
                "s":0
            }];

            for (var i = 0; i < expected_recommendation_list.length; i++) {
                expect(result.rfs).toContain(expected_recommendation_list[i]);
            }

            expect(result._id).toEqual(expected_id);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});
