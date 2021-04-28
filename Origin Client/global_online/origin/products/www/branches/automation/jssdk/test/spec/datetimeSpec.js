describe('Origin Datetime API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.datetime.getTrustedClock()', function() {
        var timeDiff = Math.abs(Origin.datetime.getTrustedClock()-Date.now());
        expect(timeDiff).toBeLessThan(1000);
        expect(Origin.datetime.isInitialized()).toBeFalsy('Clock should not be initialized yet');
    });

    it('Origin.datetime.initializeTrustedClock()', function() {
        // Cannot return this trusted clock now, just set flag at the moment
        Origin.datetime.initializeTrustedClock(11000);
        expect(Origin.datetime.isInitialized()).toBeTruthy('Clock should be initialized');
    });

    it('Origin.datetime.secondsToDHMS()', function() {
        var expectedTime = Origin.datetime.secondsToDHMS(112220000);
        var checkTime = new Date(112220000000);
        expect(expectedTime.days).toEqual(Math.floor(checkTime/8.64e7));
        expect(expectedTime.hours).toEqual(checkTime.getUTCHours());
        expect(expectedTime.minutes).toEqual(checkTime.getUTCMinutes());
        expect(expectedTime.seconds).toEqual(checkTime.getUTCSeconds());
    });

});
