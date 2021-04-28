describe('Origin Navigation API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.events.CLIENT_NAV_OPENMODAL_FINDFRIENDS', function(done) {

      var asyncTicker = new AsyncTicker();

      asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_NAV_OPENMODAL_FINDFRIENDS should be triggered');

      var cb = function(result) {
        Origin.events.off(Origin.events.CLIENT_NAV_OPENMODAL_FINDFRIENDS, cb);

        expect(result).toEqual(true);
        asyncTicker.clear(done);

      };

      Origin.events.on(Origin.events.CLIENT_NAV_OPENMODAL_FINDFRIENDS, cb);

      window.OriginClientRouting.openModalFindFriends(true);

    });
});
