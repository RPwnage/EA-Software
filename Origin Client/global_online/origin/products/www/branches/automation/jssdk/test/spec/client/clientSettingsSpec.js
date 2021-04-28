describe('Origin Client Settings API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.client.settings.hotkeyConflictSwap()', function(done) {

        var expected = {
            'Ctrl-S': 'Save Game'
        };

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.hotkeyConflictSwap().then(function(result) {

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });




    });

    it('Origin.client.settings.supportedLanguagesData()', function() {

        var expected = {
            'en-US': 'US English',
            'en-CA': 'Canadian English',
            'fr-CA': 'Canadian Frnech'
        };

        expect(Origin.client.settings.supportedLanguagesData()).toEqual(expected);

    });

    it('Origin.client.settings.hotkeyInputHasFocus()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.hotkeyInputHasFocus('true').then(function(result) {

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.settings.pinHotkeyInputHasFocus()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.pinHotkeyInputHasFocus('true').then(function(result) {

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.settings.readSetting()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.readSetting('true').then(function(result) {

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.settings.startLocalHostResponder()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.startLocalHostResponder('true').then(function(result) {

            expect(result).toEqual("Host Responder Started");
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.settings.startLocalHostResponderFromOptOut()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.startLocalHostResponderFromOptOut('true').then(function(result) {

            expect(result).toEqual("Host Responder Started From Opt Out");
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.settings.stopLocalHostResponder()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.stopLocalHostResponder('true').then(function(result) {

            expect(result).toEqual("Host Responder Stopped");
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.settings.writeSetting()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.settings.writeSetting('Save Game', 'Ctrl-S').then(function(result) {

            expect(result).toEqual("Setting Changed");
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.events.CLIENT_SETTINGS_UPDATESETTINGS', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SETTINGS_UPDATESETTINGS should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, cb);

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, cb);

        window.OriginClientSettings.updateSettings(false);

    });

    it('Origin.events.CLIENT_SETTINGS_RETURN_FROM_DIALOG', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SETTINGS_RETURN_FROM_DIALOG, cb);

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SETTINGS_RETURN_FROM_DIALOG, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SETTINGS_RETURN_FROM_DIALOG should be triggered');

        window.OriginClientSettings.returnFromSettingsDialog(false);

    });

    it('Origin.events.CLIENT_SETTINGS_ERROR', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SETTINGS_ERROR, cb);

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SETTINGS_ERROR, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SETTINGS_ERROR should be triggered');

        window.OriginClientSettings.settingsError(false);

    });

});
