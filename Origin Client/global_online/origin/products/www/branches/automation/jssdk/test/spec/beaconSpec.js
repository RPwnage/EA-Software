describe('Origin Beacon API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.beacon.installed()', function(done) {

        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expected installed() to be resolved');

        Origin.beacon.installed().then(function(result) {
            expect(result).toEqual('10.0.0.13864', 'Expected installed() to return the version number of the Origin Client');
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

	it('Origin.beacon.installable() - Windows machines', function(done) {

		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'Expected installable() to be resolved');

		Origin.beacon.installable('Mozilla/5.0 (Windows NT 6.7; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.112 Safari/537.36').then(function(result) {
            expect(result).toBe(true);
            asyncTicker.clear(done);
		}).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
	});

    it('Origin.beacon.installable() - Mac OS', function(done) {

        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expected installable() to be resolved');

        Origin.beacon.installable('Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_4) AppleWebKit/601.5.17 (KHTML, like Gecko) Version/9.1 Safari/601.5.17').then(function(result) {
            expect(result).toBe(true);
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.beacon.installable() - Unsupported Platforms (Linux)', function(done) {

        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expected installable() to be resolved');

        Origin.beacon.installable('Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/34.0.1847.137 Safari/4E423F').then(function(result) {
            expect(result).toBe(false);
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.beacon.installableOnPlatform() - PCWIN, PC Client UA', function(done) {
        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expected installable() to be resolved');

        Origin.beacon.installableOnPlatform('PCWIN',  'Mozilla/5.0 (Windows NT 6.7; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.112 Safari/537.36').then(function(result) {
            expect(result).toBe(true);
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.beacon.installableOnPlatform() - MAC, PC Client UA', function(done) {
        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expected installable() to be resolved');

        Origin.beacon.installableOnPlatform('MAC',  'Mozilla/5.0 (Windows NT 6.7; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.112 Safari/537.36').then(function(result) {
            expect(result).toBe(false);
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.beacon.installableOnPlatform() - PCWIN, Mac Client UA', function(done) {
        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expected installable() to be resolved');

        Origin.beacon.installableOnPlatform('PCWIN',  'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_4) AppleWebKit/601.5.17 (KHTML, like Gecko) Version/9.1 Safari/601.5.17').then(function(result) {
            expect(result).toBe(false);
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.beacon.installableOnPlatform() - MAC, Mac Client UA', function(done) {
        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expected installable() to be resolved');

        Origin.beacon.installableOnPlatform('MAC',  'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_4) AppleWebKit/601.5.17 (KHTML, like Gecko) Version/9.1 Safari/601.5.17').then(function(result) {
            expect(result).toBe(true);
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

	it('Origin.beacon.running()', function(done) {

		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'Expected running() to be resolved');

		Origin.beacon.running().then(function(result) {
			expect(result).toEqual(true, 'Expected running() to indicate that the Origin client is running');
			asyncTicker.clear(done);
		}).catch(function(error) {
			expect().toFail(error.message);
			asyncTicker.clear(done);
		});
	});
});