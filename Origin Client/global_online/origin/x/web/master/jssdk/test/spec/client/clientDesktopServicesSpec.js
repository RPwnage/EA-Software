describe('Origin Client DesktopServices API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

	it('Origin.events.CLIENT_DESKTOP_DOCKICONCLICKED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_DESKTOP_DOCKICONCLICKED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_DESKTOP_DOCKICONCLICKED, cb);

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_DESKTOP_DOCKICONCLICKED, cb);

        window.DesktopServices.dockIconClicked(true);

    });
	
    it('Origin.client.desktopServices.asyncOpenUrl()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.desktopServices.asyncOpenUrl('url').then(function(result) {

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
	
	it('Origin.client.desktopServices.flashIcon()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.desktopServices.flashIcon('int').then(function(result) {

			expected = {
				'int': 'int'
			};
		
            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
			
            expect().toFail(error.message);
            asyncTicker.clear(done);
			
        });
    });
	
	it('Origin.client.desktopServices.asyncOpenUrlWithEADPSSO()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.desktopServices.asyncOpenUrlWithEADPSSO('url').then(function(result) {
		
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
	
});