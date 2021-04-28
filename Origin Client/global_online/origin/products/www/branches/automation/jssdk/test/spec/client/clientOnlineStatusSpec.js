describe('Origin Client OnlineStatus API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.client.onlineStatus.goOnline(), Origin.events.CLIENT_ONLINESTATECHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, cb);

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_ONLINESTATECHANGED should be triggered');

        Origin.client.onlineStatus.goOnline().then(function(result) {
            expect(result).toEqual(undefined);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.onlineStatus.isOnline()', function() {

        expect(Origin.client.onlineStatus.isOnline()).toEqual(true);

    });

    it('Origin.events.CLIENT_CLICKEDOFFLINEMODEBUTTON', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_CLICKEDOFFLINEMODEBUTTON, cb);

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_CLICKEDOFFLINEMODEBUTTON, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_CLICKEDOFFLINEMODEBUTTON should be triggered');

        window.OriginOnlineStatus.offlineModeBtnClicked(true);
    });


});
