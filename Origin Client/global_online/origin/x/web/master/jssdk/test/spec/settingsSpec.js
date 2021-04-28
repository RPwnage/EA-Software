describe('Origin Settings API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.settings.postUserSettings()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.settings.postUserSettings('GENERAL','telemetry_enabled=true').then(function(result) {

            var expected = {
                userSettings : {
                    userId : '1000147395502',
                    payload : null
                }
            }
            
            expect(result).toEqual(expected);

            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
    
    it('Origin.settings.retrieveUserSettings()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.settings.retrieveUserSettings().then(function(result) {
            
            var expected = {
                userSettings : {
                    userId : '1000147395502',
                    payload : null
                }
            }
            
            expect(result).toEqual(expected)
            
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});