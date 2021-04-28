/**
 * Jasmine functional test
 */

'use strict';

describe('Wishlist API Factory', function() {
    var wishlistFactory, wishlistStorageMock, featureConfig, GamesEntitlementFactory, GamesDataFactory, OfferTransformer, ProductInfoHelper, GameRefiner;

    beforeEach(function() {
        window.OriginComponents = {};

        function Communicator() {}
        Communicator.prototype.fire = function() {};

        window.Origin = {
            log: {
                'message': function(data) {console.log(data);}
            },
            utils: {
                os: function() { return 'PCWIN'; },
                Communicator: Communicator
            },
            user: {
                userPid: function() {}
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

            $provide.factory('FeatureConfig', function() {
                return {
                    isWishlistEnabled: function() { return true; }
                };
            });

            $provide.factory('GamesEntitlementFactory', function() {
                return {
                    getAllEntitlements: function() {}
                };
            });

            $provide.factory('GamesDataFactory', function() {
                return {
                    getCatalogInfo: function(){}
                };
            });

            $provide.factory('OfferTransformer', function() {
                return {
                    transformOffer: function(){}
                };
            });
            $provide.factory('ProductInfoHelper', function() {
                return {
                    isSubscriptionAvailable: function(){}
                };
            });
            $provide.factory('GameRefiner', function() {
                return {
                    isOwnedOutright: function(){}
                };
            });
        });

        wishlistStorageMock = {
            getOfferList: function() {},
            addOffer: function() {},
            removeOffer: function() {},
            getWishlistId: function() {},
            getUserIdByWishlistId: function() {}
        };
    });

    beforeEach(inject(function(_WishlistFactory_, _FeatureConfig_, _GamesEntitlementFactory_, _GamesDataFactory_, _OfferTransformer_, _ProductInfoHelper_, _GameRefiner_) {
        wishlistFactory = _WishlistFactory_;
        featureConfig = _FeatureConfig_;
        GamesEntitlementFactory = _GamesEntitlementFactory_;
        GamesDataFactory = _GamesDataFactory_;
        OfferTransformer = _OfferTransformer_;
        ProductInfoHelper = _ProductInfoHelper_;
        GameRefiner = _GameRefiner_;

    }));

    describe('isWishlistEnabled', function() {
        it('will allow business to enable the feature operationally', function() {
            spyOn(featureConfig, 'isWishlistEnabled').and.returnValue(true);

            expect(wishlistFactory.isWishlistEnabled()).toBe(true);
            expect(featureConfig.isWishlistEnabled).toHaveBeenCalled();
        });
    });

    describe('getWishlist', function() {
        it('will create a wishlist instance', function() {
            var wishlist = wishlistFactory.getWishlist('jim108', wishlistStorageMock);

            expect(wishlist.userId).toBe('jim108');
            expect(wishlist.storage).toBe(wishlistStorageMock);
        });
    });

    describe('isOfferInWishlist', function() {
        var wishlist, offerList= [
                {id: 'abc123'},
                {id: 'foo345'}
            ];


        beforeEach(function() {
            wishlist = wishlistFactory.getWishlist('jim108', wishlistStorageMock);
        });

        it('will determine if an offer is in a wishlist group', function() {
            spyOn(wishlistStorageMock, 'getOfferList').and.returnValue(Promise.resolve(offerList));
            wishlist.isOfferInWishlist('abc123').then(function(data) {
                expect(data).toEqual(true);
            });
        });

        it('will determine if an offer is not in a wishlist group', function() {
            spyOn(wishlistStorageMock, 'getOfferList').and.returnValue(Promise.resolve(offerList));
            wishlist.isOfferInWishlist('def123').then(function(data) {
                expect(data).toEqual(false);
            });
        });

        it('will return false if the offe list is not defined', function() {
            spyOn(wishlistStorageMock, 'getOfferList').and.returnValue(Promise.resolve([]));
            wishlist.isOfferInWishlist('abc123').then(function(data) {
                expect(data).toEqual(false);
            });
        });
    });

    describe('getUserIdByWishlistId', function() {
        it('utility func to return a userid in exchange for a wishlist id', function(done) {
            var wishlistId = 'e8da279584',
                userId = 'jim109';

            spyOn(wishlistStorageMock, 'getUserIdByWishlistId').and.returnValue(Promise.resolve(userId));

            wishlistFactory.getUserIdByWishlistId(wishlistId, wishlistStorageMock)
                .then(function(data) {
                    expect(wishlistStorageMock.getUserIdByWishlistId).toHaveBeenCalledWith(wishlistId);
                    expect(data).toBe(userId);

                    done();
                });
        });
    });

    describe('<<Wishlist>> instance', function() {
        var wishlist, userId, offerList, offerId, timestamp, offerData;

        beforeEach(function() {
            userId = 'jim108';
            offerId = 'OFB-EAST:12345';
            timestamp = '2016-09-24T00:50:41.478Z';
            offerList = [
                {'id': offerId, 'ts': timestamp}
            ];
            offerData = {'OFB-EAST:12345': {subscriptionAvailable: true, isOwned: false, vaultEntitlement: true}};
            wishlist = wishlistFactory.getWishlist(userId, wishlistStorageMock);
        });

        it('will get a list of offers if wishlist is accessed by the current user', function(done) {
            spyOn(wishlistStorageMock, 'getOfferList').and.returnValue(Promise.resolve(offerList));
            spyOn(OfferTransformer, 'transformOffer').and.callFake(function(promise) {
                return Promise.resolve(promise);
            });
            spyOn(GamesDataFactory, 'getCatalogInfo').and.callFake(function() {
                return Promise.resolve(offerData);
            });
            spyOn(window.Origin.user, 'userPid').and.returnValue(userId);
            spyOn(GamesEntitlementFactory, 'getAllEntitlements').and.returnValue({});

            wishlist.getOfferList()
                .then(function(data) {
                    expect(wishlistStorageMock.getOfferList).toHaveBeenCalledWith(userId);
                    expect(data).toEqual(offerList);

                    done();
                });
        });

        it('will get a filtered list of offers if wishlist is accessed by the current user and the user owns the game', function(done) {
            spyOn(wishlistStorageMock, 'getOfferList').and.returnValue(Promise.resolve(offerList));
            spyOn(OfferTransformer, 'transformOffer').and.callFake(function(promise) {
                return Promise.resolve(promise);
            });
            spyOn(GamesDataFactory, 'getCatalogInfo').and.callFake(function() {
                return Promise.resolve(offerData);
            });
            spyOn(window.Origin.user, 'userPid').and.returnValue(userId);
            spyOn(GamesEntitlementFactory, 'getAllEntitlements').and.returnValue({
                'OFB-EAST:12345': {'offerId': 'OFB-EAST:12345'}
            });

            wishlist.getOfferList()
                .then(function(data) {
                    expect(wishlistStorageMock.getOfferList).toHaveBeenCalledWith(userId);
                    expect(data).toEqual([]);

                    done();
                });
        });

        it('will get a full list of offers if wishlist is accessed by the current user and the user owns the game through access', function(done) {
            spyOn(wishlistStorageMock, 'getOfferList').and.returnValue(Promise.resolve(offerList));
            spyOn(OfferTransformer, 'transformOffer').and.callFake(function(promise) {
                return Promise.resolve(promise);
            });
            spyOn(GamesDataFactory, 'getCatalogInfo').and.callFake(function() {
                return Promise.resolve(offerData);
            });
            spyOn(ProductInfoHelper, 'isSubscriptionAvailable').and.returnValue(true);
            spyOn(GameRefiner, 'isOwnedOutright').and.returnValue(false);
            spyOn(window.Origin.user, 'userPid').and.returnValue(userId);
            spyOn(GamesEntitlementFactory, 'getAllEntitlements').and.returnValue({
                'OFB-EAST:12345': {'offerId': 'OFB-EAST:12345'}
            });

            wishlist.getOfferList()
                .then(function(data) {
                    expect(wishlistStorageMock.getOfferList).toHaveBeenCalledWith(userId);
                    expect(data).toEqual(offerList);

                    done();
                });
        });

        it('will get an unfiltered list of offers from the persistence layer if wishlist is accessed by a friend', function(done) {
            spyOn(wishlistStorageMock, 'getOfferList').and.returnValue(Promise.resolve(offerList));

            wishlist.getOfferList()
                .then(function(data) {
                    expect(wishlistStorageMock.getOfferList).toHaveBeenCalledWith(userId);
                    expect(data).toEqual(offerList);

                    done();
                });
        });

        it('will add an offer to persistence where a precise time is given', function(done) {
            spyOn(wishlistFactory.events, 'fire').and.callThrough();
            spyOn(wishlistStorageMock, 'addOffer').and.returnValue(Promise.resolve(offerList));

            wishlist.addOffer(offerId, new Date(timestamp));

            wishlistStorageMock.addOffer(offerId, timestamp)
                .then(function(data) {
                    expect(wishlistStorageMock.addOffer).toHaveBeenCalledWith(userId, offerId, timestamp);
                    expect(wishlistFactory.events.fire).toHaveBeenCalledWith('wishlist:addOffer:jim108');
                    expect(data).toEqual(offerList);

                    done();
                });
        });

        it('will add an offer to persistence where the time must be the curent local time', function(done) {
            spyOn(wishlistStorageMock, 'addOffer').and.returnValue(Promise.resolve(offerList));

            wishlist.addOffer(offerId);

            wishlistStorageMock.addOffer(offerId, timestamp)
                .then(function(data) {
                    expect(wishlistStorageMock.addOffer).toHaveBeenCalled();
                    expect(data).toEqual(offerList);

                    done();
                });
        });

        it('will remove an offer from persistence', function(done) {
            spyOn(wishlistFactory.events, 'fire').and.callThrough();
            spyOn(wishlistStorageMock, 'removeOffer').and.returnValue(Promise.resolve(offerList));

            wishlist.removeOffer(offerId)
                .then(function(data) {
                    expect(wishlistStorageMock.removeOffer).toHaveBeenCalledWith(userId, offerId);
                    expect(wishlistFactory.events.fire).toHaveBeenCalledWith('wishlist:removeOffer:jim108');
                    expect(data).toEqual(offerList);

                    done();
                });
        });

        it('will return a wishlistId for use with social sharing', function(done) {
            spyOn(wishlistStorageMock, 'getWishlistId').and.returnValue(Promise.resolve('e8da279584'));

            var userId = 'jim109',
                wishlist = wishlistFactory.getWishlist(userId, wishlistStorageMock);

            wishlist.getWishlistId()
                .then(function(data) {
                    expect(wishlistStorageMock.getWishlistId).toHaveBeenCalledWith('jim109');
                    expect(data).toBe('e8da279584');

                    done();
                });
        });
    });
});