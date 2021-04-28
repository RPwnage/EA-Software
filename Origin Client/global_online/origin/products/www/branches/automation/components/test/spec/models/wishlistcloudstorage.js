/**
 * Jasmine functional test
 */

'use strict';

describe('Wishlist API Factory', function() {
    var wishlistCloudStorage;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            log: {
                'message': function(data) {console.log(data);}
            },
            wishlist: {
                getOfferList: function() {},
                addOffer: function() {},
                removeOffer: function() {}
            },
            idObfuscate: {
                encodeUserId: function() {},
                decodeUserId: function() {}
            }
        };

        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    error: function() {},
                    log: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_WishlistCloudStorageFactory_) {
        wishlistCloudStorage = _WishlistCloudStorageFactory_;
    }));

    describe('getUserIdByWishlistId', function() {
        it('will fetch a user ID given a wishlist Id', function(done) {
            var nucleusId = '1029090390',
                wishlistId = 'yTe02i20ie02ie0iii--';

            spyOn(window.Origin.idObfuscate, 'decodeUserId').and.returnValue(Promise.resolve({
                id: nucleusId
            }));

            wishlistCloudStorage.getUserIdByWishlistId(wishlistId)
                .then(function(data) {
                    expect(window.Origin.idObfuscate.decodeUserId).toHaveBeenCalledWith(wishlistId);
                    expect(data).toEqual(nucleusId);

                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });
        });
    });

    describe('getWishlistId', function() {
        it('will fetch a wishlist ID given a nucleusId', function(done) {
            var nucleusId = '1029090390',
                wishlistId = 'yTe02i20ie02ie0iii--';

            spyOn(window.Origin.idObfuscate, 'encodeUserId').and.returnValue(Promise.resolve({
                id: wishlistId
            }));

            wishlistCloudStorage.getWishlistId(nucleusId)
                .then(function(data) {
                    expect(window.Origin.idObfuscate.encodeUserId).toHaveBeenCalledWith(nucleusId);
                    expect(data).toEqual(wishlistId);

                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });
        });
    });

    describe('getOfferList', function() {
        it('will fetch an empty wishlist', function(done) {
            var nucleusId = '1029090390';

            spyOn(window.Origin.wishlist, 'getOfferList').and.returnValue(Promise.resolve({
                wishlist: []
            }));

            wishlistCloudStorage.getOfferList(nucleusId)
                .then(function(data) {
                    expect(window.Origin.wishlist.getOfferList).toHaveBeenCalledWith(nucleusId);
                    expect(data).toEqual([]);

                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });
        });

        it('will fetch a populated wishlist', function(done) {
            var date = new Date('2016-10-25T20:39:52.000Z'),
                isoDate = date.toISOString(),
                unixDate = date.toString(),
                nucleusId = '1029090390',
                offerId = 'OFB:EAST12345';

            spyOn(window.Origin.wishlist, 'getOfferList').and.returnValue(Promise.resolve({
                wishlist: [{
                    offerId: offerId,
                    addedAt: unixDate,
                    displayOrder: 1
                }]
            }));

            wishlistCloudStorage.getOfferList(nucleusId)
                .then(function(data) {
                    expect(window.Origin.wishlist.getOfferList).toHaveBeenCalledWith(nucleusId);
                    expect(data).toEqual([{
                        id: offerId,
                        ts: isoDate
                    }]);

                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done(err);
                });
        });

        it('will handle an HTTP 409 by rejecting the promise - used when the user is a friend and has set their visibility to private', function(done) {
            var nucleusId = '29029090002';

            spyOn(window.Origin.wishlist, 'getOfferList').and.returnValue(Promise.reject(new Error('409')));

            wishlistCloudStorage.getOfferList(nucleusId)
                .then(function(data) {
                    this.fail(Error(data));
                    done();
                })
                .catch(function(err) {
                    expect(_.get(err, 'message')).toContain('409');
                    done();
                });
        });
    });

    describe('addOffer', function() {
        it('will add an offer to the wishlist', function(done) {
            var nucleusId = '1029090390',
                offerId = 'OFB:EAST12345';

            spyOn(window.Origin.wishlist, 'addOffer').and.returnValue(Promise.resolve('ok'));

            wishlistCloudStorage.addOffer(nucleusId, offerId)
                .then(function(data) {
                    expect(window.Origin.wishlist.addOffer).toHaveBeenCalledWith(nucleusId, offerId, undefined);
                    expect(data).toEqual('ok');

                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });
        });

        it('will handle a 409 by rejecting the promise - used when the wishlist cap is reached or the same offer is added again', function(done) {
            var nucleusId = '29029090002',
                offerId = 'OFB:EAST12345';

            spyOn(window.Origin.wishlist, 'addOffer').and.returnValue(Promise.reject(new Error('409')));

            wishlistCloudStorage.addOffer(nucleusId, offerId)
                .then(function(data) {
                    this.fail(Error(data));
                    done();
                })
                .catch(function(err) {
                    expect(_.get(err, 'message')).toContain('409');
                    done();
                });
        });
    });

    describe('removeOffer', function() {
        it('will remove an offer from the wishlist', function(done) {
            var nucleusId = '1029090390',
                offerId = 'OFB:EAST12345';

            spyOn(window.Origin.wishlist, 'removeOffer').and.returnValue(Promise.resolve('ok'));

            wishlistCloudStorage.removeOffer(nucleusId, offerId)
                .then(function(data) {
                    expect(window.Origin.wishlist.removeOffer).toHaveBeenCalledWith(nucleusId, offerId);
                    expect(data).toEqual('ok');

                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });
        });
    });
});