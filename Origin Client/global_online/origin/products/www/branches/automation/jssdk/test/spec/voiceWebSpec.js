describe('Origin Voice API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.voice.supported()", function() {

        expect(Origin.voice.supported()).toBeFalsy();

    });

});
