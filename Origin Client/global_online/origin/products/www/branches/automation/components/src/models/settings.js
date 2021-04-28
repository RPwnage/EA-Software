/**
 * @file common/settings.js
 */
(function() {
    'use strict';

    /**
     * Augment serializer object with capabilities to add/remove string prefix to serialized strings
     * @param {string} prefix - string prefix to add to the result of serialization
     * @param {object} serializer - serializer object to augment
     * @return {object}
     */
    function createSerializerWithPrefix(prefix, serializer) {
        var PREFIX_SEPARATOR = '|';

        return {
            deserialize: function (string) {
                return serializer.deserialize(string.split(PREFIX_SEPARATOR)[1]);
            },
            serialize: function (object) {
                return prefix + PREFIX_SEPARATOR + serializer.serialize(object);
            }
        };
    }

    function dummySerializer() {
        return {
            deserialize: function () {
                return {};
            },
            serialize: function () {
                return '';
            }
        };
    }

    function SettingsFactory(ObjectSerializerFactory, ArraySerializerFactory, ComponentsLogFactory, AuthFactory, ExperimentFactory) {
        var myEvents = new Origin.utils.Communicator(),
            settingsData = {},
            settingsReady = false,
            HIDDENGAMES_VER = '1.0',
            savedCDstr = '',

            hiddenGamesSerializer = createSerializerWithPrefix(HIDDENGAMES_VER, ArraySerializerFactory),
            serializers = {
                'GENERAL': ObjectSerializerFactory,
                'NOTIFICATION': ObjectSerializerFactory,
                'FAVORITEGAMES': ArraySerializerFactory,
                'HIDDENGAMES': hiddenGamesSerializer
            },
            defaultSerializer = dummySerializer();

        /**
         * Picks proper serializer for a category
         * @return {Object}
         */
        function getSerializer(category) {
            if (serializers[category]) {
                return serializers[category];
            } else {
                return defaultSerializer;
            }
        }

        /**
         * Clear settings cache
         * @return {void}
         */
        function clearCache() {
            settingsReady = false;
            settingsData = {};
        }

        /**
         * Get the latest state of the settings in a promise.
         * If the user is logged out - wait until they log in and the factory reads their settings.
         * @return {Promise}
         */
        function getSettings() {
            var promise = new Promise(function (resolve) {
                if (settingsReady) {
                    resolve(settingsData);
                } else {
                    myEvents.once('settingsRetrieved', function() {
                        resolve(settingsData);
                    });
                }
            });

            return promise;
        }

        function clearSavedCDstr() {
            savedCDstr = '';
        }

        function setExperimentCustomDimension() {
            var cdStr,
                combinedStr,
                mergedchanged = false,  //saved off cd merged with one retrieved from atom/privacysettings
                expremoved = false;     //experiment removed because no longer active

            cdStr = _.get(settingsData, ['GENERAL', 'experiments']) || '';

            //if savedCDstr is non-empty, then we already had an inExperiment call before the settings got loaded
            //so let's combine that with what's retrieved from the server
            //but also replace previous variant if savedCDstr contains latest variant
            // e.g. previously was control but variant % changed so user is now in variantA, so we need to remove control and retain variantA
            combinedStr = ExperimentFactory.mixAndReplaceCustomDimension(cdStr, savedCDstr);
            mergedchanged = (combinedStr !== cdStr);  //we were bucketed into a new experiment before atom/privacySetting loaded
            clearSavedCDstr();

            if (combinedStr) {
                expremoved = ExperimentFactory.setUserCustomDimension(combinedStr);

                //if it changed (e.g. experiments removed because no longer active), we need to
                //update the settings on the server
                if (mergedchanged || expremoved) {
                    //get the latest custom dimension string
                    cdStr = ExperimentFactory.getUserCustomDimension();
                    updateCustomDimension(cdStr);
                }
            }
        }

        /**
         * Parse, deserialize and cache settings that come from the server.
         * @param {object} response - server response
         * @return {object}
         */
        function deserializeSettingsData(response) {
            settingsData = {};

            if (typeof response.privacySettings === 'undefined') {
                throw new Error('[SettingsFactory:deserializeSettingsData] response.privacySettings.privacySetting is not defined');
            }

            if (response.privacySettings !== null) {
                var privacySetting = response.privacySettings.privacySetting;

                if (privacySetting.constructor === Array) {
                    angular.forEach(privacySetting, cacheSettingsCategory);
                } else if (typeof privacySetting === 'object') {
                    cacheSettingsCategory(privacySetting);
                } else {
                    throw new Error('[SettingsFactory:deserializeSettingsData] Unexpected response type');
                }
            }

            //as a precaution, update the custom dimension string in the expSDK BEFORE firing settingsRetrieved
            setExperimentCustomDimension();

            myEvents.fire('settingsRetrieved');

            settingsReady = true;

            return settingsData;
        }

        /**
         * Handles errors that happen when retreiving user's settings from the server
         * @param {object} error - error object
         * @return {void}
         */
        function processSettingsError(error) {
            ComponentsLogFactory.error('[SettingsFactory:processSettingsError]', error);
            myEvents.fire('initialSettingsRetrieved');
            settingsReady = true;
        }

        /**
         * Get user's settings from the server if hasn't done that already
         * @return {void}
         */
        function readSettingsFromServer() {
            if (!settingsReady) {
                Origin.settings.retrieveUserSettings()
                    .then(deserializeSettingsData)
                    .catch(processSettingsError);

            } else {
                ComponentsLogFactory.warn('[SETTINGSFACTORY] already retrieved settings, but requesting again');
            }
        }

        /**
         * Returns true if the settings have been retrieved from the server and false otherwise
         * @return {boolean}
         */
        function isDataReady() {
            return settingsReady;
        }

        /**
         * Deserialize and cache single category of settings.
         * @param {object|Array} settingsPackage - portion of server response
         * @return {void}
         */
        function cacheSettingsCategory(settingsPackage) {
            var category = settingsPackage.category,
                serializer = getSerializer(category),
                payload = settingsPackage.payload;

            settingsData[category] = serializer.deserialize(payload);
        }

        /**
         * Updates settings category on the server. Settings data is taken from the cache
         * @param {string} category - category to be sent to the server
         * @return {void}
         */
        function saveSettings(category) {
            var serializer = getSerializer(category),
                payload = serializer.serialize(settingsData[category]);

            Origin.settings.postUserSettings(category, payload);
        }

        /**
         * Pipline filter that returns a single category from the incoming settings object.
         * @param {string} category - category to lock the filter on
         * @param {mixed} defaultValue - (optional) value to return if the category is not found
         * @return {function}
         */
        function findCategory(category, defaultValue) {
            return function (settings) {
                return settings[category] || defaultValue;
            };
        }

        /**
         * Generates functions that replace entire category with a new value and send updates to the server
         * @param {string} category - category to lock the filter on
         * @return {function}
         */
        function updateCategory(category) {
            return function (newValue) {
                settingsData[category] = newValue;
                saveSettings(category);
            };
        }

        /**
         * Generates functions that change a single value in a category and send updates to the server
         * @param {string} category - category to lock the filter on
         * @return {function}
         */
        function createSetterForCategory(category) {
            return function setGeneralSetting(key, value) {
                if (!settingsData[category]) {
                    settingsData[category] = {};
                }

                settingsData[category][key] = value;
                saveSettings(category);
            };
        }

        function updateCustomDimension(customDimensionStr) {
            ComponentsLogFactory.log('SETTINGS: updating experiment customDimension:', customDimensionStr);
            //save the custom dimension out to the settings
            createSetterForCategory('GENERAL')('experiments', customDimensionStr);
        }

        function onUpdateCDsettingsReady() {
            myEvents.off('initialSettingsRetrieved', onUpdateCDsettingsReady); //failure of retrieval of settings
            updateCustomDimension(savedCDstr);
            clearSavedCDstr();
        }

        function onUpdateCustomDimension(customDimensionStr) {
            //make sure settings is already loaded
            if (settingsReady) {
                updateCustomDimension(customDimensionStr);
            } else {
                savedCDstr = customDimensionStr;

                //we'll only connect to listen for failrue to retrieve settings
                //we won't connect to listen for settingsRetrieved which is fired when there's a successful retrieval of settings
                //because we call setExperimentCustomDimension to initialize the cdStr in the experiment sdk
                //but before we do that we'll combine savedCDstr with what's retrieved from the server
                //otherwise if we hook it up to listen for settingsRetrieved and call onUpdateCDsettingReady, it will end up
                //overwriting what's retrieved from the server
                myEvents.on('initialSettingsRetrieved', onUpdateCDsettingsReady); //failure of retrieval of settings
            }
        }

        function onAuthChanged(loginObj) {
            if (loginObj) {
                if (loginObj.isLoggedIn) {
                    readSettingsFromServer();
                    ExperimentFactory.events.on('experiment:updateCustomDimension', onUpdateCustomDimension);
                } else {
                    clearCache();
                    ExperimentFactory.events.off('experiment:updateCustomDimension', onUpdateCustomDimension);
                }
            }
        }

        function init(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                readSettingsFromServer ();
                ExperimentFactory.events.on('experiment:updateCustomDimension', onUpdateCustomDimension);
            }

            AuthFactory.events.on('myauth:change', onAuthChanged);
        }

        function handleAuthReadyError(error) {
            ComponentsLogFactory.error('SETTINGSFACTORY - authReady error:', error);
        }

        AuthFactory.waitForAuthReady()
            .then(init)
            .catch(handleAuthReadyError);

        return {
            /**
             * returns true if the settings data is in the loacl cache
             * @return {boolean}
             */
            isDataReady: isDataReady,
            /**
             * gets the favorite games settings
             * @return {string[]} an array of favorite offerIds
             */
            getFavoriteGames: function () {
                return getSettings().then(findCategory('FAVORITEGAMES', []));
            },
            /**
             * updates the favorite games settings
             * @param {string[]} list an array of favorite offerIds
             */
            updateFavoriteGames: updateCategory('FAVORITEGAMES'),
            /**
             * gets the hidden games settings
             * @return {string[]} an array of hidden offerIds
             */
            getHiddenGames: function () {
                return getSettings().then(findCategory('HIDDENGAMES', []));
            },
            /**
             * updates the hidden games settings
             * @param {string[]} offerIds an array of hidden offerIds
             */
            updateHiddenGames: function (offerIds) {
                updateCategory('HIDDENGAMES')(offerIds);
            },
            /**
             * gets the hidden games settings
             * @return {object} general settings object
             */
            getGeneralSettings: function () {
                return getSettings().then(findCategory('GENERAL', {}));
            },
            /**
             * updates a general setting
             * @param {string} key key for the general setting
             * @param {object} value value for the general setting
             */
            setGeneralSetting: createSetterForCategory('GENERAL'),

            /**
             * gets the hidden games settings
             * @return {object} general settings object
             */
            getNotificationSettings: function () {
                return getSettings().then(findCategory('NOTIFICATION', {}));
            },
            /**
             * updates a general setting
             * @param {string} key key for the general setting
             * @param {object} value value for the general setting
             */
            setNotificationSetting: createSetterForCategory('NOTIFICATION')
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function SettingsFactorySingleton(ObjectSerializerFactory, ArraySerializerFactory, ComponentsLogFactory, AuthFactory, ExperimentFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('SettingsFactory', SettingsFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.SettingsFactory

     * @description
     *
     * SettingsFactory
     */
    angular.module('origin-components')
        .factory('SettingsFactory', SettingsFactorySingleton);
}());
