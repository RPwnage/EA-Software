/**
 * Jasmine functional test
 */

'use strict';

describe('Stack Factory', function() {
    var stackFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
    });

    beforeEach(inject(function(_StackFactory_) {
        stackFactory = _StackFactory_;
    }));

    describe('push', function() {
        it('should add elements to the stack', function(){
            var stack = stackFactory.make();
            stack.push('1');
            stack.push('2');
            stack.push('3');

            expect(stack.elements).toEqual(['1','2','3']);
        });

        it('should add elements to the stack regardless of what is popped', function(){
            var stack = stackFactory.make(),
                firstPop, secondPop;

            stack.push('1');
            stack.push('2');

            firstPop = stack.pop();

            stack.push('3');
            stack.push('4');

            secondPop = stack.pop();
            stack.push('5');

            expect(stack.elements).toEqual(['1','3','5']);
            expect(firstPop).toEqual('2');
            expect(secondPop).toEqual('4');
        });
    });

    describe('pop', function() {
        it('should pop the last element from the stack', function(){
            var stack = stackFactory.make(),
                popped;

            stack.push('1');
            stack.push('2');
            stack.push('3');

            popped = stack.pop();

            expect(popped).toEqual('3');
            expect(stack.elements).toEqual(['1','2']);
        });

        it('should return undefined of there are no elements', function(){
            var stack = stackFactory.make(),
                popped = stack.pop();
            expect(popped).toEqual(undefined);
        });

    });

    describe('peek', function() {
        it('should show the last element and not remove it', function(){
            var stack = stackFactory.make(),
                peeked;

            stack.push('1');
            stack.push('2');
            stack.push('3');

            peeked = stack.peek();

            expect(peeked).toEqual('3');
            expect(stack.elements).toEqual(['1','2', '3']);
        });

        it('should return undefined of there are no elements', function(){
            var stack = stackFactory.make(),
                peeked = stack.peek();

            expect(peeked).toEqual(undefined);
        });
    });

    describe('length', function() {
        it('should reflect the length', function(){
            var stack = stackFactory.make();

            stack.push('1');
            stack.push('2');
            stack.push('3');
            stack.push('4');
            stack.pop();
            stack.push('5');
            stack.pop();
            stack.push('6');
            stack.push('7');

            expect(stack.length()).toEqual(5);

        });

        it('should reflect no length ', function(){
            var stack = stackFactory.make();
            expect(stack.length()).toEqual(0);
        });
    });

});