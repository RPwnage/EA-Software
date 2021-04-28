describe('Origin Atom API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.atom.atomUserInfoByUserIds()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.atom.atomUserInfoByUserIds('1000120382991,12306939085').then(function(result) {

            var expected = [{
                userId : '1000120382991',
                personaId : '1000060182991',
                EAID : 'ithasbeenthere'
            }, {
                userId : '12306939085',
                personaId : '408187599',
                EAID : 'itsnotthere',
                firstName : null,
                lastName : null
            }];

            for (var i = 0; i < expected.length; i++) {
                expect(result).toContain(expected[i]);
            }

            expect(result.length).toEqual(expected.length, 'returned result should have ' + expected.length + ' entries');
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.atom.atomGameUsage()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.atom.atomGameUsage('182555', '71654').then(function(result) {

            var expected = {
                gameId : '182555',
                total : '0',
                MultiplayerId : '71654',
                lastSession : '0',
                lastSessionEndTimeStamp : '0'
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
    
    it('Origin.atom.atomReportUser()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.atom.atomReportUser('1000120382991', 'In Game', 'Cheating', 'Test Comment').then(function(result) {

            var expected = {
                reportUser : {
                    contentType : 'Cheating',
                    reportReason : 'In Game',
                    comments : 'Test Comment'
                }
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
	it('Origin.atom.atomFriendsForUser()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
		
        Origin.atom.atomFriendsForUser('1000120382991').then(function(result){
			
			var expected = [{
					userId: '1000123301435',
					personaId: '1000100101435',
					EAID: 'jssdkgamepackage',
					firstName: 'gamepackages',
					lastName: 'jssdk'
				}, {
					userId: '1000147395502',
					personaId: '1000101795502',
					EAID: 'jssdkmockupdata',
					firstName: 'Mockup',
					lastName: 'Jssdk'
				}, {
					userId: '12306939085',
					personaId: '408187599',
					EAID: 'itsnotthere',
					firstName:  null,
					lastName: null
			}];
			
			expect(result).toEqual(expected);
			asyncTicker.clear(done);
		
		}).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
	it('Origin.atom.atomGameLastPlayed()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
		
        Origin.atom.atomGameLastPlayed().then(function(result){
			
			var expected =  [{ masterTitleId: '74342', timestamp: '2015-03-18T20:19:18.282Z' }];
			
			expect(result).toEqual(expected);
			asyncTicker.clear(done);
		
		}).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
	it('Origin.atom.atomGamesOwnedForUser()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
		
        Origin.atom.atomGamesOwnedForUser('1000120382991').then(function(result){
			
			var expected =  [{ 
				productId: 'OriginQABuilds.OFR.0008', 
				displayProductName: 'Origin QA Test Build DiP 3.0 Battlefield Flow', 
				cdnAssetRoot: 'http://static.cdn.ea.com/ebisu/u/f/products/1023728', 
				imageServer: 'https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod', 
				softwareList: { 
					software: { 
						softwarePlatform: 'PCWIN', 
						achievementSetOverride: '50823_180977_50340_71314' 
					} 
				}
			}];
			
			expect(result).toEqual(expected);
			asyncTicker.clear(done);
		
		}).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});
