/**
 * @file bootstrap/factories/overrides.js
 */
(function() {
    'use strict';

    function OverridesFactory(HttpUtilFactory, LogFactory, APPCONFIG, COMPONENTSCONFIG) {
        var jssdkOverrides = {
                common: {},
                overrides: {}
            },
            environmentId = 'env',
            cmsId = 'cmsstage';

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
         * takes a primitive and mixes it with the APPCONFIG.common, COMPONENTSCONFIG.common, JSSDCONFIG.common objects
         * @param  {string} commonProperty   name of the property we want to mix it with
         * @param  {primitive} incomingProperty the value we want to replace the property with
         */
        function updateCommonConfigs(incomingProperty, commonProperty) {
            var incomingObj = {};

            if (commonProperty) {
                incomingObj[commonProperty] = incomingProperty;
            } else {
                incomingObj = incomingProperty;
            }

            Origin.utils.mix(APPCONFIG.common, incomingObj);
            Origin.utils.mix(COMPONENTSCONFIG.common, incomingObj);
            Origin.utils.mix(jssdkOverrides.common, incomingObj);
        }

        /**
         * updates the cms origin if we have a env=sb param so that we read the config files from a local build
         */
        function updateDynServicesOrigin() {
            var env = getQueryParameterValue(environmentId);
            if (env === 'sb') {
                var origin = location.protocol + '//' + location.hostname;
                updateCommonConfigs(origin, 'dynServicesOrigin');
            }
        }

        /**
         * updates the environment we should use
         */
        function updateEnvironment() {
            var env = getQueryParameterValue(environmentId),
                validEnv = ['prod', 'int', 'qa', 'fcqa', 'local', 'dev', 'sb'];

            if (env) {
                if (validEnv.indexOf(env) < 0) {
                    LogFactory.warn('OVERRIDES: \'' + env + '\' not a recognized environment.');
                } else {
                    updateCommonConfigs(env, 'env');
                }

            }
        }

        /**
         * updates the cms stage we should
         */
        function updateCmsStage() {
            var stage = getQueryParameterValue(cmsId);

            if (stage) {
                APPCONFIG.common.cmsstage = stage;
                COMPONENTSCONFIG.common.cmsstage = stage;
            } else {
                //temp right now, if there's no stage specified lets use our temp cms.x.origin.com location
                updateCommonConfigs('https://cms.x.origin.com', 'cmsOrigin');
            }
        }

        /**
         * mixes the eacoredata with the hardcoded overrides
         * @param {object} eaCoreData json object coming from the EACoreIniUtil service
         */
        function mixEACoreIni(eaCoreData) {
            if (eaCoreData && eaCoreData.urls) {
                if (eaCoreData.urls) {
                    updateCommonConfigs(eaCoreData.urls.common);
                }

                //mix the override urls hardcoded in the app with the ones coming from the EACore.ini
                Origin.utils.mix(APPCONFIG.overrides, eaCoreData.urls.app);
                Origin.utils.mix(COMPONENTSCONFIG.overrides, eaCoreData.urls.components);
                Origin.utils.mix(jssdkOverrides.overrides, eaCoreData.urls.jssdk);
            } else {
                LogFactory.warn('OVERRIDES: No urls to override');
            }
        }

        /**
         * sets up the configuration data by parsing the query params and pulling in any individual url overrides
         * @return {configObject} config data object containing serviceUrls, configUrl, cmsOrigin, and env
         */
        function getOverrides() {
            var checkForEACoreIniOverride = getQueryParameterValue(environmentId),
                promiseObject = null;

            updateEnvironment();
            updateCmsStage();
            updateDynServicesOrigin();

            //start and get the overridden env and urls if available, then retrieve appUrls from server
            //this retrieves override urls for app/components/jssdk
            if (checkForEACoreIniOverride) {
                promiseObject = HttpUtilFactory.getJsonFromUrl(APPCONFIG.common.eacoreOverrideFileUrl)
                    .then(mixEACoreIni);
            } else {
                promiseObject = Promise.resolve();
            }

            return promiseObject;
        }

        return {
            getOverrides: getOverrides,
            getJSSDKOverrides: function() {
                return jssdkOverrides;
            }
        };
    }

    /**
     * @ngdoc service
     * @name originApp.factories.AppUrlsFactory
     * @requires $http
     * @description
     *
     * AppUrlsFactory
     */
    angular.module('bootstrap')
        .factory('OverridesFactory', OverridesFactory);

}());