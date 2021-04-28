/**
 * @file stack.js
 */
(function() {
    'use strict';

    function StackFactory() {

        /**
        * A stack class
        * @class Stack
        */
        function Stack() {
            this.elements = [];
        }

        /**
        * Pop an element off the stack
        * @return - top element off the stack
        * @method pop
        */
        Stack.prototype.pop = function() {
            return this.elements.pop();
        };

        /**
        * Push an element onto the stack
        * @param - whatever to push onto the stack
        * @method push
        */
        Stack.prototype.push = function(obj) {
            this.elements.push(obj);
        };

        /**
        * Peak at the last element on the stack
        * @return - the top element off the stack
        * @method peek
        */
        Stack.prototype.peek = function() {
            return this.elements.length ? this.elements[this.elements.length-1] : undefined;
        };

        /**
        * Remove the item if the match callback is true
        * @method remove
        */
        Stack.prototype.remove = function(match) {
            for (var i=0, j=this.elements.length; i<j; i++) {
                if (match(this.elements[i])) {
                    this.elements.splice(i, 1);
                    break;
                }
            }
        };

        /**
        * Look for item in the stack
        * @return - Boolean if we found a matching item
        * @method remove
        */
        Stack.prototype.contains = function(match) {
            for (var i=0, j=this.elements.length; i<j; i++) {
                if (match(this.elements[i])) {
                    return true;
                }
            }
            return false;
        };

        /**
        * Get the length of the stack
        * @return {Number}
        * @method length
        */
        Stack.prototype.length = function() {
            return this.elements.length;
        };

        /**
        * Make a new stack
        * @return {Stack}
        * @method make
        */
        function make() {
            return new Stack();
        }

        return {
            make: make
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ObservableFactory
     * @description
     *
     * Helpers for manipulating objects
     */
    angular.module('origin-components')
        .factory('StackFactory', StackFactory);

}());