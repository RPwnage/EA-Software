// Karma configuration
// http://karma-runner.github.io/0.10/config/configuration-file.html

module.exports = function(config) {
    config.set({
        // base path, that will be used to resolve files and exclude
        basePath: '',

        // testing framework to use (jasmine/mocha/qunit/...)
        frameworks: ['jasmine'],

        preprocessors: {
            'dist/views/*.html': ['ng-html2js']
        },

        // list of files / patterns to load in the browser
        files: [

            // angular
            'src/bower_components/angular/angular.js',
            'src/bower_components/angular-mocks/angular-mocks.js',
            'src/bower_components/angular-resource/angular-resource.js',
            'src/bower_components/angular-cookies/angular-cookies.js',
            'src/bower_components/angular-sanitize/angular-sanitize.js',
            'src/bower_components/angular-route/angular-route.js',

            // ngDialog
            'src/bower_components/ngDialog/js/ngDialog.js',

            // localization
            'src/bower_components/origin-i18n/dist/origin-i18n.js',

            // javascript
            'src/*.js',
            'src/directives/**/scripts/*.js',
            'src/directives/home/**/scripts/*.js',
            //'src/factories/*.js',

            // unit tests
            'test/spec/**/*.js',

            // templates
            'dist/views/*.html'

        ],

        // list of files / patterns to exclude
        exclude: [],

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
        browsers: ['Chrome'],


        // Continuous Integration mode
        // if true, it capture browsers, run tests and exit
        singleRun: false,


        ngHtml2JsPreprocessor: {
            moduleName: 'templates'
        }

    });
};
