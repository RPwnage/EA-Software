describe('Origin Wishlist API', function() {
    it('will get a wishlist offer list', function(done) {
        Origin.wishlist.getOfferList('1000120382991')
            .then(function(data) {
                // Offer Id is present
                expect(data.wishlist[0].offerId).toBe('Origin.OFR.50.0000940');

                // List indexed from 1
                expect(data.wishlist[0].displayOrder).toBe(1);

                // Timestamps represented as Unix time stamps
                expect(data.wishlist[0].addedAt).toBe(1476897845000);
                done();
            })
            .catch(function(err) {
                fail(err);
                done(err);
            });
    });

    it('will remove an offer from the list', function(done) {
        Origin.wishlist.addOffer('1000120382991', 'DR226213600')
            .then(function(data) {
                expect(data).toBe('ok');
                done();
            })
            .catch(function(err) {
                fail(err);
                done(err);
            });
    });

    it('will delete an offer from the list', function(done) {
        Origin.wishlist.removeOffer('1000120382991', 'DR226213600')
            .then(function(data) {
                expect(data).toBe('ok');
                done();
            })
            .catch(function(err) {
                fail(err);
                done(err);
            });
    });
});