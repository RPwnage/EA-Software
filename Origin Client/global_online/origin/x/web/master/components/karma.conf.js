// Karma configuration for Origin components
// http://karma-runner.github.io/0.10/config/configuration-file.html

'use strict';

module.exports = function(config) {
    config.set({
        frameworks: ['jasmine'],

        files: [
            // vendor libs
            'dist/bower_components/promise-polyfill/Promise.js',
            'dist/bower_components/angular/angular.js',
            'dist/bower_components/angular-mocks/angular-mocks.js',
            'dist/bower_components/angular-resource/angular-resource.js',
            'dist/bower_components/angular-cookies/angular-cookies.js',
            'dist/bower_components/angular-sanitize/angular-sanitize.js',
            'dist/bower_components/angular-toArrayFilter/toArrayFilter.js',
            'dist/bower_components/moment/moment.js',
            'dist/bower_components/lodash/lodash.js',

            // origin source code
            'dist/bower_components/origin-i18n/dist/origin-i18n.js',
            '.tmp/karma-test/origin-test-components.js',

            // unit test specs
            'test/spec/**/*.js'
        ],

        // web server port
        port: 9191,

        // level of logging
        // possible values: LOG_DISABLE || LOG_ERROR || LOG_WARN || LOG_INFO || LOG_DEBUG
        logLevel: config.LOG_INFO,

        // enable / disable watching file and executing tests whenever any file changes
        autoWatch: false,

        // Start these browsers, currently available:
        // - Chrome
        // - ChromeCanary
        // - Firefox
        // - Opera
        // - Safari (only Mac)
        // - PhantomJS
        // - IE (only Windows)
        browsers: ['PhantomJS'],

        // Continuous Integration mode
        // if true, it capture browsers, run tests and exit
        singleRun: true,

        //this directive generates coverage reports
        reporters: ['progress', 'coverage'],

        //instrument test coverage on our components package
        preprocessors: {
            '.tmp/karma-test/origin-test-components.js': ['coverage']
        },

        coverageReporter:{
            type: 'html',
            dir: 'tmp/coverage_report'
        }
    });
};
