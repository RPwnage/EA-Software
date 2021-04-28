/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define(['core/logger'], function(logger) {

    var TYPES = {
        'undefined': 'undefined',
        'number': 'number',
        'boolean': 'boolean',
        'string': 'string',
        '[object Function]': 'function',
        '[object RegExp]': 'regexp',
        '[object Array]': 'array',
        '[object Date]': 'date',
        '[object Error]': 'error'
    };

    var osName = 'UnknownOS';

    function setOS() {
        if (window.navigator.appVersion.indexOf('Win') !== -1) {
            osName = 'PCWIN';
        } else if (window.navigator.appVersion.indexOf('Mac') !== -1) {
            osName = 'MAC';
        } else if (window.navigator.appVersion.indexOf('Linux') !== -1) {
            osName = 'Linux';
        }
    }

    /**
     *
     * @return {String} the type
     * @method type
     */
    function type(o) {
        return TYPES[typeof o] || TYPES[Object.prototype.toString.call(o)] || (o ? 'object' : 'null');
    }

    /**
     *
     * @return {Boolean}
     * @method isObject
     */
    function isObject(o) {
        var t = type(o);
        return (o && (t === 'object')) || false;
    }

    /**
     * Mix the data together
     * @return {void}
     */
    function mix(oldData, newData) {
        for (var key in newData) {
            if (!newData.hasOwnProperty(key)) {
                continue;
            }
            if (isObject(oldData[key]) && isObject(newData[key])) {
                mix(oldData[key], newData[key]);
            } else {
                oldData[key] = newData[key];
            }
        }
    }

    /**
     * Check if the chain of arguments are defined in the object
     * @param {object} obj - the object to check
     * @param {Array} chain - the chain of properties to check in the object
     * @return {Boolean}
     */
    function isChainDefined(obj, chain) {
        var tobj = obj;
        for (var i = 0, j = chain.length; i < j; i++) {
            if (typeof(tobj[chain[i]]) !== 'undefined') {
                tobj = tobj[chain[i]];
            } else {
                return false;
            }
        }

        return true;
    }

    /**
     * returns the object associated with the defined chain, null if undefined
     * @param {object} obj - the object to check
     * @param {Array} chain - the chain of properties to check in the object
     * @return {Object}
     */
    function getProperty(obj, chain) {
        if (!obj) {
            return null;
        }

        var tobj = obj;
        for (var i = 0, j = chain.length; i < j; i++) {
            if (typeof(tobj[chain[i]]) !== 'undefined') {
                tobj = tobj[chain[i]];
            } else {
                return null;
            }
        }
        return tobj;
    }

    /**
     * @class Communicator
     */
    function Communicator() {
        this.map = {};
    }

    /**
     * Determines if the callback is already in the event map
     * @param {Array} arr - array to check
     * @param {Function} fn - the callback function
     * @return {Boolean} determines if it is in the arr or not
     * @method inCallbacks
     */
    Communicator.prototype.inCallbacks = function(arr, fn) {
        for (var i = 0, j = arr.length; i < j; i++) {
            if (arr[i].callback === fn) {
                return true;
            }
        }
        return false;
    };

    /**
     * Execute all of the callbacks passed in with the arguments provided
     * @param {Array} callbacks - array of callback objects
     * @param {Array} args - array of arguments to pass to the callbacks
     * @return {void}
     * @method executeCallbacks
     */
    Communicator.prototype.executeCallbacks = function(callbacks, args) {
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
                //                console.error('EXCEPTION in executeCallbacks: ', e.message, e.stack);
                logger.error(e.message + e.stack);
            }
        }
    };

    /**
     * Get the callbacks that are not applied using once,
     * that is callbacks that will be fired again the next
     * time the event is fired.
     * @param {Array} callbacks - array of callbacks objects
     * @return {Array} callbacks - array of callback objects
     * @method getPersistentCallbacks
     */
    Communicator.prototype.getPersistentCallbacks = function(callbacks) {
        return callbacks.filter(function(item) {
            return !item.once;
        });
    };

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
        if (typeof eventName === 'undefined') {
            throw new Error('Communicator.on: eventName is undefined');
        }
        if (!this.map[eventName]) {
            this.map[eventName] = [];
        }
        if (!this.inCallbacks(this.map[eventName], fn)) {
            this.map[eventName].push({
                'callback': fn,
                'context': context || window,
                'once': false
            });
        }
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
        if (typeof eventName === 'undefined') {
            throw new Error('Communicator.once: eventName is undefined');
        }
        if (!this.map[eventName]) {
            this.map[eventName] = [];
        }
        if (!this.inCallbacks(this.map[eventName], fn)) {
            this.map[eventName].push({
                'callback': fn,
                'context': context || window,
                'once': true
            });
        }
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
            callbackContext = context || window;
        if (callbacks) {
            for (var i = 0, j = callbacks.length; i < j; i++) {
                if (callbacks[i].callback === fn && callbacks[i].context === callbackContext) {
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
            this.executeCallbacks(callbacks, args);
            this.map[eventName] = this.getPersistentCallbacks(callbacks);
        }
    };

    /**
     * returns true if the bridge available, we just choose any proxy object
     */
    function bridgeAvailable() {
        return (typeof OriginGamesManager !== 'undefined');
    }

    setOS();

    return {
        Communicator: Communicator,

        /** Tells if the bridge is available or not
        @method
        @returns {boolean}
        */
        bridgeAvailable: bridgeAvailable,

        /**
         * returns the object associated with the defined chain, null if undefined
         * @param {object} obj - the object to check
         * @param {Array} chain - the chain of properties to check in the object
         * @return {Object}
         */
        getProperty: getProperty,

        /**
         * Check if the chain of arguments are defined in the object
         * @method
         * @param {object} obj - the object to check
         * @param {Array} chain - the chain of properties to check in the object
         * @return {Boolean}
         */
        isChainDefined: isChainDefined,

        /**
         * Mix the data together
         * @return {void}
         * @method mix
         */
        mix: mix,

        /**
         * return the OS
         * @return {String}
         * @method os
         */
        os: function() {
            return osName;
        }
    };
});