/**
 * Jasmine functional test
 */

'use strict';

describe('Wishlist Observer factory', function() {
    var wishlistObserverFactory,
        wishlistFactory,
        observerFactory,
        mockScope;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            'utils': {
                'Communicator': function() {}
            },
            'client': {
                games: {
                    isGamePlaying: function() {}
                }
            }
        };

        angular.mock.module('origin-components');

        module(function($provide){
            $provide.factory('WishlistFactory', function() {
                return {
                    getWishlist: function() {},
                    events: {
                        on: function() {}
                    }
                };
            });
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    error: function() {}
                };
            });
            $provide.factory('ObserverFactory', function() {
                return {
                    create: function() {
                        return {
                            addFilter: function() {
                                return this;
                            },
                            onUpdate: function() {
                                return this;
                            }
                        };
                    },
                    noDigest: function() {}
                };
            });

            $provide.factory('GamesDataFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });

            $provide.factory('GamesClientFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    }
                };
            });
        });

        mockScope = {
            $on: function() {},
            $destroy: function() {}
        };
    });

    beforeEach(inject(function(_WishlistObserverFactory_, _ObserverFactory_, _WishlistFactory_) {
        wishlistFactory = _WishlistFactory_;
        wishlistObserverFactory = _WishlistObserverFactory_;
        observerFactory = _ObserverFactory_;
    }));

    describe('getObserver', function() {
        it('will provide an observer handle that calls a callback', function() {
            var userId = 'joe:2010',
                callback = function() {};

            var observer = wishlistObserverFactory.getObserver(userId, mockScope, callback);

            expect(typeof(observer)).toEqual('object');
        });
    });

    describe('getModel', function() {
        it('will cast a sucessful response into a success object', function(done) {
            var offerList = [
                {'id': 'OFB-EAST:12345', 'ts': '2016-09-24T00:50:41.478Z'}
            ];

            spyOn(wishlistFactory, 'getWishlist').and.returnValue({
                getOfferList: function() {
                    return Promise.resolve(offerList);
                }
            });

            wishlistObserverFactory.getModel('joe108')
                .then(function(model) {
                    expect(model).toEqual({
                        offerList: offerList,
                        errorMessage: undefined,
                        isPrivate: false
                    });

                    done();
                });
        });

        it('will cast an error from a private response into an error object', function(done) {
            var errorMessage = 'origin-jssdk.js:4357 GET https://dev.api2.origin.com/private/account 409 ()';

            spyOn(wishlistFactory, 'getWishlist').and.returnValue({
                getOfferList: function() {
                    return Promise.reject(new Error(errorMessage));
                }
            });

            wishlistObserverFactory.getModel('joe108')
                .then(function(model) {
                    expect(model).toEqual({
                        offerList: [],
                        errorMessage: errorMessage,
                        isPrivate: true
                    });

                    done();
                });
        });

        it('will cast an error from an other http error response into an error object', function(done) {
            var errorMessage = 'origin-jssdk.js:4357 GET https://dev.api2.origin.com/nonexistent/account 404 ()';

            spyOn(wishlistFactory, 'getWishlist').and.returnValue({
                getOfferList: function() {
                    return Promise.reject(new Error(errorMessage));
                }
            });

            wishlistObserverFactory.getModel('joe108')
                .then(function(model) {
                    expect(model).toEqual({
                        offerList: [],
                        errorMessage: errorMessage,
                        isPrivate: false
                    });

                    done();
                });
        });
    });
});