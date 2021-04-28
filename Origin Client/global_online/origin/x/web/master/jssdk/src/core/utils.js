/*jshint strict: false */
/*jshint unused: false */

define(['core/Communicator'], function(Communicator) {
    /**
     * utility functions
     * @module module:utils
     * @memberof module:Origin
     */
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
     */
    function type(o) {
        return TYPES[typeof o] || TYPES[Object.prototype.toString.call(o)] || (o ? 'object' : 'null');
    }

    /**
     *
     * @return {Boolean}
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

    function replaceInObject(data, replacementObject) {
        for (var key in data) {
            if (!data.hasOwnProperty(key)) {
                continue;
            }
            if (typeof data[key] === 'object') {
                replaceInObject(data[key], replacementObject);
            } else {
                for (var prop in replacementObject) {
                    if (replacementObject.hasOwnProperty(prop) && (typeof(data[key]) === 'string')) {
                        data[key] = data[key].replace(prop, replacementObject[prop]);
                    }
                }
            }
        }
    }

    function normalizeOverrides(overrides) {
        //the prod and live versions of env and cmsstage respectively are represented in the apis by omission of any env or cmsstage in the path
        //so we blank them out here
        if (overrides.env.toLowerCase() === 'production') {
            overrides.env = '';
        }

        if (overrides.cmsstage.toLowerCase() === 'live') {
            overrides.cmsstage = '';
        }

        return overrides;
    }



    function replaceTemplatedValuesInConfig(configObject) {
        var env = '',
            version = '',
            cmsstage = '';

        if (configObject.overrides) {

            //for prod and live overrides we set them to blank as the urls represent prod/live by omitting env
            normalizeOverrides(configObject.overrides);
            if (configObject.overrides.env) {
                env = configObject.overrides.env + '.';
            }

            if (configObject.overrides.version) {
                version = configObject.overrides.version + '.';
            }

            if (configObject.overrides.cmsstage) {
                cmsstage = configObject.overrides.cmsstage + '.';
            }
        }
        //first we replace the override information in the hostname section
        replaceInObject(configObject.hostname, {
            '{base}': configObject.hostname.base,
            '{env}': env,
            '{version}': version,
            '{cmsstage}': cmsstage
        });

        //then lets replace the tokens with the actual hosts in the urls
        replaceInObject(configObject.urls, {
            '{baseapi}': configObject.hostname.baseapi,
            '{basedata}': configObject.hostname.basedata,
        });
    }

    setOS();

    return /**@ lends Origin.module:utils */ {
        Communicator: Communicator,

        /**
         * returns the object associated with the defined chain, null if undefined
         * @method
         * @static
         * @param {object} obj - the object to check
         * @param {Array} chain - the chain of properties to check in the object
         * @return {Object}
         */
        getProperty: getProperty,

        /**
         * Check if the chain of arguments are defined in the object
         * @method
         * @static
         * @param {object} obj - the object to check
         * @param {Array} chain - the chain of properties to check in the object
         * @return {Boolean}
         */
        isChainDefined: isChainDefined,

        /**
         * Mix the data together
         * @method
         * @static
         * @param {object} obj1 - the destination object to mix to
         * @param {object} obj2 - the object to mix from
         */
        mix: mix,
        /**
         * Replaces tokens in config files replaces tokens such as env, cmsstage, version in an object
         * @method
         * @static
         * @param {object} configObject The config object
         */
        replaceTemplatedValuesInConfig: replaceTemplatedValuesInConfig,
        /**
         * return the OS
         * @method
         * @static
         * @return {String}
         */
        os: function() {
            return osName;
        }
    };
});