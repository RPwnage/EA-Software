/**
 * Grunt module to read JSON configuration files
 */
'use strict';

module.exports = function(grunt) {
    /**
     * Traverses configuration and recursively includes linked files if needed
     * @param {Object} config configuration object to parse
     * @return {Object} configuration with all the links included
     */
    function processConfigSection(config) {
        var processedConfig = getParameterValue(config);

        // Objects & arrays could potentially contain links to included configs
        if (processedConfig instanceof Array || processedConfig instanceof Object) {
            Object.keys(processedConfig).forEach(function(key) {
                var value = processedConfig[key];
                processedConfig[key] = processConfigSection(value);
            });
        }

        return processedConfig;
    }

    /**
     * Returns the configuration parameter value resolving links to other JSON files if needed.
     * Strings starting with @ contain paths to JSON files to be included, everything else is a primitive
     * @param {mixed} configParameter configuration parameter to resolve
     * @return {mixed}
     */
    function getParameterValue(configParameter) {
        var includePath;

        if (typeof configParameter === 'string' && configParameter.trim().charAt(0) === '@') {
            // This will load any local files and return those instead
            includePath = configParameter.trim().substring(1);
            return grunt.file.readJSON(includePath);
        } else {
            return configParameter;
        }
    }

    /**
     * Reads configuration from JSON files
     * @param {string} configFileName configuration file name
     * @return {Object}
     */
    function readConfig(config) {
        var retObj = {
            constants: {}
        };

        if(config.dest || config.name) {
            retObj['options'] = {
                dest: config.dest,
                name: config.name                
            }
        }
        
        //if its not an array lets make an array of 1
        if (config.constructor !== Array) {
            config = [config];
        }

        for (var i = 0; i < config.length; i++) {
            retObj.constants[config[i].id] = processConfigSection(grunt.file.readJSON(config[i].filename));
        }

        return retObj;
    }

    return {
        processConfigSection: processConfigSection,
        getParameterValue: getParameterValue,
        readConfig: readConfig
    };

};