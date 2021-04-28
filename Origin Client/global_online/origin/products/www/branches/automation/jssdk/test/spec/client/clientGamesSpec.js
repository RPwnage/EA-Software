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

        Origin.client.games.requestGamesStatus().then(function(result) {

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

        Origin.client.games.startDownload('1234').then(function(result) {

            expect(result).toEqual(['1234', 'startDownload', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelDownload()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelDownload('1234').then(function(result) {

            expect(result).toEqual(['1234', 'cancelDownload', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseDownload()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseDownload('1234').then(function(result) {

            expect(result).toEqual(['1234', 'pauseDownload', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeDownload()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeDownload('1234').then(function(result) {

            expect(result).toEqual(['1234', 'resumeDownload', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.checkForUpdateAndInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.checkForUpdateAndInstall('1234').then(function(result) {

            expect(result).toEqual(['1234', 'checkForUpdateAndInstall', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.installUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.installUpdate('1234').then(function(result) {

            expect(result).toEqual(['1234', 'installUpdate', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseUpdate('1234').then(function(result) {

            expect(result).toEqual(['1234', 'pauseUpdate', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeUpdate('1234').then(function(result) {

            expect(result).toEqual(['1234', 'resumeUpdate', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelUpdate()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelUpdate('1234').then(function(result) {

            expect(result).toEqual(['1234', 'cancelUpdate', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.repair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.repair('1234').then(function(result) {

            expect(result).toEqual(['1234', 'repair', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelRepair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelRepair('1234').then(function(result) {

            expect(result).toEqual(['1234', 'cancelRepair', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseRepair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseRepair('1234').then(function(result) {

            expect(result).toEqual(['1234', 'pauseRepair', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeRepair()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeRepair('1234').then(function(result) {

            expect(result).toEqual(['1234', 'resumeRepair', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelCloudSync()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelCloudSync('1234').then(function(result) {

            expect(result).toEqual(['1234', 'cancelCloudSync', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pollCurrentCloudUsage()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pollCurrentCloudUsage('1234').then(function(result) {

            expect(result).toEqual(['1234', 'pollCurrentCloudUsage', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.restoreLastCloudBackup()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.restoreLastCloudBackup('1234').then(function(result) {

            expect(result).toEqual(['1234', 'restoreLastCloudBackup', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.removeFromLibrary()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.removeFromLibrary('1234').then(function(result) {

            expect(result).toEqual(['1234', 'removeFromLibrary', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });


    it('Origin.client.games.play()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.play('1234').then(function(result) {

            expect(result).toEqual(['1234', 'play', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.isGamePlaying()', function(done) {


        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.isGamePlaying().then(function(result) {

            expect(result).toEqual(false);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });



    });

    it('Origin.client.games.install()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.install('1234').then(function(result) {

            expect(result).toEqual(['1234', 'install', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.cancelInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.cancelInstall('1234').then(function(result) {

            expect(result).toEqual(['1234', 'cancelInstall', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.pauseInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.pauseInstall('1234').then(function(result) {

            expect(result).toEqual(['1234', 'pauseInstall', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.resumeInstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.resumeInstall('1234').then(function(result) {

            expect(result).toEqual(['1234', 'resumeInstall', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.customizeBoxArt()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.customizeBoxArt('1234').then(function(result) {

            expect(result).toEqual(['1234', 'customizeBoxArt', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.uninstall()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.uninstall('1234').then(function(result) {
            expect(result).toEqual(['1234', 'uninstall', { silent: false }]);
            asyncTicker.clear(done);


        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.restore()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.restore('1234').then(function(result) {

            expect(result).toEqual(['1234', 'restore', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.moveToTopOfQueue()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.moveToTopOfQueue('1234').then(function(result) {

            expect(result).toEqual(['1234', 'moveToTopOfQueue', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.installParent()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.installParent('1234').then(function(result) {

            expect(result).toEqual(['1234', 'installParent', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.getMultiLaunchOptions()', function(done) {

        var asyncTicker = new AsyncTicker();
        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.getMultiLaunchOptions('1234').then(function(result) {

            expect(result).toEqual(['1234', 'getMultiLaunchOptions', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.getAvailableLocales()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.getAvailableLocales('1234').then(function(result) {

            expect(result).toEqual(['1234', 'getAvailableLocales', {}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.games.updateInstalledLocale()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.games.updateInstalledLocale('1234', 'en_US').then(function(result) {

            expect(result).toEqual(['1234', 'updateInstalledLocale', { newLocale: 'en_US' }]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
});

