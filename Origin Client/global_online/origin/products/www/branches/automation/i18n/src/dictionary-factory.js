/**
 * Handles the storing the dictionary, pulling in the defaults and
 * merging new dictionaries down.
 * @file dictionary-factory.js
 */
(function () {
    'use strict';

    function LocDictionaryFactory(LocUrlHelper, DEFAULT_DICTIONARY) {

        var prefixWithNamespace = false,
            dictionary = false,
            showMissingStrings = false,
            useFixtureFile = true,
            debugDictionary = false,
            showComponentNames = false;

        /**
         * Find entries that only have a single source - they are isolated
         * @param {Object} obj - the object to find the isolated entries in
         * @return {Array} loners
         */
        function findLoners(obj) {
            var loners = [];
            Object.keys(obj).forEach(function addIsolatedEntry(entry) {
                if (!angular.isObject(obj[entry])) {
                    return;
                }
                // not on a leaf
                if (angular.isUndefined(obj[entry].string)) {
                    loners = loners.concat(findLoners(obj[entry]));
                } else {
                    if (!!obj[entry].source && obj[entry].source.length === 1) {
                        loners.push(obj[entry]);
                    }
                }
            });
            return loners;
        }

        /**
         * Find entries that are isolated (only have a single source)
         * and then log them to the console so that developers know
         * that their entries are not in the CMS, etc.
         */
        function logIsolatedEntries() {
            var loners = findLoners(debugDictionary),
                map = {},
                source;
            for (var i = 0, j = loners.length; i < j; i++) {
                source = loners[i].source[0];
                if (angular.isUndefined(map[source])) {
                    map[source] = [];
                }
                map[source].push(loners[i].namespace + ' ' + loners[i].string);
            }
            // here we can log the isolated entries so we know what is
            // in the cms vs what is local
            console.log('DICTIONARY: isolated entries', map);
        }

        /**
         * Merge the data into the passed in object.  This is called recursively
         * if the value is an object instead of a string (a string is a leaf)
         * but we transform the dictionary leaf node into an object that stores
         * the prefix, the source array and the string instead of just the string.
         * This way we can process the dictionary entries much easier.
         *
         * The data passed in will merge over the data in the object.
         *
         * @param {Object} obj - the object to merge into
         * @param {Object} data - the data to merge into obj
         * @param {String} tag - the source to tag the dictionary entry (e.g. default, cms)
         * @param {String} prefix - a prefix to add to the beginning of a string
         * @param {String} namespace - an optional namespace that we will keep track of
         */
        function tagAndMerge(obj, data, tag, prefix, namespace) {
            var ns = namespace ? namespace + ':' : '';
            Object.keys(data).forEach(function tagAndMergeEntry(key) {
                if (!obj[key]) {
                    obj[key] = {};
                }
                if (angular.isString(data[key])) {
                    if (angular.isUndefined(obj[key].source)) {
                        obj[key].source = [];
                    }
                    obj[key].string = data[key];
                    obj[key].source.push(tag);
                    obj[key].prefix = prefix;
                    obj[key].namespace = ns + key;
                } else if (angular.isObject(data[key])) {
                    tagAndMerge(obj[key], data[key], tag, prefix, key);
                }
            });
        }

        /**
         * Merge in the default dictionary if there
         * isn't currently a dictionary
         */
        function initDefaults() {
            var fixtureDictionary = useFixtureFile ? DEFAULT_DICTIONARY : {};
            if (!dictionary) {
                dictionary = {};
                debugDictionary = {};

                tagAndMerge(
                    debugDictionary,
                    fixtureDictionary || {},
                    'default',
                    LocUrlHelper.getParamValue('defaultsprefix')
                );

                angular.merge(dictionary, DEFAULT_DICTIONARY);

            }
        }

        /**
         * Merge a dictionary down into our current dictionary
         * @param {Object} obj - object in the form {'data': {}, 'source': '', 'prefix': ''}
         *   where data is a set of key value pairs, where the value can be a string or
         *   another key value pair object.  A normal dictionary looks like
         *   {'origin-component-namespace': {'learnmore': 'Learn More'}} where
         *   'origin-component-namespace' is called the namespace, 'learnmore' is
         *   called the key of the string in the dictionary and the actual string is
         *   'Learn More'.  Note that you can also have the dictionary look like
         *   {'learnmore': 'Learn More'}, that is, not have your strings namespaced.
         */
        function addDictionary(obj) {
            tagAndMerge(
                debugDictionary,
                obj.data,
                obj.source || '',
                obj.prefix || ''
            );

            angular.merge(dictionary, obj.data);
        }

        /**
         * Get the string from the dictionary
         * @param {String} key - the key of the string in the dictionary,
         *   this is the english string by default.
         * @param {String} namespace - *optional* the namespace that the translation belongs
         *   to, like mygames, home, etc.
         * @return {String} - the string in the dictionary, or the english string if its not found
         */
        function getDictionaryStr(key, namespace) {
            var entry = namespace ?
            debugDictionary[namespace] && debugDictionary[namespace][key] :
                debugDictionary[key];
            if (!entry) {
                if(showMissingStrings) {
                    return '###MISSINGSTRING:['+ namespace+']:' + key +'###';
                } else {
                    throw 'key ' + key + ' was not found in namespace ' + namespace;
                }
            } else {
                if (prefixWithNamespace || showComponentNames) {
                    return entry.prefix + entry.namespace + ' ' + entry.string;
                } else {
                    return entry.prefix + entry.string;
                }
            }
        }

        /**
         * Get the string from the dictionary
         * @param {String} namespace - *optional* the namespace that the translation belongs
         *   to, like mygames, home, etc.
         * @return {Object}
         */
        function getDefaults(namespace) {
            if (namespace && dictionary[namespace]) {
                return dictionary[namespace];
            } else {
                return null;
            }
        }

        /**
         * Set prefixWithNamespace
         * @param {Boolean} val
         */
        function setPrefixWithNamespace(val) {
            prefixWithNamespace = val;
        }

        /**
         * returns whether the dictionary is empty or not
         * @return {boolean}
         */
        function isLoaded() {
            return !!dictionary;
        }

        /**
         * Retrieves the default for the key name spaced in the value.
         * @param {String} key The key for the default
         * @param {String} namespace
         * @returns {String}
         */
        function getDefaultValue(key, namespace) {
            var dictionaryContent;
            try {
                dictionaryContent = getDictionaryStr(key, namespace);
            } catch (e) {
                console.log('DICTIONARY: Missing namespace ' + namespace + ' key ' + key);
            }
            return dictionaryContent;
        }


        function processDictionaryContent(data) {
                addDictionary({
                    'data': data.data,
                    'source': 'remote',
                    'prefix': ''
                });

                logIsolatedEntries();
        }

        /**
         * Need to initialize the loc factory, which makes
         * the request to get the dictionary.
         * @param {string} originDefaults - origin defaults loaded by Kernel
         */
        function init(originDefaults, useFixtureFileOverride, showMissingStringsOverride, showComponentNamesOverride) {
            useFixtureFile = useFixtureFileOverride;
            showMissingStrings = showMissingStringsOverride;
            showComponentNames = showComponentNamesOverride;

            initDefaults();

            processDictionaryContent(originDefaults);
        }

        return {
            init: init,
            getDefaultValue: getDefaultValue,
            getDefaults: getDefaults,
            loaded: isLoaded,
            setPrefixWithNamespace: setPrefixWithNamespace,
            getDictionaryStr: getDictionaryStr,
            isLoaded: isLoaded,
            addDictionary: addDictionary,
            initDefaults: initDefaults,
            logIsolatedEntries: logIsolatedEntries
        };
    }

    /**
     * @ngdoc service
     * @name origin-i18n.factories.LocDictionaryFactory
     *
     * @description
     *
     * Handles the storing the dictionary, pulling in the defaults and
     * merging new dictionaries down.
     */
    angular.module('origin-i18n')
        .factory('LocDictionaryFactory', LocDictionaryFactory);
}());
