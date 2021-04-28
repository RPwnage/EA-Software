describe('Origin Client Dialogs API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.events.CLIENT_DIALOGCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_DIALOGCHANGED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_DIALOGCHANGED, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_DIALOGCHANGED, cb);

        window.OriginDialogs.dialogChanged();

    });

    it('Origin.events.CLIENT_DIALOGCLOSED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_DIALOGCLOSED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_DIALOGCLOSED, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_DIALOGCLOSED, cb);

        window.OriginDialogs.dialogClosed();

    });

    it('Origin.events.CLIENT_DIALOGOPEN', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_DIALOGOPEN should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_DIALOGOPEN, cb);

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_DIALOGOPEN, cb);

        window.OriginDialogs.dialogOpen();

    });

    it('Origin.client.dialogs.finished()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        var expectedFinishedObject = {
            id: '',
            accepted: true,
            content: {}
        };

        Origin.client.dialogs.finished(expectedFinishedObject).then(function(result) {

            expect(result).toEqual(expectedFinishedObject);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.dialogs.linkReact()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        var expectedLinkReactObject = {
            href: ''
        };

        Origin.client.dialogs.linkReact(expectedLinkReactObject).then(function(result) {

            expect(result).toEqual(expectedLinkReactObject);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.dialogs.show()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        var expectedShowObject = {
            context: ''
        };

        Origin.client.dialogs.show(expectedShowObject).then(function(result) {

            expect(result).toEqual(expectedShowObject);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.dialogs.showingDialog()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        var expectedShowingDialogObject = {
            id: ''
        };

        Origin.client.dialogs.showingDialog(expectedShowingDialogObject).then(function(result) {

            expect(result).toEqual(expectedShowingDialogObject);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});