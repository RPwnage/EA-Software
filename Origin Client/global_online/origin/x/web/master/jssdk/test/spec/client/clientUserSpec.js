describe('Origin Client User API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.client.user.offlineUserInfo()', function(done) {

	    var asyncTicker = new AsyncTicker();
	
		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.user.offlineUserInfo().then(function(result) {
			
			var expected = {
				'nucleusId': 'jssdkfunctionaltest',
				'personaId': 1000101795502,
				'originId': 'jssdkmockupdata',
				'dob': '1980-01-01'
			};
		
            expect(result).toEqual(expected);
			asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
		
    });

    it('Origin.client.user.requestLogout()', function(done) {

		var asyncTicker = new AsyncTicker();
	
		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.user.requestLogout().then(function(result) {
	        expect(result).toEqual({});
			asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

});