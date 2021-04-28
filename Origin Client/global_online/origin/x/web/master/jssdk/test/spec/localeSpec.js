describe('Origin Locale API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.locale.countryCode()', function() {
        expect(Origin.locale.countryCode()).toEqual('US');
    });
	
	it('Origin.locale.locale()', function() {
        expect(Origin.locale.locale()).toEqual('en_US');
    });
	
	it('Origin.locale.setLocale()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        var localeTrigger = function() {
            Origin.events.off(Origin.events.LOCALE_CHANGED, localeTrigger);
            asyncTicker.clear(done);
        };

        Origin.events.on(Origin.events.LOCALE_CHANGED, localeTrigger);
		asyncTicker.tick(1000, done, 'event Origin.events.LOCALE_CHANGED should be triggered');
        Origin.locale.setLocale('en_GB');
    });
});
