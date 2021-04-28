/**
 * Jasmine functional test
 */

'use strict';

describe('Function Helper Factory', function() {
    var functionHelper;

    function sum(a, b) {
        return a + b;
    }

    function getId(id) {
        if(id === 'foo') {
            return 20;
        } else {
            return 5;
        }
    }

    function addOne(a) {
        return a + 1;
    }

    function boolTrue() {
        return true;
    }

    function boolFalse() {
        return false;
    }

    function truthy() {
        return 'defined';
    }

    function falsy() {
        return undefined;
    }

    function reflectInput(input) {
        return input;
    }

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){}
                };
            });
        });
    });

    beforeEach(inject(function(_FunctionHelperFactory_){
            functionHelper = _FunctionHelperFactory_;
    }));

    describe('deferred - creates a deferred function execution context.', function() {
        it('will create a callback to delay execution of the passed function', function() {
            var deferredFunc = functionHelper.deferred(addOne, [1]);
            expect(deferredFunc()).toBe(2);
        });
    });

    describe('executeWith - apply() over one or multiple passed functions with the supplied parameters', function() {
        it('will execute a function with multiple params provided in an array', function() {
            expect(functionHelper.executeWith(1, 2)(sum)).toBe(3);
        });

        it('will execute multiple functions and one intitializer param', function() {
            expect(functionHelper.executeWith('foo')([getId, addOne])).toBe(21);
        });

        it('will pass through non-function types', function() {
            expect(functionHelper.executeWith(1, 2)('astring')).toBe('astring');
        });
    });

    describe('pipe - creates a pipeline which passes result of previous execution to the next function', function() {
        it('will pipeline a single argument', function(){
            var result = functionHelper.pipe([getId, addOne])('foo');
            expect(result).toBe(21);
        });
    });

    describe('chain - create a deferred execution promise chain', function() {
        it('will create a function with an immediately resovled promise context. it does not unwrap the promise.', function(done) {
            var result = functionHelper.chain([getId, addOne])('foo');
            result.then(function(data) {
                expect(data).toBe(21);
                done();
            })
            .catch(function(err){
                fail(err);
                done();
            });
        });
    });

    describe('or - Executes array of filter functions and performs the logical \'or\' on the results.', function() {
        it('will test for the first boolean true function return', function() {
            expect(functionHelper.or([boolTrue, boolFalse])()).toBe(true);
        });

        it('will test for the first boolean true function return', function() {
            expect(functionHelper.or([boolFalse, truthy])()).toBe(true);
        });

        it('will test for false results', function() {
            expect(functionHelper.or([boolFalse, boolFalse])()).toBe(false);
        });

        it('will test for falsy results', function() {
            expect(functionHelper.or([falsy, falsy])()).toBe(false);
        });

        it('will accept late input', function() {
            expect(functionHelper.or([reflectInput])('a string should be truthy, right?')).toBe(true);
        });
    });

    describe('and - Executes array of filter functions and performs the logical \'and\' on the results.', function() {
        it('will test for the first boolean true function return', function() {
            expect(functionHelper.and([boolTrue, boolFalse])()).toBe(false);
        });

        it('will test for the first boolean true function return', function() {
            expect(functionHelper.and([boolTrue, truthy])()).toBe(true);
        });

        it('will test for false results', function() {
            expect(functionHelper.and([boolFalse])()).toBe(false);
        });

        it('will test for falsy results', function() {
            expect(functionHelper.and([falsy])()).toBe(false);
        });

        it('will accept late input', function() {
            expect(functionHelper.and([reflectInput])(null)).toBe(false);
        });
    });
});