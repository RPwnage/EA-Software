/**
 * @file bootstrap/factories/overrides.js
 */
(function() {
    'use strict';

    function OverridesFactory(HttpUtilFactory, LogFactory, APPCONFIG, COMPONENTSCONFIG) {
        var layerConfigurationInfoArray = [],
            queryParamsValidValueMap = {},
            overrideConstants = {
                ENV: 'env',
                CMSSTAGE: 'cmsstage',
                CONFIG: 'config',
                AUTOMATION: 'automation',
                VERSION: 'version',
                APP: 'app',
                COMPONENTS: 'components',
                JSSDK: 'jssdk',
                ENVTOKEN: '{env}',
                PRODUCTION: 'production',
                INT: 'int',
                QA: 'qa',
                FCQA: 'fc.qa',
                LOCAL: 'local',
                DEV: 'dev',
                PREVIEW: 'preview',
                REVIEW: 'review',
                APPROVE: 'approve',
                LIVE: 'live'
            },
            defaultEnvironment = overrideConstants.DEV;

        /**
         * helper function to init our config tables
         * @param {string} id               string representing the names of the different layers -- app/components/jssdk (must match whats used in EACore.ini)
         * @param {[type]} configObject     the baked in object containing urls for the layer
         * @param {[type]} overrideFilenames a override filename is one is needed (only jssdk needs one right now)
         */
        function addLayerConfig(id, configObject, overrideFilenames) {
            layerConfigurationInfoArray.push({
                id: id,
                configObject: configObject,
                overrideFilenames: overrideFilenames
            });
        }

        /**
         * helper function to setup the valid values table for the query params
         * @param {string} id               query param id
         * @param {string} validValuesArray an array containing valid values, if the array is empty that means all values are accepted
         */
        function addValidQueryParams(id, validValuesArray) {
            queryParamsValidValueMap[id] = validValuesArray;
        }

        function init() {
            addLayerConfig(overrideConstants.APP, APPCONFIG, ['app-config.json']);
            addLayerConfig(overrideConstants.COMPONENTS, COMPONENTSCONFIG, ['components-config.json']);
            addLayerConfig(overrideConstants.JSSDK, Origin.config, ['jssdk-origin-config.json', 'environment/' + overrideConstants.ENVTOKEN + '/jssdk-nonorigin-config.json']);

            addValidQueryParams(overrideConstants.ENV, [overrideConstants.PRODUCTION, overrideConstants.INT, overrideConstants.QA, overrideConstants.FCQA, overrideConstants.LOCAL, overrideConstants.DEV]);
            addValidQueryParams(overrideConstants.CMSSTAGE, [overrideConstants.PREVIEW, overrideConstants.REVIEW, overrideConstants.APPROVE, overrideConstants.LIVE]);
            addValidQueryParams(overrideConstants.CONFIG, []);
            addValidQueryParams(overrideConstants.AUTOMATION, []);

        }
        /**
         * parses query parameter
         * @param  {string} name name of query parameter
         * @return {string} value of query parameter
         */
        function getQueryParameterValue(name) {
            name = name.replace(/[\[]/, '\\[').replace(/[\]]/, '\\]');
            var regex = new RegExp('[\\?&]' + name + '=([^&#]*)'),
                results = regex.exec(location.search);
            return results === null ? '' : decodeURIComponent(results[1].replace(/\+/g, ' '));
        }

        /**
         * validates the value of the query parameter against a set
         * @param  {string} queryId the query param id
         * @return {string}         returns the value if its valid, null if its not
         */
        function parseAndValidateQueryParam(queryId) {
            var value = getQueryParameterValue(queryId),
                validValues = queryParamsValidValueMap[queryId];
            if (value) {
                value = value.toLowerCase();
            }

            if (validValues && validValues.length && value && validValues.indexOf(value) < 0) {
                LogFactory.warn(value + ' is not an accepted query param for id: ' + queryId);
                value = '';
            }
            return value;
        }

        /**
         * determine where to get the config files from, the predetermined path or a completely custom one
         * @param  {string} configFileUrl config url from query param if there is one
         * @return {string} the correct url to grab the config from
         */
        function determineConfigFilesPath(configFileUrl) {
            if (!configFileUrl) {
                configFileUrl = 'configs/';
            } else {
                //add a trailing slash if one is missing
                if(configFileUrl[configFileUrl.length-1] !== '/') {
                    configFileUrl += '/';
                }
            }
            return configFileUrl;
        }

        /**
         * mix the data from the hosted json files
         * @param  {object} layerConfigInfo the current baked in json config
         */
        function mixDataFromHostedJson(configObject) {
            return function(incomingData) {
                Origin.utils.mix(configObject, incomingData);
            };

        }

        /**
         * mix in the EACore.ini data
         * @param  {object} responses results from the async requests promise
         */
        function mixDataFromEACoreIni(responses) {
            //the first value in the array is the eacore.ini response
            var eaCoreData = responses[0],
                i = 0;
            for (i = 0; i < layerConfigurationInfoArray.length; i++) {
                var layerConfigInfo = layerConfigurationInfoArray[i];
                Origin.utils.mix(layerConfigInfo.configObject, eaCoreData[layerConfigInfo.id]);
            }
        }

        /**
         * retrieves data from the EACore.ini file if there was any query params specified
         * @return {promise} resolved when the file has been retrieved
         */
        function getEACoreIniOverride() {
            //if we have a query param check for EACore.ini otherwise do not
            if (window.location.search.length) {
                return HttpUtilFactory.getJsonFromUrl(APPCONFIG.eacoreini).catch(function() {
                    return {};
                });
            } else {
                return Promise.resolve({});
            }
        }

        /**
         * retrieve a file from a hosted json source if needed an mix with the baked in data
         * @param  {object} layerConfigInfo the current config info
         * @param  {string} configFilesPath the path to look for the override (minus the filename)
         * @return {promise}
         */
        function retriveAndMixFilesForSingleHostedJson(configObject, filePath) {
            return HttpUtilFactory.getJsonFromUrl(filePath)
                .then(mixDataFromHostedJson(configObject));
        }

        /**
         * retrieve override files needed for a layer
         * @param  {object} layerConfigInfo the current config info
         * @param  {string} configFilesPath the path to look for the override (minus the filename)
         * @param  {promise[]} asyncRequests   array containing promises for async requests
         * @param  {object} overrides       object containing override values
         */
        function retriveAndMixFilesForLayer(layerConfigInfo, configFilesPath, asyncRequests, overrides) {
            var i = 0,
                filename = '';

            for (i = 0; i < layerConfigInfo.overrideFilenames.length; i++) {
                filename = configFilesPath + layerConfigInfo.overrideFilenames[i];
                filename = filename.replace(overrideConstants.ENVTOKEN, overrides[overrideConstants.ENV]);
                asyncRequests.push(retriveAndMixFilesForSingleHostedJson(layerConfigInfo.configObject, filename));
            }
        }
        /**
         * updates the config files with overrides obtained from hosted json configs and the EACore.ini
         * @return  {promise} resolved when all the async override requests have completed
         */
        function updateConfigFiles(overrides) {
            //send the EACoreIni (if needed) request and the config file requests all at the same time
            var configFilesPath = determineConfigFilesPath(overrides[overrideConstants.CONFIG]),
                asyncRequests = [getEACoreIniOverride()],
                i = 0;

            for (i = 0; i < layerConfigurationInfoArray.length; i++) {
                retriveAndMixFilesForLayer(layerConfigurationInfoArray[i], configFilesPath, asyncRequests, overrides);
            }

            return Promise.all(asyncRequests).then(mixDataFromEACoreIni);
        }

        /**
         * parses the url path for env and version
         * @return  {object} overrides obtained from the url query params
         */
        function parseOverridesFromUrlHost() {
            var parsedHostArray = window.location.host.toLowerCase().split('.'),
                wwwLocation = parsedHostArray.indexOf('www'),
                envLocationIndex = wwwLocation - 1,
                versionLocationIndex = wwwLocation - 2,
                overrides = {};

            if (envLocationIndex >= 0) {
                overrides[overrideConstants.ENV] = parsedHostArray[envLocationIndex].toLowerCase();
            }

            if (versionLocationIndex >= 0) {
                overrides[overrideConstants.VERSION] = parsedHostArray[versionLocationIndex].toLowerCase();
            }

            return overrides;

        }

        /**
         * parses the url for registered query params
         * @return  {object} overrides obtained from the url query params
         */
        function parseOverridesFromQueryParams() {
            var overrides = {};
            for (var p in queryParamsValidValueMap) {
                if (queryParamsValidValueMap.hasOwnProperty(p)) {
                    var value = parseAndValidateQueryParam(p);
                    if (value.length) {
                        overrides[p] = value;
                    }
                }
            }

            return overrides;
        }

        /**
         * attach to he overrides to the different layers
         * @param  {object} overrides overrides obtained from the url
         */
        function attachOverridesToConfigs(overrides) {
            for (var i = 0; i < layerConfigurationInfoArray.length; i++) {
                layerConfigurationInfoArray[i].configObject.overrides = overrides;
            }
        }

        /**
         * sets up the configuration data by parsing the query params and pulling in any individual url overrides
         * @return {configObject} config data object containing serviceUrls, configurl, cmsOrigin, and env
         */
        function getOverrides() {
            var overrides = {
                version: '',
                env: overrideConstants.PRODUCTION,
                cmsstage: overrideConstants.LIVE,
                configurl: '',
                automation: false
            };

            //get overrides from the url
            Origin.utils.mix(overrides, parseOverridesFromUrlHost());
            Origin.utils.mix(overrides, parseOverridesFromQueryParams());

            //if the environment selected is "LOCAL" we default it to a default environment (since there are not apis that have a local env)
            if (overrides[overrideConstants.ENV].toLowerCase() === overrideConstants.LOCAL) {
                overrides[overrideConstants.ENV] = defaultEnvironment;
            }

            //attach the overrides to configs so they can be processed in the respective layers
            attachOverridesToConfigs(overrides);

            //get updates the baked in configs with any overrides
            return updateConfigFiles(overrides);
        }

        init();

        return {
            /**
             * overrides any urls configs based on path/query parammeters
             */
            getOverrides: getOverrides
        };
    }

    /**
     * @ngdoc service
     * @name originApp.factories.OverridesFactory
     * @requires $http
     * @description
     *
     * OverridesFactory
     */
    angular.module('bootstrap')
        .factory('OverridesFactory', OverridesFactory);

}());