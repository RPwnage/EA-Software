describe('Origin Client installDirectory API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.client.installDirectory.browseInstallerCache()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.installDirectory.browseInstallerCache().then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
	it('Origin.client.installDirectory.chooseDownloadInPlaceLocation()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.installDirectory.chooseDownloadInPlaceLocation().then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
	it('Origin.client.installDirectory.chooseInstallerCacheLocation()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.installDirectory.chooseInstallerCacheLocation().then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
	it('Origin.client.installDirectory.deleteInstallers()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.installDirectory.deleteInstallers().then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
	it('Origin.client.installDirectory.resetDownloadInPlaceLocation()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.installDirectory.resetDownloadInPlaceLocation().then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

	it('Origin.client.installDirectory.resetInstallerCacheLocation()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.installDirectory.resetInstallerCacheLocation().then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
	
});