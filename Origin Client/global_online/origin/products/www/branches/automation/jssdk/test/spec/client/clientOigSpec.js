describe('Origin Client OIG API', function() {

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

    it('Origin.client.oig.openIGOSPA()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.oig.openIGOSPA("foo", "bar").then(function(result) {

            expect(result).toEqual(["foo", "bar"]);
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });
    });

    it('Origin.client.oig.getNonCachedIGOActiveValue()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.oig.getNonCachedIGOActiveValue("foo").then(function(result) {

            expect(result).toEqual("foo");
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });
    });

    it('Origin.client.oig.closeIGO()', function(done) {

        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.oig.closeIGO("foo").then(function(result) {

            expect(result).toEqual("foo");
            asyncTicker.clear(done);

        }).catch(function(error) {

            expect().toFail(error.message);
            asyncTicker.clear(done);

        });
    });

    it ('Origin.client.oig.IGOIsActive()', function() {
        window.OriginIGO.IGOActive = false;
        expect (Origin.client.oig.IGOIsActive()).toBe(false);
        window.OriginIGO.IGOActive = true;
        expect (Origin.client.oig.IGOIsActive()).toBe(true);
    });

    it ('Origin.client.oig.IGOIsAvailable()', function() {
        window.OriginIGO.IGOAvailable = false;
        expect (Origin.client.oig.IGOIsAvailable()).toBe(false);
        window.OriginIGO.IGOAvailable = true;
        expect (Origin.client.oig.IGOIsAvailable()).toBe(true);
    });

});
