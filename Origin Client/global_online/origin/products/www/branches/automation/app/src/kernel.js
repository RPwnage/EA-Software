(function() {
    'use strict';

    var overridesPromise = null;
    var HTTP_REQUEST_TIMEOUT = 30000; //milliseconds

    if (typeof(window.OriginKernel) === 'undefined') {window.OriginKernel = {};}

    OriginKernel.promises = {};
    try {
        OriginKernel.loadedFromChildView = !!_.get(window, ['opener', 'Origin']);
    } catch (exp) {
        //We expect this error if the SPA is being loaded in a popup,
        //Therefore we don't have access to window.opener
        OriginKernel.loadedFromChildView = false;
    }

    OriginKernel.APPCONFIG = {};
    OriginKernel.COMPONENTSCONFIG = {};
    OriginKernel.JSSDKCONFIG = {};
    OriginKernel.BREAKPOINTS = {};

    OriginKernel.initJSSDK = function() {
        try {
            if (OriginKernel.loadedFromChildView) {
                Origin = window.opener.Origin;
                return Promise.resolve();
            } else {
                _.extend(Origin.config, OriginKernel.JSSDKCONFIG);
                return Origin.init();
            }
        } catch(exp) {
            //We expect this error if the SPA is being loaded in a popup,
            //Therefore we don't have access to window.opener
            return Origin.init();
        }
    };

    OriginKernel.prefetchRequests = function() {
        if(!OriginKernel.loadedFromChildView) {
            //initiate supercat load first 'cuz it takes a while for worker to kick in
            OriginKernel.promises.getSupercat = OriginKernel.Prefetcher.getSupercat();
        }

        OriginKernel.promises.initJSSDK = OriginKernel.initJSSDK();

        //if we are in OIG we don't want to run the login logic, as the auth model is shared and not run again
        if (!OriginKernel.loadedFromChildView) {
            OriginKernel.promises.testForLogin = OriginKernel.Prefetcher.testForLoggedIn()
                .then(function(data) {
                    return data.data;
                }).catch(function() {
                    //resolve it anyways, cuz we want the rest to proceed,
                    //we need data.code to not be undefined so Origin.auth.login will occur
                    return {
                        data: {
                            code: ''
                        }
                    };
                });
        }

        OriginKernel.promises.getDefaults = OriginKernel.Prefetcher.getDefaults();
        OriginKernel.Prefetcher.getTemplates();
    };

    OriginKernel.initiateAuth = function() {
        function startJSSDKLogin(data) {
            if (typeof(data[1].code) !== 'undefined') {
                OriginKernel.promises.startJSSDKLogin = Origin.auth.login();
            }
        }

        //if we are in OIG we don't want to run the login logic, as the auth model is shared and not run again
        if(!OriginKernel.loadedFromChildView)  {
            return Promise.all([OriginKernel.promises.initJSSDK, OriginKernel.promises.testForLogin])
                .then(startJSSDKLogin);
        } else {
            return Promise.resolve();
        }
    };


    OriginKernel.Utils = (function() {

        /**
         * determines whether we should load the JSSDK depend based on window.opener
         * @return {boolean}
         */

        function isJSSDKLoaded() {
            try {
                if (!OriginKernel.loadedFromChildView) {
                    return false;
                }
            } catch (exp) {
                //We expect this if Origin is loaded via _blank target from another site
                return false;
            }
            return true;
        }

        function get(url, withCredentials) {
            // Return a new promise.
            return new Promise(function(resolve, reject) {
                // Do the usual XHR stuff
                var req = new XMLHttpRequest();
                req.open('GET', url);

                //set a timeout value if the request takes longer than HTTP_REQUEST_TIMEOUT
                req.timeout = HTTP_REQUEST_TIMEOUT;

                if(withCredentials) {
                    req.withCredentials =  true;
                }

                req.onload = function() {
                    // This is called even on 404 etc
                    // so check the status
                    if (req.status === 200) {
                        // Resolve the promise with the response text
                        var data = {};
                        try {
                            data = JSON.parse(req.response);
                        } catch (e) {}
                        resolve({data: data});
                    } else {
                        // Otherwise reject with the status text
                        // which will hopefully be a meaningful error
                        reject(Error(req.statusText));
                    }
                };

                // Handle network errors
                req.onerror = function() {
                    reject(Error('[OriginKernel.Utils.get]: Network Error - ') + url);
                };

                // Handle the timeout by rejecting the promise
                req.ontimeout = function() {
                    reject(Error('[OriginKernel.Utils.get]: Timeout Error - ') + url);
                };

                // Make the request
                req.send();
            });
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

        return {
            isJSSDKLoaded: isJSSDKLoaded,
            get: get,
            replaceInObject: replaceInObject
        };

    }());

    OriginKernel.Prefetcher = (function() {

        function getDefaults() {
            return OriginKernel.Utils.get(OriginKernel.APPCONFIG.dictionary.remote);
        }

        function testForLoggedIn() {
            return OriginKernel.Utils.get(OriginKernel.APPCONFIG.auth.testForLoginUrl, true);
        }

        function getSupercat() {
            var locale = originLocaleApi.getLocale().toLowerCase(),
                splitLocale = locale.split('-'),
                country = originLocaleApi.getCountryCode().toUpperCase(),
                url = OriginKernel.JSSDKCONFIG.urls.criticalCatalogInfo,

                //map from cq5 locale to eadp locale by country, if entry is missing, then no need to map
                //table is set here: https://confluence.ea.com/pages/viewpage.action?spaceKey=EBI&title=Origin+X+EADP+Store+Front+Mappings
                LOCALE2EADPLOCALE = {
                    'en_US': {
                        'AU': 'en_AU',
                        'BE': 'en_BE',
                        'BR': 'en_BR',
                        'CA': 'en_CA',
                        'DE': 'en_DE',
                        'DK': 'en_DK',
                        'ES': 'en_ES',
                        'FI': 'en_FI',
                        'FR': 'en_FR',
                        'GB': 'en_GB',
                        'HK': 'en_HK',
                        'IE': 'en_IE',
                        'IN': 'en_IN',
                        'IT': 'en_IT',
                        'JP': 'en_JP',
                        'KR': 'en_KR',
                        'MX': 'en_MX',
                        'NL': 'en_NL',
                        'NO': 'en_NO',
                        'NZ': 'en_NZ',
                        'PL': 'en_PL',
                        'RU': 'en_RU',
                        'SE': 'en_SE',
                        'SG': 'en_SG',
                        'TH': 'en_TH',
                        'TW': 'en_TW',
                        'ZA': 'en_ZA'
                    },

                    'fr_FR': {
                        'BE': 'fr_BE'
                    },

                    'nl_NL': {
                        'BE': 'nl_BE'
                    }
                };

            splitLocale[1] = splitLocale[1].toUpperCase();
            locale = splitLocale.join('_');

            if (LOCALE2EADPLOCALE[locale] && LOCALE2EADPLOCALE[locale][country]) {
                locale = LOCALE2EADPLOCALE[locale][country];
            }

            url = url.replace(/{country2letter}/g, country);
            url = url.replace(/{locale}/g, locale);

            return new Promise(function(resolve, reject) {
                var timeoutHandle = null,
                TIMEOUT_VALUE = 5000;

                function resetTimeout() {
                    if(timeoutHandle) {
                        clearTimeout(timeoutHandle);
                        timeoutHandle = null;
                    }
                }

                function handleError(error) {
                    Origin.performance.endTime('PREFETCH:supercat');
                    reject(error);
                }

                function handleTimeout() {
                    timeoutHandle = null;
                    handleError(new Error('PREFETCH: supercat timed out'));
                }

                function handleResponse(e) {
                    resetTimeout();
                    if (e.data.byteLength) {
                        //we got a valid response
                        var decoder = new TextDecoder('utf-8'),
                            view = new DataView(e.data, 0, e.data.byteLength),
                            string = decoder.decode(view),
                            object = JSON.parse(string);

                        Origin.performance.endTime('PREFETCH:supercat');
                        Origin.performance.measureTime('PREFETCH:supercat');
                        resolve(object);
                    } else {
                       handleError(new Error(e.data));
                    }

                   // we don't use the worker anymore so lets free anything associated with it
                   OriginKernel.worker  = null;
                }

                //lets wait a max of TIMEOUT_VALUE before we abort the supercat
                //incase there's any issue with the worker file (not likely)
                timeoutHandle = setTimeout(handleTimeout, TIMEOUT_VALUE);
                OriginKernel.worker.addEventListener('message', handleResponse, false);

                Origin.performance.beginTime('PREFETCH:supercat');

                //start the worker
                OriginKernel.worker.postMessage(url);
            });

        }


        function getTemplateError() {
            //don't do anything, just catch error
        }

        function getTemplates() {
            return Promise.all([
                    OriginKernel.Utils.get(OriginKernel.APPCONFIG.urls['shell-navigation']),
                    OriginKernel.Utils.get(OriginKernel.APPCONFIG.urls.storehome),
                    OriginKernel.Utils.get(OriginKernel.APPCONFIG.urls.home),
                    OriginKernel.Utils.get(OriginKernel.APPCONFIG.urls['game-library'])
                ])
                .catch(getTemplateError);
        }

        return {
            getDefaults: getDefaults,
            getSupercat: getSupercat,
            getTemplates: getTemplates,
            testForLoggedIn: testForLoggedIn
        };

    }());

    OriginKernel.Overrides = (function() {
        var layerConfigurationInfoArray = [],
            queryParamsValidValueMap = {},
            constants = {
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
                INTEGRATION: 'integration',
                LOADTEST: 'loadtest',
                QA: 'qa',
                FCQA: 'fc.qa',
                LOCAL: 'local',
                DEV: 'dev',
                PREVIEW: 'preview',
                REVIEW: 'review',
                APPROVE: 'approve',
                LIVE: 'live',
                ENABLESOCIAL: 'enablesocial',
                TRUE: 'true',
                SHOWCOMPONENTNAMES: 'showcomponentnames',
                SHOWMISSINGSTRINGS: 'showmissingstrings',
                SHOWMISSINGSTRINGSFALLBACK: 'showmissingstringsfallback',
                SHOWLOGGING: 'showlogging',
                HIDESTATICSHELL: 'hidestaticshell',
                HIDESITESTRIPE: 'hidesitestripe',
                USEFIXTUREFILE:'usefixturefile',
                OIGCONTEXT: 'oigcontext',
                TEALIUMENV: 'tealiumenv',
                ALPHA: 'alpha',
                BETA: 'beta',
                ABTEST: 'abTest',
                TEST: 'test'
            },
            defaultEnvironmentForLocal = constants.DEV,
            knownEnvironmentLabel = [
                constants.LOCAL,
                constants.DEV,
                constants.QA,
                constants.INTEGRATION,
                constants.LOADTEST
            ];

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

        /**
         * parses query parameter
         * @param  {string} name name of query parameter
         * @return {string} value of query parameter
         */
        function getQueryParameterValue(name) {
            name = name.replace(/[\[]/, '\\[').replace(/[\]]/, '\\]');
            var regex = new RegExp('[\\?&]' + name + '=([^&#]*)'),
                results = regex.exec(location.search),
                result = '';

                if (results !== null) {
                    //get rid of trailing /
                    result = results[1];

                    //remove trailing, e.g. preview/
                    if(result.substr(-1) === '/') {
                        result = result.substr(0, result.length - 1);
                    }

                    //replace any + with space
                    result = result.replace(/\+/g, ' ');
                    result = decodeURIComponent(result);

                    // ui router / angular tends to double encode slashes so
                    // here we handle that.  The browser converts slashes to
                    // %2F then angular encodes it again as it interprets it
                    // as a regular slash.
                    if (result.indexOf('%2F') > -1) {
                        result = result.replace(/%2F/g, '/');
                    }

                }

            return result;
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
                //LogFactory.warn(value + ' is not an accepted query param for id: ' + queryId);
                value = '';
            }
            return value;
        }

        /**
         * add a / to the url if needed
         * @param  {string} configFileUrl config url we are cleaning
         * @return {string} the url in the correct format
         */
        function sanitizeUrl(configFileUrl) {
            //add a trailing slash if one is missing
            if (configFileUrl[configFileUrl.length-1] !== '/') {
                configFileUrl += '/';
            }
            return configFileUrl;
        }

        /**
         * mix the data from the hosted json files
         * @param  {object} layerConfigInfo the current baked in json config
         */
        function mixDataFromHostedJson(configObject) {
            return function(incomingData) {
                _.merge(configObject, incomingData.data);
            };
        }

        /**
        * handler to do nothing
        */
        function onGetError() {}

        /**
         * retrieve a file from a hosted json source if needed an mix with the baked in data
         * @param  {object} layerConfigInfo the current config info
         * @param  {string} configFilesPath the path to look for the override (minus the filename)
         * @return {promise}
         */
        function retriveAndMixFilesConfigFile(configObject, filePath, fileName) {
            if (filePath) {
                return OriginKernel.Utils.get(filePath+fileName)
                    .then(mixDataFromHostedJson(configObject))
                    .catch(onGetError);
            } else {
                if (!!OriginKernel.configs['dist/configs/' + fileName]) {
                    _.merge(configObject, OriginKernel.configs['dist/configs/' + fileName]);
                }
                return Promise.resolve();
            }
        }

        /**
         * retrieve override files needed for a layer
         * @param  {object} layerConfigInfo the current config info
         * @param  {string} configFilesPath the path to look for the override (minus the filename)
         * @param  {promise[]} asyncRequests   array containing promises for async requests
         * @param  {object} overrides       object containing override values
         */
        function retrieveAndMixConfigs(configFilesPath, overrides) {
            var filename = '',
                asyncRequests = [];
            for (var i = 0; i < layerConfigurationInfoArray.length; i++) {
                for (var j = 0; j < layerConfigurationInfoArray[i].overrideFilenames.length; j++) {
                    filename = layerConfigurationInfoArray[i].overrideFilenames[j];
                    filename = filename.replace(constants.ENVTOKEN, overrides[constants.ENV]);
                    asyncRequests.push(retriveAndMixFilesConfigFile(layerConfigurationInfoArray[i].configObject, configFilesPath, filename));
                }
            }
            return asyncRequests;
        }
        /**
         * updates the config files with overrides obtained from hosted json configs
         * @return  {promise} resolved when all the async override requests have completed
         */
        function updateConfigFiles(overrides, configFilesPath) {
            configFilesPath = configFilesPath ? sanitizeUrl(configFilesPath) : '';
            return Promise.all(retrieveAndMixConfigs(configFilesPath, overrides));
        }

        function isKnownEnvironmentLabel(hostLabel) {
            return (knownEnvironmentLabel.indexOf(hostLabel) > -1);
        }

        /**
         * parses the url path for env and version
         * @return  {object} overrides obtained from the url query params
         */
        function parseOverridesFromUrlHost() {
            var parsedHostArray = window.location.host.toLowerCase().split('.'),
                envLocationIndex = -1,
                versionLocationIndex = -1,
                overrides = {},
                parsedVersionArray = [],
                isNum = true;

            for (var i = 0; i < parsedHostArray.length; i++) {
                if (isKnownEnvironmentLabel(parsedHostArray[i].toLowerCase())) {
                    envLocationIndex = i;
                }

                if (parsedHostArray[i].indexOf('-') > -1) {
                    //make sure it's a #
                    parsedVersionArray = parsedHostArray[i].split('-');
                    for (var j = 0; j < parsedVersionArray.length; j++) {
                        if (isNaN(parseInt(parsedVersionArray[j]))) {
                            isNum = false;
                            break;
                        }
                    }

                    if (isNum) {
                        versionLocationIndex = i;
                    }
                } else if ((parsedHostArray[i].toLowerCase() === constants.ALPHA) ||
                          (parsedHostArray[i].toLowerCase() === constants.BETA)) {
                    versionLocationIndex = i;
                }
            }

            if (envLocationIndex >= 0) {
                overrides[constants.ENV] = parsedHostArray[envLocationIndex].toLowerCase();
            } else {
                //there is no env location in url assume production
                overrides[constants.ENV] = constants.PRODUCTION;
            }

            if (versionLocationIndex >= 0) {
                overrides[constants.VERSION] = parsedHostArray[versionLocationIndex].toLowerCase();
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
         * converts an incoming string to boolean
         * @param  {string} incomingString a boolean string
         * @return {boolean} returns true/false
         */
        function convertToBool(incomingString) {
            return incomingString ? String.prototype.toLowerCase.apply(incomingString) === 'true' : false;
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

            var locale = originLocaleApi.getLocale().toLowerCase(),
                country = originLocaleApi.getThreeLetterCountryCode().toLowerCase(),
                env = '',
                version = '',
                cmsstage = '',
                replaceMap;

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
                    cmsstage = configObject.overrides.cmsstage + '/';
                }
            }

            //first we replace the override information in the hostname section
            OriginKernel.Utils.replaceInObject(configObject.hostname, {
                '{base}': configObject.hostname.base,
                '{env}': env,
                '{version}': version,
                '{cmsstage}': cmsstage
            });

            replaceMap = {
                '{baseapi}': configObject.hostname.baseapi,
                '{basedata}': configObject.hostname.basedata,
                '{basenoversion}': configObject.hostname.basenoversion,
                '{cdn}': configObject.hostname.cdn,
                '{websocket}': configObject.hostname.websocket,
                '{countrylocale}': locale + '.' + country
            };

            //then lets replace the tokens with the actual hosts in the urls
            OriginKernel.Utils.replaceInObject(configObject.urls, replaceMap);
            OriginKernel.Utils.replaceInObject(configObject.dictionary, replaceMap);

        }

        function replaceTemplatedValues() {
            replaceTemplatedValuesInConfig(OriginKernel.APPCONFIG);
            replaceTemplatedValuesInConfig(OriginKernel.COMPONENTSCONFIG);
            //if we are using the main SPA's jssdk because we are a popout window, do not try to override the urls again, stopping over the origin jssdk urls
            if (!OriginKernel.Utils.isJSSDKLoaded()) {
                replaceTemplatedValuesInConfig(OriginKernel.JSSDKCONFIG);
            }
        }

        /**
         * look for bad env/cmsstage conbinations
         * prod + cmsstage=review|preview
         * integration + cmsstage=approve|live|''
         */
        function isBadConfig(/*overrides*/) {
            //MY: turn this off for now until we can get this sorted out
            /*
            if (overrides[constants.ENV].toLowerCase() === constants.PRODUCTION) {
                if (overrides[constants.CMSSTAGE].toLowerCase() === constants.PREVIEW ||
                    overrides[constants.CMSSTAGE].toLowerCase() === constants.REVIEW ) {
                    return true;
                }
            } else if (overrides[constants.ENV].toLowerCase() === constants.INTEGRATION) {
                if (overrides[constants.CMSSTAGE].toLowerCase() === constants.APPROVE ||
                    overrides[constants.CMSSTAGE].toLowerCase() === constants.LIVE  ||
                    overrides[constants.CMSSTAGE].toLowerCase() === '' ) {
                    return true;
                }
            }
            */
            return false;
        }

        /**
         * sets up the configuration data by parsing the query params and pulling in any individual url overrides
         * @return {configObject} config data object containing serviceUrls, configurl, cmsOrigin, and env
         */
        function getOverrides() {
            var overrides = {
                version: '',
                env: constants.DEV, //for now, force everything to default to DEV until we can get the sandbox endpoints updated
                cmsstage: constants.LIVE,
                configurl: '',
                automation: false
            };

            //get overrides from the url
            _.merge(overrides, parseOverridesFromUrlHost());
            _.merge(overrides, parseOverridesFromQueryParams());

            //NOTE: these should only be set from the url params; should not be set with a fallback value as it will end up adding it as
            //a param across routes and will break offline for embedded
            overrides[constants.USEFIXTUREFILE] = convertToBool(overrides[constants.USEFIXTUREFILE]);
            overrides[constants.HIDESITESTRIPE] = convertToBool(overrides[constants.HIDESITESTRIPE]);
            overrides[constants.SHOWMISSINGSTRINGS] = convertToBool(overrides[constants.SHOWMISSINGSTRINGS]);
            overrides[constants.SHOWCOMPONENTNAMES] = convertToBool(overrides[constants.SHOWCOMPONENTNAMES]);
            overrides[constants.SHOWMISSINGSTRINGSFALLBACK] = convertToBool(overrides[constants.SHOWMISSINGSTRINGSFALLBACK]);

            //used to hide the static shell, actual check exists in staticshell.js, which happens before this is loaded
            //kept here for consistency
            overrides[constants.HIDESTATICSHELL] = convertToBool(overrides[constants.HIDESTATICSHELL]);


            //the actual show logging check is handled in logger.js by the jssdk itself, we setup the same variable here to keep it in the url
            //the logger needs this variable at the time of jssdk PARSING which happens before this code is called.
            overrides[constants.SHOWLOGGING] = convertToBool(overrides[constants.SHOWLOGGING]);

           //if the environment selected is "PRODUCTION" we do not use a fixture file
            if (overrides[constants.ENV].toLowerCase() === constants.PRODUCTION) {
                overrides[constants.USEFIXTUREFILE] = false;
            }

            //if the environment selected is "LOCAL" we default it to a default environment (since there are not apis that have a local env)
            if (overrides[constants.ENV].toLowerCase() === constants.LOCAL) {
                overrides[constants.ENV] = defaultEnvironmentForLocal;
            }

            if (isBadConfig(overrides)) {
                var err = new Error();
                err.msg = 'BADCONFIG';
                return Promise.reject(err);
            } else {

                var configLocation;

                if ((overrides[constants.ENV].toLowerCase() !== constants.PRODUCTION) &&
                    (overrides[constants.ENV].length !== 0)) {
                    //non-prod environments get this set, which asks the updateConfig files to
                    //attempt to retrive configs from server
                    configLocation = '/configs/';
                }

                //attach the overrides to configs so they can be processed in the respective layers
                attachOverridesToConfigs(overrides);

                //1. grab data from external source for non prod environments (Prod ones are already baked in)
                //2. if there is a configURL set, overlay those on top
                return updateConfigFiles(overrides, configLocation)
                    .then(function() {
                        var configUrl = overrides[constants.CONFIG];
                        //only config urls that start with config.x.origin.com are valid
                        if (_.startsWith(configUrl, '/configs/pubint') ||
                            _.startsWith(configUrl, 'https://config.x.origin.com/') ||
                            _.startsWith(configUrl, 'https://dev.mockup.x.origin.com/') ||
                            _.startsWith(configUrl, 'https://www.mockup.x.origin.com/')
                            ) {
                            return updateConfigFiles(overrides, configUrl);
                        } else {
                            return Promise.resolve();
                        }
                    });

            }
        }

        addLayerConfig(constants.APP, OriginKernel.APPCONFIG, ['app-config.json', 'environment/' + constants.ENVTOKEN + '/app-nonorigin-config.json']);
        addLayerConfig(constants.COMPONENTS, OriginKernel.COMPONENTSCONFIG, ['components-config.json', 'environment/' + constants.ENVTOKEN + '/components-nonorigin-config.json']);
        addLayerConfig('breakpoints', OriginKernel.BREAKPOINTS, ['breakpoints-config.json']);

        // if we are using the main SPA's jssdk because we are a popout window,
        // do not try to override the urls again, stopping over the origin jssdk urls
        if (!OriginKernel.Utils.isJSSDKLoaded()) {
            addLayerConfig(constants.JSSDK, OriginKernel.JSSDKCONFIG, ['jssdk-origin-config.json', 'environment/' + constants.ENVTOKEN + '/jssdk-nonorigin-config.json']);
        }

        addValidQueryParams(constants.ENV, [constants.PRODUCTION, constants.INTEGRATION, constants.QA, constants.FCQA, constants.LOCAL, constants.DEV, constants.LOADTEST]);
        addValidQueryParams(constants.CMSSTAGE, [constants.PREVIEW, constants.REVIEW, constants.APPROVE, constants.LIVE]);
        addValidQueryParams(constants.ENABLESOCIAL, [constants.TRUE]);
        addValidQueryParams(constants.CONFIG, []);
        addValidQueryParams(constants.AUTOMATION, []);
        addValidQueryParams(constants.SHOWCOMPONENTNAMES, []);
        addValidQueryParams(constants.SHOWMISSINGSTRINGS, []);
        addValidQueryParams(constants.SHOWLOGGING, [constants.TRUE]);
        addValidQueryParams(constants.SHOWMISSINGSTRINGSFALLBACK, [constants.TRUE]);
        addValidQueryParams(constants.USEFIXTUREFILE, []);
        addValidQueryParams(constants.HIDESITESTRIPE, [constants.TRUE]);
        addValidQueryParams(constants.OIGCONTEXT, [constants.TRUE]);
        addValidQueryParams(constants.TEALIUMENV, [constants.PRODUCTION, constants.DEV, constants.QA]);
        addValidQueryParams(constants.ABTEST, [constants.PRODUCTION, constants.TEST]);

        return {
            constants: constants,
            getOverrides: getOverrides,
            replaceTemplatedValues: replaceTemplatedValues
        };
    }());

    overridesPromise = OriginKernel.Overrides.getOverrides()
        .then(OriginKernel.Overrides.replaceTemplatedValues)
        .then(OriginKernel.prefetchRequests)
        .then(OriginKernel.initiateAuth);




    OriginKernel.Bootstrap = (function () {
        /**
        * writes an entry to localstorage to state that the client has the data to run offline mode [EMBEDDED BROWSER ONLY]
        * @param {boolean} offlineGoodToGo flag to state whether we have the files we need to run offline
        */
        function setOfflineCapable(offlineGoodToGo) {
            if (!OriginKernel.Utils.isJSSDKLoaded() && Origin.client.isEmbeddedBrowser() && Origin.client.onlineStatus.isOnline()) {
                Origin.client.settings.setSPAofflineCapable(offlineGoodToGo);
                console.log('BootstrapInitFactory: setting SPAPreloadSuccess=', offlineGoodToGo);
            }
        }

        /**
        * init the appurls and preload any templates we need for offline
        * @returns {promise}
        */
        function initAppUrlsAndOfflineCache() {
            return Promise.resolve(true);
        }


        /**
        * Initialize the locale settings in JSSDK from OriginLocale's api instance
        */
        function initJSSDKLocale() {
            //don't reinitialize this if we're loading this from a child view
            if (!OriginKernel.loadedFromChildView) {
                Origin.locale.setLocale(originLocaleApi.getCasedLocale());
                Origin.locale.setLanguageCode(originLocaleApi.getLanguageCode());
                Origin.locale.setCountryCode(originLocaleApi.getCountryCode());
                Origin.locale.setThreeLetterCountryCode(originLocaleApi.getThreeLetterCountryCode());
                Origin.locale.setCurrencyCode(originLocaleApi.getCurrencyCode());
            }
        }

        /**
        * function waits for all promises to resolve then resolves itself
        * @returns {promise}
        */
        function getAsynchronousDependencies() {
            return Promise.all([overridesPromise, initAppUrlsAndOfflineCache(), OriginKernel.promises.initJSSDK]);
        }

        /**
        * handles the results of the batched async dependencies requests
        * @param  {object} results the results of the async requests are stored in an object under properties that correspond to the object used in getAsynchronousDependencies.
        */
        function handleAsyncDependencyResults(overrides, appUrls) {
            //set offline capable with the result of the preload promise.
            setOfflineCapable(appUrls);

            // set the locale in JSSDK
            initJSSDKLocale();
        }

        function setupTealium(a, b, c, d) {

            // Prepare to select Tealium environment.
            var env,
                environmentNames = {},
                defaultEnvironment = OriginKernel.Overrides.constants.PRODUCTION;

            // Define the valid Tealium environment names.
            environmentNames[OriginKernel.Overrides.constants.PRODUCTION] = 'prod';
            environmentNames[OriginKernel.Overrides.constants.QA] = 'qa';
            environmentNames[OriginKernel.Overrides.constants.DEV] = 'dev';

            // If there is a valid tealium environment selected,
            if (environmentNames.hasOwnProperty(OriginKernel.APPCONFIG.tealium.environment)) {

                // Select the appropriate environment name.
                env = environmentNames[OriginKernel.APPCONFIG.tealium.environment];

            // Otherwise,
            } else {

                // Select the default environment name.
                env = environmentNames[defaultEnvironment];
            }

            // If there is a valid environment override selected,
            if (environmentNames.hasOwnProperty(OriginKernel.APPCONFIG.overrides.tealiumenv)) {

                // Select the overridden environment name.
                env = environmentNames[OriginKernel.APPCONFIG.overrides.tealiumenv];
            }

            // Inject the specified Tealium environment script.
            a = '//tags.tiqcdn.com/utag/ea/originx/' + env + '/utag.js';
            b = document;
            c = 'script';
            d = b.createElement(c);
            d.src = a;
            d.type = 'text/java' + c;
            d.async = true;
            a = b.getElementsByTagName(c)[0];
            a.parentNode.insertBefore(d, a);
        }

        /*
         * Injects Optimizely onto page
         */
        function setupOptimizely() {
            var DEV_PROJECT_ID = '7576692909';
            var PROD_PROJECT_ID = '869004069';
            var project = PROD_PROJECT_ID; // set default project as prod

            // Check which Optimizely project to serve
            if (
                OriginKernel.APPCONFIG.overrides.abTest === OriginKernel.Overrides.constants.TEST ||
                _.get(OriginKernel, 'APPCONFIG.abtest.environment') === OriginKernel.Overrides.constants.TEST
            ) {
                project = DEV_PROJECT_ID;
                _.set(OriginKernel, 'APPCONFIG.abTest.environment', OriginKernel.Overrides.constants.TEST);
            }

            // Create specified Optimizely snippet
            var snippet = document.createElement('script');
            snippet.src = '//cdn.optimizely.com/js/' + project + '.js';
            snippet.type = 'text/javascript';
            snippet.async = true;

            // Inject Optimizely snippet
            var firstScriptElement = document.getElementsByTagName('script')[0];
            firstScriptElement.parentNode.insertBefore(snippet, firstScriptElement);
        }

        /**
        * handles the case when bad config is passed in -- we abort load of the SPA, otherwise it passes the error down
        */
        /*function handleGetOverrideError(err) {
            if (err.msg === 'BADCONFIG') {
                window.alert('Invalid environment and cmsstage combination');
                window.location.href = 'about:blank';
            } else {
                //pass the error down
                Promise.reject(err);
            }
        }*/

        /**
        * the init function to kick off all the async requests
        * @returns {promise}
        */
        function init() {
            return getAsynchronousDependencies()
                .then(_.spread(handleAsyncDependencyResults))
                .then(setupTealium)
                .then(setupOptimizely);
        }

        return {
            /**
            * the init function to kick off all the async requests
            * @returns {promise}
            */
            init: init
        };

    })();


}());
