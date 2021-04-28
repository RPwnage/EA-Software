/**
 * Object-helper functional tests
 */

'use strict';

describe('Object helper factory', function () {
    var objectHelper,
        nestedData = {
            a: 'blah',
            b: 'nothing here',
            c: {
                d: 12345
            }
        },
        testObject = {
            something: 'some text',
            a: 'some other text',
            '200': 100,
            nestedData: nestedData
        },
        doubles = {
            callback: function () {}
        };

    beforeEach(function () {
        window.OriginComponents = {};
        module('origin-components');
    });

    beforeEach(inject(function (_ObjectHelperFactory_) {
        objectHelper = _ObjectHelperFactory_;
    }));

    describe('toArray - type casting to native Array', function () {
        it('should convert the first level of object property nesting to native Array', function () {
            expect(objectHelper.toArray(testObject))
                .toEqual(['some text', 'some other text', 100, {a: 'blah', b: 'nothing here', c: {d: 12345}}]);
        });

        it('should wrap primitives into native arrays', function () {
            expect(objectHelper.toArray(100)).toEqual([100]);
            expect(objectHelper.toArray(true)).toEqual([true]);
            expect(objectHelper.toArray('string')).toEqual(['string']);
        });

        it('should pass through arrays, nulls and undefined\'s', function () {
            expect(objectHelper.toArray(undefined)).toEqual(undefined);
            expect(objectHelper.toArray(null)).toEqual(null);
            expect(objectHelper.toArray(['blah', 'nothing here', {d: 12345}])).toEqual(['blah', 'nothing here', {d: 12345}]);
        });
    });

    describe('foreach - iterating over an object', function () {
        it('should call the callback for each 1-level property of a given object', function () {
            spyOn(doubles, 'callback');

            objectHelper.forEach(doubles.callback, testObject);

            expect(doubles.callback.calls.count()).toEqual(4);

            expect(doubles.callback.calls.argsFor(0)[0]).toEqual('some text');
            expect(doubles.callback.calls.argsFor(1)[0]).toEqual('some other text');
            expect(doubles.callback.calls.argsFor(2)[0]).toEqual(100);
            expect(doubles.callback.calls.argsFor(3)[0]).toEqual(nestedData);

            expect(doubles.callback.calls.argsFor(0)[1]).toEqual('something');
            expect(doubles.callback.calls.argsFor(1)[1]).toEqual('a');
            expect(doubles.callback.calls.argsFor(2)[1]).toEqual('200');
            expect(doubles.callback.calls.argsFor(3)[1]).toEqual('nestedData');
        });
    });

    describe('forEachWith - Apply foreach with a function', function() {
        it ('will, given a function, iterate over a collection', function() {
            var addBaz = function(object) {
                object.baz = 'baz';
                return object;
            };

            var inputObject = {foo: {bar: 'bar'}, oil: {wax: 'wax'}};
            var outputObject = objectHelper.forEachWith(addBaz)(inputObject);

            expect(outputObject.foo.bar).toBe('bar');
            expect(outputObject.foo.baz).toBe('baz');
            expect(outputObject.oil.wax).toBe('wax');
            expect(outputObject.oil.baz).toBe('baz');
        });
    });

    describe('map - mapping a function over an object', function () {
        it('should produce a new object as a result of applying a function to old object\'s own properties', function () {
            var result = objectHelper.map(function (item) {
                    return typeof item;
                }, testObject);

            // It should keep the original object intact
            expect(result).not.toEqual(testObject);

            expect(result).toEqual({
                something: 'string',
                a: 'string',
                '200': 'number',
                nestedData: 'object'
            });
        });
    });

    describe('mapWith - partially apply map', function () {
        it('should produce a partially applied function to use on the object', function () {
            var func = function (item) {
                return typeof item;
            }

            var result = objectHelper.mapWith(func)(testObject);

            expect(result).toEqual({
                something: 'string',
                a: 'string',
                '200': 'number',
                nestedData: 'object'
            });
        });
    });

    describe('getProperty - get a property from an object', function() {
        it('will retrieve a property from an object safely', function() {
            var object = {name:'simon'};
            expect(objectHelper.getProperty('name')(object)).toBe('simon');
        });

        it('will retrieve a deeply nested property from an object safely', function() {
            var object = {
                employees: {
                    sector7: {
                        name: 'simon'
                    }
                }
            };

            expect(objectHelper.getProperty(['employees', 'sector7', 'name'])(object)).toBe('simon');
        });

        it('will retrieve a deeply nested property from a complex object with arrays safely', function() {
            var object = {
                employees: [
                    {
                        name: 'simon'
                    }
                ]
            };

            expect(objectHelper.getProperty(['employees', 0, 'name'])(object)).toBe('simon');
        });
    });

    describe('transform - transforming object\'s property names', function () {
        it('should create new object changing the property names according to the map', function () {
            var object = {
                    a: 1,
                    b: 2,
                    'deep':{
                        'link': {
                            'name': 'simon',
                            'object': {'foo': 'bazbat'}
                        }
                    }
                },
                map = {
                    blah: 'a',
                    wow: 'b',
                    test: ['deep', 'link', 'name']
                },
                result = objectHelper.transform(map, object);

            expect(result).toEqual({blah: 1, wow: 2, test: 'simon'});
        });
    });

    describe('transform - transforming object\'s property names in the case of a missing param', function () {
        it('should create new object changing the property names according to the map', function () {
            var object = {
                    'deep':{
                        'link': {
                        }
                    }
                },
                map = {
                    wow: 'wow',
                    test: ['deep', 'link', 'name']
                },
                result = objectHelper.transform(map, object);

            expect(result).toEqual({wow: undefined, test: undefined});
        });
    });

    describe('transformWith - partially applied transform', function(){
        it('will apply a transformation map on a given object', function(){
            var object = {
                    'wow': 'wow',
                    'deep':{
                        'link': {
                            'name': 'simon',
                        }
                    }
                },
                map = {
                    wow: 'wow',
                    test: ['deep', 'link', 'name']
                };

            expect(objectHelper.transformWith(map)(object))
                .toEqual({wow: 'wow', test: 'simon'});
        });
    });

    describe('merge - Recursively merge own enumerable properties of the source object(s)', function () {
        it('should merge properties of the second object into the first one', function () {
            var target = {a: 1, b: 2},
                source = {b: 200, c: 400},
                result = objectHelper.merge(target, source);

            expect(result).toEqual({a: 1, b: 200, c: 400});

            // Source stays intact but target gets modified
            expect(result).not.toEqual(source);
            expect(result).toEqual(target);
        });
    });

    describe('mergeWith - Partially applied merge', function () {
        it('should merge properties of the second object into the first one', function () {
            var target = {a: 1, b: 2},
                source = {b: 200, c: 400},
                result = objectHelper.mergeWith(target)(source);

            expect(result).toEqual({a: 1, b: 200, c: 400});

            // Source stays intact but target gets modified
            expect(result).not.toEqual(source);
            expect(result).toEqual(target);
        });
    });

    describe('filter - filtering object properties', function () {
        it('should produce a new object containing only the properties satisfying filtering function', function () {
            var result = objectHelper.filter(function (item) {
                    return !isNaN(item);
                }, testObject);

            // It should keep the original object intact
            expect(result).not.toEqual(testObject);

            expect(result).toEqual({
                '200': 100
            });
        });
    });

    describe('filterBy - partially applied filter', function () {
        it('should produce a new object containing only the properties satisfying filtering function', function () {
            var filterFunction = function (item) {
                return !isNaN(item);
            };

            var result = objectHelper.filterBy(filterFunction)(testObject);

            // It should keep the original object intact
            expect(result).not.toEqual(testObject);

            expect(result).toEqual({
                '200': 100
            });
        });
    });

    describe('find - Iterates over elements of collection, returning the first element predicate returns truthy for.', function () {
        it('should use standard lodash functionality with the arguments reversed', function () {
            var users = [
              { 'user': 'barney',  'age': 36, 'active': true },
              { 'user': 'fred',    'age': 40, 'active': false },
              { 'user': 'pebbles', 'age': 1,  'active': true }
            ];

            var usersUnder30 = function(user) {
              return user.age < 30;
            };

            var result = objectHelper.find(usersUnder30, users);

            expect(result).not.toEqual(users);
            expect(result).toEqual({ 'user': 'pebbles', 'age': 1,  'active': true });
        });
    });

    describe('where - partially applied function that iterates over elements of a collection and checks that data exists', function () {
        var users = [
            { 'user': 'barney',  'age': 36, 'active': true },
            { 'user': 'fred',    'age': 40, 'active': false }
        ];

        it('should return the collection rows given a match', function () {
            var usersThatAre36 = {
                'age': 36
            };

            var result = objectHelper.where(usersThatAre36)(users);
            expect(result).not.toEqual(users);
            expect(result).toEqual([{ 'user': 'barney',  'age': 36, 'active': true }]);
        });

        it('should return an empty array if the collection found no matches', function(){
            var usersThatAre20 = {
                'age': 20
            };

            var result = objectHelper.where(usersThatAre20)(users);
            expect(result).not.toEqual(users);
            expect(result).toEqual([]);
        })
    });

    describe('reduce - Reduces collection to a value which is the accumulated result of running each element in collection through iteratee, where each successive invocation is supplied the return value of the previous', function() {
        it('should apply reduction over an array', function() {
            function sum(total, n) {
                return total + n;
            }

            expect(objectHelper.reduce(sum, 0, [1,2])).toBe(3);
        });

        it('should provide reduction over an object', function() {
            function multiplyEach(result, n, key) {
                return result[key] = n * 3;
            }

            expect(objectHelper.reduce(multiplyEach, 0, {a:1, b:2})).toBe(6);
        });
    });

    describe('reduceWith - partially applied reduce', function() {
        it('should return a function to do work on the given object', function() {
            var summer = function(total, n) {
                    return total + n;
                },
                data = [1,2];

            expect(objectHelper.reduceWith(summer, 0)(data)).toBe(3);
        })
    });

    describe('orderByField - with a given object property, reorder the data descending', function() {
        var data = [
            {
                id: 1, name: 'one'
            }, {
                id: 2, name: 'two'
            }, {
                id: 3, name: 'three'
            }
        ];

        it('will sort a collection by a sorting function - numbers descending', function() {
            expect(objectHelper.orderByField('id')(data)).toEqual([
                {
                    id: 3, name: 'three'
                }, {
                    id: 2, name: 'two'
                },{
                    id: 1, name: 'one'
                },
            ]);
        });

        it('will sort a collection by a sorting function - alpha descending', function() {
            expect(objectHelper.orderByField('name')(data)).toEqual([
                {
                    id: 2, name: 'two'
                },{
                    id: 3, name: 'three'
                }, {
                    id: 1, name: 'one'
                },
            ]);
        });

    });

    describe('takeHead - get the first element of a collection', function() {
        it('will unshift the first item of an array', function() {
            var items = ['a','b','c'];
            expect(objectHelper.takeHead(items)).toBe('a');
        });
    });

    describe('substitute - provide a default value if the source is matched', function() {
        it('will replace a source value with a target on match', function() {
            expect(objectHelper.substitute(undefined, 'wee')(undefined)).toBe('wee');
        });
        it('will replace a source value with a target on match', function() {
            expect(objectHelper.substitute(undefined, 'wee')('sam')).toBe('sam');
        });
    });

    describe('pick - Creates an object composed of the picked object properties', function() {
        var data = {
            offer: 'abc',
            platform: 'zzy',
            i18n: {
                'title': 'baz'
            }
        }

        it('will create a new object based on the keys requested', function() {
            var result = objectHelper.pick(['offer', 'platform'])(data);
            expect(result).not.toEqual(data);
            expect(result).toEqual({offer: 'abc', platform: 'zzy'});
        });

        it('will safely pass undefined filter conditions', function() {
            var result = objectHelper.pick(undefined)(data);
            expect(result).not.toEqual(data);
            expect(result).toEqual({});
        });
    });



    describe('copy - make a copy of an object', function() {
        it('will copy an object', function (){
            var data = {
                    'offer': 'abc',
                    'platform': 'zzy',
                    'i18n': {
                        'title': 'baz'
                    }
                },
                dataCopy = objectHelper.copy(data);

            expect(dataCopy).toEqual(data);
        });
        it('will copy an array', function (){
            var data = ['a','b','c'],
                dataCopy = objectHelper.copy(data);

            expect(dataCopy).toEqual(data);
        });
    });

    describe('measuring object\'s size', function () {
        it('should calculate number of the object\'s own properties', function () {
            expect(objectHelper.length(testObject)).toEqual(4);
        });
    });

    describe('creating property getters', function () {
        var mockObject = {
            parent: {
                child: "exists"
            }
        };
        it('should create a function that applied to an object returns object\'s property value ', function () {
            var getter = objectHelper.getProperty('something');
            expect(getter(testObject)).toEqual('some text');
        });
        it('should return undefined if a property can\'t be found in a given object', function() {
            var getter = objectHelper.getProperty('something');
            expect(getter({})).toBeUndefined;
        });
        it('should return a value if nested property is present', function() {
            var getter = objectHelper.getProperty(['parent','child'])(mockObject);
            expect(getter).toEqual('exists');
        });
        it('should return undefined if nested property is not present', function() {
            var getter = objectHelper.getProperty(['parent','nonexistant'])(mockObject);
            expect(getter).toBeUndefined;
        });
    });
});
