describe('Initialize', function() {

    beforeEach(function() {
        originalTimeout = jasmine.DEFAULT_TIMEOUT_INTERVAL;
        jasmine.DEFAULT_TIMEOUT_INTERVAL = 10000;
        jasmine.addMatchers(matcher);
    });

    afterEach(function() {
        jasmine.DEFAULT_TIMEOUT_INTERVAL = originalTimeout;
    });

    var baseURL = 'http://127.0.0.1:1400';
    var basePath = '/data/jssdkfunctionaltest';
    var jssdkOverrideURL = baseURL + basePath + '/jssdkconfig.json';

    it('is mockup server ready (5 seconds timeout)', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(5000, done, 'Expect mockup server is running in 5 seconds');

        setTimeout(function() {
            var endPoint = baseURL + basePath + '/blank.json';
            var req = new XMLHttpRequest();
            req.timeout = 1000;
            req.open('GET', endPoint, true /*async*/ );

            req.onload = function() {
                expect(req.status).toEqual(200, 'Expect 200 returned');
                asyncTicker.clear(done);
            };

            req.onerror = function(e) {
                expect().toFail('Error : req.status (' + req.status + ')');
                asyncTicker.clear(done);
            };

            req.ontimeout = function() {
                expect().toFail('Expect mockup server to response before timeout');
                asyncTicker.clear(done);
            };

            req.send('');
        }, 3000);

    });

    it('Origin.init()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(3000, done, 'Expect onResolve() to be executed');

        var req = new XMLHttpRequest();
        req.timeout = 1000;
        req.open('GET', jssdkOverrideURL, true /*async*/ );

        req.onload = function() {
            Origin.utils.mix(Origin.config, JSON.parse(req.response));
            Origin.init('en_US').then(function(result) {
                expect().toPass(result);
                asyncTicker.clear(done);
            }).catch(function(error) {
                expect().toFail(error.message);
                asyncTicker.clear(done);
            });
        };

        req.onerror = function(e) {
            expect().toFail('Error : could not retrive jssdk mocked endpoints(' + req.status + ')');
            asyncTicker.clear(done);
        };

        req.ontimeout = function() {
            expect().toFail('Expect jssdk mocked endpoints timed out');
            asyncTicker.clear(done);
        };

        req.send('');



    });
	
	it('Origin.version()', function() {
		// Since version could change, we can only test it does not crash
	    expect(Origin.version()).not.toBeNull();
		expect(Origin.version().length > 0).toBeTruthy();
    });

});