// Karma configuration
// http://karma-runner.github.io/0.10/config/configuration-file.html

module.exports = function(config) {
  config.set({
    // base path, that will be used to resolve files and exclude
    basePath: '',

    // testing framework to use (jasmine/mocha/qunit/...)
    frameworks: ['jasmine'],

    // list of files / patterns to load in the browser
    files: [
      'dist/bower_components/moment/moment.js',
      'dist/bower_components/moment-timezone/builds/moment-timezone-with-data.min.js',
      'dist/bower_components/lodash/lodash.js',
      'dist/bower_components/promise-polyfill/Promise.js',
      'dist/bower_components/angular/angular.js',
      'dist/bower_components/angular-mocks/angular-mocks.js',
      'dist/bower_components/angular-resource/angular-resource.js',
      'dist/bower_components/angular-cookies/angular-cookies.js',
      'dist/bower_components/angular-sanitize/angular-sanitize.js',
      'dist/bower_components/angular-route/angular-route.js',
      '.tmp/constants/**.js',
      'test/framework/originapp-standalone.js',
      'src/telemetry/**/*.js',
      'test/spec/**/*.js'
    ],

    // list of files / patterns to exclude
    exclude: [],

    // web server port
    port: 8080,

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

      // to avoid DISCONNECTED messages
      captureTimeout: 60000,
      browserDisconnectTimeout : 10000, // default 2000
      browserDisconnectTolerance : 1, // default 0
      browserNoActivityTimeout : 60000, //default 10000

    // Continuous Integration mode
    // if true, it capture browsers, run tests and exit
    singleRun: false,

    //this directive generates coverage reports
    reporters: ['progress', 'coverage', 'junit'],

    //instrument test coverage on our components package
    preprocessors: {
        'src/telemetry/**/*.js': ['coverage'],
    },

    coverageReporter:{
        type: 'html',
        dir: 'tmp/coverage_report'
    },

    junitReporter: {
        outputDir: 'tmp/junit' // results will be saved as $outputDir/$browserName.xml
    }
  });
};
