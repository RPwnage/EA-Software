// Karma configuration for Origin components
// http://karma-runner.github.io/0.10/config/configuration-file.html

'use strict';

module.exports = function(config) {
    config.set({
        frameworks: ['jasmine'],
        files: [
            // vendor libs
            'bower_components/promise-polyfill/Promise.js',
            'bower_components/lodash/lodash.js',

            // experimentation sdk source code
            'dist/eax-experimentsdk.js',

            // unit test scenarios
            'test/spec/**/*.js'
        ],
        port: 9192,
        logLevel: config.LOG_INFO,
        autoWatch: false,
        browsers: ['PhantomJS'],
        singleRun: true,
        reporters: ['progress', 'coverage'],
        preprocessors: {
            'dist/eax-experimentsdk.js': ['coverage']
        },
        coverageReporter:{
            type: 'html',
            dir: 'tmp/coverage_report'
        }
    });
};
