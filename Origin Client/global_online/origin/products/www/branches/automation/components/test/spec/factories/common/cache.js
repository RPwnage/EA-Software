/**
 * Generic cache decorator functional tests
 */

'use strict';

describe('Cache decorator factory - an in-memory, ttl based cache implementation', function() {
    var cacheFactory;

    beforeEach(function () {
        window.OriginComponents = {};
        module('origin-components');
    });

    beforeEach(inject(function (_CacheFactory_) {
        cacheFactory = _CacheFactory_;
    }));

    describe('decorate - wrap any javascript function for getting data', function() {
        // the dummy synchronous data getter
        function add(a, b) {
            return a + b;
        }

        // the dummy promise context data getter
        function addAsync(a, b) {
            return Promise.resolve(a + b);
        }

        var helpers = {
            //expects arguments list as an array - returns unique storage key
            getKey: function() {
                return 'abc';
            },

            //expects an input of {Date} and return boolean
            isExpired: function(date) {
                return false;
            }
        };

        it('Will wrap a synchronous function results using the default strategies for key and expiry generation - no ttl provided', function () {
            var cachedAdd = cacheFactory.decorate(add);

            expect(cachedAdd(1,2)).toEqual(3);
        });

        it('Allows for custom key generating and expiry callbacks', function () {
            spyOn(helpers, 'getKey').and.callThrough();
            spyOn(helpers, 'isExpired').and.callThrough();

            var cachedAdd = cacheFactory.decorate(add, 500, helpers.getKey, helpers.isExpired);

            // simulate a cache miss - only the key gen helper should be run
            expect(cachedAdd(12, 3)).toEqual(15);
            expect(helpers.getKey).toHaveBeenCalled();
            expect(helpers.getKey).toHaveBeenCalledWith([12, 3]);

            // now there will be a value in storage - the expiry helper will be called
            cachedAdd(12, 3);
            expect(helpers.isExpired).toHaveBeenCalled();
        });

        it('will evict old records', function() {
            spyOn(helpers, 'isExpired').and.returnValue(true);

            var cachedAdd = cacheFactory.decorate(add, undefined, undefined, helpers.isExpired);

            expect(cachedAdd(4, 3)).toEqual(7);
            expect(helpers.isExpired).not.toHaveBeenCalled();
            expect(cachedAdd(4, 3)).toEqual(7);
            expect(helpers.isExpired).toHaveBeenCalled();
            expect(cachedAdd(4, 3)).toEqual(7);
            expect(helpers.isExpired).toHaveBeenCalled();
        });

        it('Will cache promise continuations', function(done) {
            spyOn(helpers, 'isExpired').and.callThrough();

            var cachedAdd = cacheFactory.decorate(addAsync, 500, undefined, helpers.isExpired);

            cachedAdd(1, 5)
                .then(function(data) {
                    expect(data).toBe(6);

                    return cachedAdd(1, 5)
                        .then(function(data) {
                            expect(helpers.isExpired).toHaveBeenCalled();
                            expect(data).toBe(6);
                            done();
                        });
                });
        });

        it('Will not cache promise continuations if they throw an exception and will pass through the exception as is', function(done) {
            spyOn(helpers, 'isExpired').and.callThrough();

            function addFails(a, b) {
                return Promise.reject('Nope!');
            }

            var cachedAddFails = cacheFactory.decorate(addFails, 500, undefined, helpers.isExpired);

            cachedAddFails(1, 5)
                .catch(function(err){
                    expect(err).toEqual('Nope!');

                    return cachedAddFails(1, 5)
                        .catch(function(err) {
                            expect(helpers.isExpired).toHaveBeenCalled();
                            done();
                        });
                });
        });
    });

    describe('decorate - wrap promises with the decorator', function() {
        function getCollection() {
            return Promise.resolve([
                {
                    name: 'Bill Block'
                }, {
                    name: 'William Foe'
                }, {
                    name: 'Oouphus Seal'
                }
            ]);
        }

        function filterName(name) {
            return function (data) {
                return _.filter(data, function(rosterItem) { return (rosterItem.name === name) ? true: false; });
            };
        }

        it('Will internally unwrap promises to preserve a snapshot of the function\'s state', function(done) {
            var cachedGetCollection = cacheFactory.decorate(getCollection);

            //subsequent calls within the context will be cached and referent safe
            cachedGetCollection()
                .then(filterName('Bill Block'))
                .then(function(data) {
                    expect(data.length).toBe(1);
                    expect(data).toEqual([{ name: 'Bill Block'}]);

                    cachedGetCollection()
                        .then(filterName('Oouphus Seal'))
                        .then(function(data) {
                            expect(data.length).toBe(1);
                            expect(data).toEqual([{ name: 'Oouphus Seal'}]);

                            done();
                        })
                        .catch(function(err) {
                            fail(err);
                            done();
                        });
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('decorate - regenerate the cache using an extra argument', function() {
        var myMath = {
            add: function (a, b) {
                return a + b;
            }
        };

        it('will allow for "forced refreshing" using an argument', function() {
            spyOn(myMath, 'add').and.callThrough();

            var cachedAdd = cacheFactory.decorate(myMath.add);

            expect(cachedAdd(4, 3, cacheFactory.refresh(true))).toEqual(7);
            expect(myMath.add).toHaveBeenCalledWith(4, 3);
        });
    });
});