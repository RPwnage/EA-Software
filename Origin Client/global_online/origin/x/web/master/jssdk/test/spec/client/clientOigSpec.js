describe('Origin Client OIG API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });


    it('Origin.client.oig.setCreateWindowRequest()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.oig.setCreateWindowRequest('url').then(function(result) {

			expected = {
				'url': 'url'
			};
		
            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
			
            expect().toFail(error.message);
            asyncTicker.clear(done);
			
        });
    });
	
	it('Origin.client.oig.moveWindowToFront()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.oig.moveWindowToFront().then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
			
            expect().toFail(error.message);
            asyncTicker.clear(done);
			
        });
    });
	
	it('Origin.client.oig.openIGOConversation()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.oig.openIGOConversation().then(function(result) {
		
            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
			
            expect().toFail(error.message);
            asyncTicker.clear(done);
			
        });
    });
	
	it('Origin.client.oig.openIGOProfile()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.oig.openIGOProfile().then(function(result) {
		
            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
			
            expect().toFail(error.message);
            asyncTicker.clear(done);
			
        });
    });
	
	
});