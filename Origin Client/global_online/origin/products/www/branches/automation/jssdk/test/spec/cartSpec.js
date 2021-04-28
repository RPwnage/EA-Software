describe('Origin Cart API', function() {

	beforeEach(function() {
		jasmine.addMatchers(matcher);
	});

    it('Origin.cart.addOffer()', function(done) {
        var data = {
            offerIdList: [22, 23],
            bundleType: 'testBundle',
            bundlePromotionRuleId: 'testRule',
            cartName: 'testCart',
            storeId: 'testStore',
            currency: 'testCurrency',
            countryCode: 'testCountry'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'addOffer did not complete in 1000ms');
        Origin.cart.addOffer(data).then(function(result) {
            expect(result.cartResponse.offerId).toEqual(22, 'Expected the offerId to be 22');
            expect(result.cartResponse.quantity).toEqual(55, 'Expected the quanity to be 55');
            timeout.clear(done);
        }).catch(function(reason) {
            timeout.clear(done);
            expect().toFail(reason.message);
        });
    });

    it('Origin.cart.addCoupon()', function(done) {
        var data = {
            cartName: 'testCart',
            storeId: 'testStore',
            couponCode: 'testCode',
            countryCode: 'testCountry'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'addCoupon did not complete in 1000ms');
        Origin.cart.addCoupon(data).then(function(result) {
            expect(result.cartResponse.couponEntry).toEqual([1,2,3], 'Expected the couponEntry to be [1,2,3]');
            timeout.clear(done);
        }).catch(function(error) {
            timeout.clear(done);
            expect().toFail(error.message);
        });
    });

    it('Origin.cart.clearCart()', function(done) {
        var data = {
            cartName: 'testCart',
            countryCode: 'testCountry'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'addCoupon did not complete in 1000ms');
        Origin.cart.clearCart(data).then(function(result) {
            expect(result.cartResponse.cartName).toEqual('mycart', 'Expected the cartName to be mycart')
            expect(result.cartResponse.totalNumberOfItems).toEqual(0, 'Expected the totalNumberOfItems to be 0');
            timeout.clear(done);
        }).catch(function(error) {
            timeout.clear(done);
            expect().toFail(error.message);
        });
    });

    it('Origin.cart.getCart()', function(done) {
        var data = {
            cartName: 'testCart',
            storeId: 'testStoreId',
            currency: 'testCurrency',
            countryCode: 'testCountry'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'getCart did not complete in 1000ms');
        Origin.cart.getCart(data).then(function(result) {
            expect(result.cartResponse.cartName).toEqual('mycart', 'Expected the cartName to be mycart')
            expect(result.cartResponse.totalNumberOfItems).toEqual(2, 'Expected the totalNumberOfItems to be 0');
            timeout.clear(done);
        }).catch(function(error) {
            timeout.clear(done);
            expect().toFail(error.message);
        });
    });

    it('Origin.cart.mergeCart()', function(done) {
        var data = {
            anonymousToken: 'testToken',
            cartName: 'testCart',
            sourcePidId: 'testPid',
            countryCode: 'testCountry'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'mergeCart did not complete in 1000ms');
        Origin.cart.mergeCart(data).then(function(result) {
            expect(result.cartResponse.cartName).toEqual('mycart', 'Expected the cartName to be mycart')
            expect(result.cartResponse.totalNumberOfItems).toEqual(0, 'Expected the totalNumberOfItems to be 0');
            timeout.clear(done);
        }).catch(function(error) {
            timeout.clear(done);
            expect().toFail(error.message);
        });
    });

    it('Origin.cart.removeCoupon()', function(done) {
        var data = {
            cartName: 'testCart',
            storeId: 'testStore',
            couponCode: 'testCode',
            countryCode: 'testCountry'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'addCoupon did not complete in 1000ms');
        Origin.cart.addCoupon(data).then(function(result) {
            expect(result.cartResponse.couponEntry).toEqual([1,2,3], 'Expected the couponEntry to be [1,2,3]');
            timeout.clear(done);
        }).catch(function(error) {
            timeout.clear(done);
            expect().toFail(error.message);
        });
    });

    it('Origin.cart.addCoupon()', function(done) {
        var data = {
            cartName: 'testCart',
            storeId: 'testStore',
            couponCode: 'testCode',
            countryCode: 'testOtherCode'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'addCoupon did not complete in 1000ms');
        Origin.cart.addCoupon(data).then(function(result) {
            expect(result.cartResponse.couponEntry).toEqual([1,2,3], 'Expected the couponEntry to be [1,2,3]');
            timeout.clear(done);
        }).catch(function(error) {
            timeout.clear(done);
            expect().toFail(error.message);
        });
    });

    it('Origin.cart.removeOffer()', function(done) {
        var data = {
            cartName: 'testCart',
            storeId: 'testStore',
            currency: 'testCurrency',
            offerEntryID: 'testId',
            countryCode: 'testCountry'
        };

        var timeout = new AsyncTicker();
        timeout.tick(1000, done, 'removeOffer did not complete in 1000ms');
        Origin.cart.removeOffer(data).then(function(result) {
            expect(result).toBeTruthy();
            timeout.clear(done);
        }).catch(function(reason) {
            timeout.clear(done);
            expect().toFail(reason.message);
        });
    });

});
