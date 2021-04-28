describe('Origin Search API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.search.searchPeople()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.search.searchPeople('jssdk', 0).then(function(result) {

            expected = {
                "totalCount":2,
                "infoList":[
                    {
                        "friendUserId":"1000123301435"
                    },
                    {
                        "friendUserId":"1000147395502"
                    }
                ]
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it("Origin.search.searchStore()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.search.searchStore('Battlefield', {}).then(function(result) {

            expected_numVisible = 3;
            expected_numFound = 83;
            expected_games_list = [
                {
                    path: '/content/web/app/games/battlefield/battlefield-3/addon/jcr:content',
                    mdmId: '50182',
                    gameId: 'OFB-EAST:55621'
                },
                {
                    path: '/content/web/app/games/battlefield/battlefield-3/addon/jcr:content',
                    mdmId: '50182',
                    gameId: 'OFB-EAST:109541468'
                },
                {
                    path: '/content/web/app/games/battlefield/battlefield-2/standard-edition/jcr:content',
                    mdmId: '68335',
                    gameId: 'OFB-EAST:60888'
                }
            ];

            for (var i = 0; i < expected_games_list.length; i++) {
                expect(result.games.game).toContain(expected_games_list[i]);
            }

            expect(result.games.numVisible).toEqual(expected_numVisible);
            expect(result.games.numFound).toEqual(expected_numFound);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});
