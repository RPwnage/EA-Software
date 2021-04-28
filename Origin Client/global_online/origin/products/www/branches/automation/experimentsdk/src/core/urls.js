/*jshint strict: false */
/*jshint unused: false */

define([
    'generated/experimentsdkconfig.js'
], function(experimentsdkconfig) {
    /**
     * helper functions for managing experimentsdk urls
     * @module module:urls
     * @memberof module:Experiment
     * @private
     */

    var environment = 'production',
        cmsstage = 'live';

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

    function replaceTemplatedValuesInConfig(configObject) {
        var env = '',
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
        replaceInObject(configObject.hostname, {
            '{base}': configObject.hostname.base,
            '{env}': env,
            '{cmsstage}': cmsstage
        });

        replaceMap = {
            '{baseapi}': configObject.hostname.baseapi,
            '{basedata}': configObject.hostname.basedata,
            '{basenoversion}': configObject.hostname.basenoversion,
        };

        //then lets replace the tokens with the actual hosts in the urls
        replaceInObject(configObject.urls, replaceMap);
        replaceInObject(configObject.dictionary, replaceMap);

    }

    function init(env, cmsstage) {
        environment = env;
        cmsstage = cmsstage;

        experimentsdkconfig.overrides = {};
        experimentsdkconfig.overrides.env = env;
        experimentsdkconfig.overrides.cmsstage = cmsstage;
        replaceTemplatedValuesInConfig(experimentsdkconfig);
    }


    return /** @lends module:Experiment.module:urls */ {

        init: init,

        /**
         * endpoints used
         */
        endPoints: experimentsdkconfig.urls
    };
});