describe('Origin Client Games API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.events.CLIENT_GAMES_CHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_GAMES_CHANGED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_GAMES_CHANGED should be triggered');

        window.OriginGamesManager.changed([]);

    });

    it('Origin.events.CLIENT_GAMES_DOWNLOADQUEUECHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_GAMES_DOWNLOADQUEUECHANGED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_GAMES_DOWNLOADQUEUECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_GAMES_DOWNLOADQUEUECHANGED should be triggered');

        window.OriginGamesManager.downloadQueueChanged([]);

    });

    it('Origin.events.CLIENT_GAMES_BASEGAMESUPDATED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_GAMES_BASEGAMESUPDATED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_GAMES_BASEGAMESUPDATED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_GAMES_BASEGAMESUPDATED should be triggered');

        window.OriginGamesManager.basegamesupdated([]);

    });

    it('Origin.events.CLIENT_GAMES_OPERATIONFAILED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_GAMES_OPERATIONFAILED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_GAMES_OPERATIONFAILED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_GAMES_OPERATIONFAILED should be triggered');

        window.OriginGamesManager.operationFailed([]);

    });

    it('Origin.events.CLIENT_GAMES_PROGRESSCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();
        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_GAMES_PROGRESSCHANGED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_GAMES_PROGRESSCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_GAMES_PROGRESSCHANGED should be triggered');


        window.OriginGamesManager.progressChanged([]);

    });

    it('Origin.client.games.requestGamesStatus()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.requestGamesStatus('').then(function(result) {

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.startDownload()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.startDownload('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelDownload()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelDownload('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseDownload()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseDownload('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeDownload()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeDownload('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.checkForUpdateAndInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.checkForUpdateAndInstall('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.installUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.installUpdate('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseUpdate('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeUpdate('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelUpdate('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.repair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.repair('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelRepair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelRepair('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseRepair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseRepair('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeRepair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeRepair('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelCloudSync()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelCloudSync('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pollCurrentCloudUsage()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pollCurrentCloudUsage('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.restoreLastCloudBackup()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.restoreLastCloudBackup('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.removeFromLibrary()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.removeFromLibrary('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });


    it('Origin.client.games.play()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.play('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.isGamePlaying()', function() {

        expect(Origin.client.games.isGamePlaying()).toEqual(false);

    });

    it('Origin.client.games.install()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.install('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelInstall('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseInstall('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeInstall('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.customizeBoxArt()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.customizeBoxArt('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.uninstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.uninstall('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.restore()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.restore('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.moveToTopOfQueue()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.moveToTopOfQueue('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.installParent()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.installParent('').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});

