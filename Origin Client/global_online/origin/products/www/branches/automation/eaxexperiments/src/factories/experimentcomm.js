(function() {
    'use strict';

    function ExperimentCommFactory() {

        /**
        * Determine if the events are equal
        * @param {object} event1
        * @param {object} event2
        * @return {Boolean}
        * @method eventsEqual
        */
        function eventsEqual(event1, event2) {
            return (event1.callback === event2.callback && event1.context === event2.context);
        }

        /**
         * Determines if the event is already in the map
         * @param {Array} arr - array to check
         * @param {object} eventObject - the event information object that we want to check
         * @return {Boolean} determines if it is in the arr or not
         * @method inCallbacks
         */
        function inCallbacks(arr, eventObject) {
            for (var i = 0, j = arr.length; i < j; i++) {
                if (eventsEqual(arr[i], eventObject)) {
                    return true;
                }
            }
            return false;
        }

        //use console to output error instead of logger so that we don't create a circular dependency with telemetry
        function outputError(e) {
            if (typeof console.error === 'undefined') {
                console.log(e.message + e.stack);
            } else {
                console.error(e.message + e.stack);
            }
        }

        /**
         * Execute all of the callbacks passed in with the arguments provided
         * @param {Array} callbacks - array of callback objects
         * @param {Array} args - array of arguments to pass to the callbacks
         * @return {void}
         * @method executeCallbacks
         */
        function executeCallbacks(callbacks, args) {
            // execute all of the callbacks, a callback may mutate
            // the state of the array,
            var callbacksCopy = callbacks.slice();
            for (var i = 0, len = callbacksCopy.length; i < len; i++) {
                var callbackObj = callbacksCopy[i];
                //since the callback could cause exception, we want to make sure we trap it
                //and not have it stop the rest of the callbacks from executing
                try {
                    callbackObj.callback.apply(callbackObj.context, args);
                } catch (e) {
                    outputError(e);
                }
            }
        }

        /**
         * Get the callbacks that are not applied using once,
         * that is callbacks that will be fired again the next
         * time the event is fired.
         * @param {Array} callbacks - array of callbacks objects
         * @return {Array} callbacks - array of callback objects
         * @method getPersistentCallbacks
         */
        function getPersistentCallbacks(callbacks) {
            return callbacks.filter(function(item) {
                return !item.once;
            });
        }

        /**
         * Add an event to the event map
         * @param {object} map - the event map
         * @param {string} eventName - the event name
         * @param {object} eventObject - the event information object
         * @return {void}
         * @method addEvent
         */
        function addEvent(map, eventName, eventObject) {
            if (typeof eventName === 'undefined') {
                throw new Error('Communicator._addEvent: eventName is undefined');
            }
            if (!map[eventName]) {
                map[eventName] = [];
            }
            if (!inCallbacks(map[eventName], eventObject)) {
                map[eventName].push(eventObject);
            }
        }

        /**
         * Create an object with handlers to detach and reattach the event
         * @param {object} obj - the communicator instance
         * @param {string} eventName - the event name to create handlers for
         * @param {object} eventOjbect - the event info object
         * @return {object}
         * @method createHandlers
         */
        function createHandlers(obj, eventName, eventObject) {
            return {
                detach: function() {
                    obj.off.call(obj, eventName, eventObject.callback, eventObject.context);
                },
                attach: function() {
                    addEvent(obj.map, eventName, eventObject);
                }
            };
        }

        /**
        * @param {Function} fn - the callback function
        * @param {object} context - the event context
        * @param {Boolean} once - if the event should be fired more than once
        * @return {object} the event object
        * @method createEvent
        */
        function createEvent(fn, context, once) {
            var eventContext = context || window;
            return {
                'callback': fn,
                'context': eventContext,
                'once': once
            };
        }

        /**
         * @class Communicator
         */
        function Communicator() {
            this.map = {};
        }

        /**
         * Subscribe to an event so when the event is fired, the callback
         * is executed in the context passed
         * @param {string} eventName - the event name
         * @param {Function} fn - the callback
         * @param {object} context - the context (what this refers to)
         * @return {void}
         * @method on
         */
        Communicator.prototype.on = function(eventName, fn, context) {
            var eventObject = createEvent(fn, context, false);
            addEvent(this.map, eventName, eventObject);
            return createHandlers(this, eventName, eventObject);
        };

        /**
         * Subscribe to an event for the first time that it is fired. When
         * the event is fired for the first time, the callback is fired
         * in the context passed.  Note that this callback is only fired ONCE
         * hence once.
         * @param {string} eventName - the event name
         * @param {Function} fn - the callback
         * @param {object} context - the context (what this refers to)
         * @return {void}
         * @method once
         */
        Communicator.prototype.once = function(eventName, fn, context) {
            var eventObject = createEvent(fn, context, true);
            addEvent(this.map, eventName, eventObject);
            return createHandlers(this, eventName, eventObject);
        };

        /**
         * Remove event subscription
         * @param {string} eventName - the event name
         * @param {Function} fn - the callback
         * @return {void}
         * @method off
         */
        Communicator.prototype.off = function(eventName, fn, context) {
            if (typeof eventName === 'undefined') {
                throw new Error('Communicator.fire: eventName is undefined');
            }
            var callbacks = this.map[eventName],
                eventObject = createEvent(fn, context);
            if (callbacks) {
                for (var i = 0, j = callbacks.length; i < j; i++) {
                    if (eventsEqual(callbacks[i], eventObject)) {
                        this.map[eventName].splice(i, 1);
                        break;
                    }
                }
            }
        };

        /**
         * Fire the given event, passing in all arguments that are assed
         * @param {string} eventName - the event name
         * @return {void}
         * @method fire
         */
        Communicator.prototype.fire = function(eventName) {
            if (typeof eventName === 'undefined') {
                throw new Error('Communicator.fire: eventName is undefined');
            }
            if (this.map[eventName]) {
                var callbacks = this.map[eventName],
                    args = [].slice.call(arguments, 1);
                executeCallbacks(callbacks, args);
                this.map[eventName] = getPersistentCallbacks(callbacks);
            }
        };

        return  {
            Communicator: Communicator
        };
    }

    /**
     * @ngdoc service
     * @name eax-experiments.factories.ExperimentCommFactory
     * @description
     *
     * Experiment Factory
     */
    angular.module('eax-experiments')
        .factory('ExperimentCommFactory', ExperimentCommFactory);
}());
