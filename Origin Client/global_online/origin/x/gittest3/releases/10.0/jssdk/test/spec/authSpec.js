describe('Origin Auth API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.auth.login()', function(done) {

        var asyncTicker = new AsyncTicker();

        var authSuccess = function() {
            Origin.events.off(Origin.events.AUTH_SUCCESSRETRY, authSuccess);
            Origin.events.off(Origin.events.AUTH_FAILEDCREDENTIAL, authFail);

            expect(Origin.auth.isLoggedIn()).toBeTruthy('Origin.auth.isLoggedIn() should return true');
            expect(Origin.user.accessToken()).toEqual('QVQwOjEuMDozLjA6MTk6UUVGd042cDZsakgwVmc4aVZlWkVjTGd0MG5XQWtVcmkwM1U6OTU1MDI6bWxsYzE', 'Origin.user.accessToken() should return correct token');
            expect(Origin.user.originId()).toEqual('jssdkmockupdata', 'Origin.user.originId() should return correct originId');
            expect(Origin.user.personaId()).toEqual(1000101795502, 'Origin.user.personaId() should return correct personaId');
            expect(Origin.user.userPid()).toEqual('jssdkfunctionaltest', 'Origin.user.userPid() should return correct userPid');
            expect(Origin.user.underAge()).toBeFalsy('Origin.user.underAge() should return false');
            expect(Origin.user.subscription()).toBeFalsy('Origin.user.subscription() should return false');

            asyncTicker.clear(done);
        };

        var authFail = function() {
            Origin.events.off(Origin.events.AUTH_SUCCESSRETRY, authSuccess);
            Origin.events.off(Origin.events.AUTH_FAILEDCREDENTIAL, authFail);

            expect().toFail('event AUTH_FAILEDCREDENTIAL event should not be triggered');

            asyncTicker.clear(done);
        };

        Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, authSuccess);
        Origin.events.on(Origin.events.AUTH_FAILEDCREDENTIAL, authFail);

        asyncTicker.tick(1000, done, 'event Origin.events.AUTH_SUCCESSRETRY should be triggered');

        Origin.auth.login();

    });

    it('Origin.auth.logout()', function(done) {

        var asyncTicker = new AsyncTicker();

        var authLoggedout = function() {
            Origin.events.off(Origin.events.AUTH_LOGGEDOUT, authLoggedout);

            expect(Origin.auth.isLoggedIn()).toBeFalsy('Origin.auth.isLoggedIn() should return false');

            asyncTicker.clear(done);
        };

        Origin.events.on(Origin.events.AUTH_LOGGEDOUT, authLoggedout);

        asyncTicker.tick(1000, done, 'event Origin.events.AUTH_LOGGEDOUT should be triggered');

        Origin.auth.logout();

    });

    it('Origin.auth.login() 403 returned from server', function(done) {

        var asyncTicker = new AsyncTicker();

        var endPoint = 'http://127.0.0.1:1400/response/set';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, false);
        req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=utf-8');
        req.send('url=http://127.0.0.1:1400/data/jssdkfunctionaltest/auth?&statusCode=403&data={"access_token": "QVQw"}');

        asyncTicker.tick(1000, done, 'Expect onReject() to be executed');

        Origin.auth.login().then(function() {
            expect().toFail('Expect reject() to be called with status code 403');
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect(Origin.auth.isLoggedIn()).toBeFalsy('Origin.auth.isLoggedIn() should return false');
            asyncTicker.clear(done);
        });

    });

    it('Origin.auth.login() retry', function(done) {

        var asyncTicker = new AsyncTicker();

        // reset all pre-set responses in case any leftover preset responses
        var endPoint = 'http://127.0.0.1:1400/response/reset';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, false);
        req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=utf-8');
        req.send('');

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.auth.login().then(function() {
            expect(Origin.auth.isLoggedIn()).toBeTruthy('Origin.auth.isLoggedIn() should return true');
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail('Expect resolve() to be called, ' + error.message);
            asyncTicker.clear(done);
        });

    });

});
