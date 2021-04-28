describe('Origin Search API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.search.searchPeople()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.search.searchPeople('jssdk', 1).then(function(result){
			 
			expected = {
				"totalCount":2,
				"infoList":[{
					"fullName":"gamepackages jssdk",
					"friendsCount":null,
					"eaid":"jssdkgamepackage",
					"avatarLink":"https://stage.download.dm.origin.com/qa/avatar/qa/1/1/40x40.JPEG",
					"friendUserId":"1000123301435",
					"gamesCount":null,
					"lineStyle":null
				},{
					"fullName":"Mockup Jssdk",
					"friendsCount":null,
					"eaid":"jssdkmockupdata",
					"avatarLink":"https://stage.download.dm.origin.com/qa/avatar/qa/1/1/40x40.JPEG",
					"friendUserId":"1000147395502",
					"gamesCount":null,
					"lineStyle":null
				}]
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

        Origin.search.searchStore('Battlefield', {}).then(function(result){
			 
			expected_numVisible = '10';
			expected_numFound = '83';
			expected_games_list = [
				{ 
					path: '/content/origin-store/ww/buy/50182/pc-download/base-game/standard-edition', 
					mdmId: '50182', 
					offerId: 'DR:225064100' 
				},
				{ 
					path: '/content/origin-store/ww/buy/50182/pc-download/addon/battlefield-3-premium', 
					mdmId: '50182', 
					offerId: 'OFB-EAST:50401' 
				},
				{
					path: '/content/origin-store/ww/buy/76889/pc-download/addon/battlefield-4-final-stand', 
					mdmId: '76889', 
					offerId: 'OFB-EAST:109550823' 
				}
			];
			 
			for (var i = 0; i < expected_games_list.length; i++) {
				expect(result.result.games.game).toContain(expected_games_list[i]);
			}
			expect(result.result.games.numVisible).toEqual(expected_numVisible);
			expect(result.result.games.numFound).toEqual(expected_numFound);
			asyncTicker.clear(done);
			 
		}).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    }); 

});
