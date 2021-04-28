/**
 * @file app.js
 */
(function() {
    'use strict';

    var DEFAULT_APP_VERSION = '10.0.1.0001';

    /**
     * @function
     * @requires ComponentsConfigFactory
     * @description sets the base path for where to find component templates
     */
    function setComponentsBasePath(ComponentsConfigFactory) {
        ComponentsConfigFactory.setBasePath('bower_components/origin-components/');
    }

    /**
     * @function
     * @description hook ourselves into the global error handler so that we can send telemetry
     */
    function GlobalErrorHandlingAndTelemetryInit ($window, $exceptionHandler, TelemetryFactory) {

        // Get a reference to the original error handler, if it exists, so we
        // don't overwrite any error-handling functionality that might be added
        // by a 3rd-party script.
        var originalErrorHandler = $window.onerror;
        // If there is no existing global error handler, let's define a mock one
        // so that our custom error handler code can be handled uniformly.
        if ( ! originalErrorHandler ) {
            originalErrorHandler = function mockHandler() {
                // By returning True, we prevent the browser's default error handler.
                return( true );
            };
        }
        // Define our custom error handler that will pipe errors into the core
        // $exceptionHandler service (where they may be further processed).
        // --
        // NOTE: Only message, fileName, and lineNumber are standardized.
        // columnNumber and error are not standardized values and will not be
        // present in all browsers. That said, they appear to work in all of the
        // browsers that "matter".
        $window.onerror = function handleGlobalError (message, fileName, lineNumber, columnNumber, error) {
            var date = new Date (),
                errorData = {
                    url: window.location.href,
                    message: message
                },
                errorDataStr = '';


            // If this browser does not pass-in the original error object, let's
            // create a new error object based on what we know.
            if ( ! error ) {
                error = new Error( message );
                // NOTE: These values are not standard, according to MDN.
                error.fileName = fileName;
                error.lineNumber = lineNumber;
                error.columnNumber = ( columnNumber || 0 );

                errorData.fileName = fileName;
                errorData.lineNumber = lineNumber;
                errorData.columnNumber = ( columnNumber || 0 );
            } else {
                errorData.stack = error.stack;
            }

            errorData.time =  date.toUTCString();
            errorDataStr = JSON.stringify(errorData);

            Origin.telemetry.sendErrorEvent('[GLOBALERROR]'+ message, errorDataStr, {type: 'GLOBAL'});

            // Pass the error off to our core error handler.
            $exceptionHandler( error );
            // Pass of the error to the original error handler.
            try {
                return( originalErrorHandler.apply( $window, arguments ) );
            } catch ( applyError ) {
                $exceptionHandler( applyError );
            }
        };

        //TODO: we should be retrieving the appversion from a config and passing that in
        TelemetryFactory.init(DEFAULT_APP_VERSION);
    }

    /**
     * @function
     * @description finishes up any initialization needed after angular has bootstrapped
     */
    function postBootstrapInit(GamesDataFactory, AuthFactory, LocFactory, LOCDICTIONARY, CPPAppDialogFactory, ReloginFactory, PopOutFactory, APPCONFIG) {
        AuthFactory.init(APPCONFIG.auth);
        LocFactory.init(LOCDICTIONARY.defaults.dictionary);
        CPPAppDialogFactory.init();
        ReloginFactory.init();
        PopOutFactory.init();
    }

    /**
     * @function
     * @description goes and fetches any asynchronous data we need and puts it in the angular constant
     */
    function getBootstrapAsyncData() {
        var bootstrapFactory = angular.injector(['ng', 'bootstrap', 'origin-app-config', 'origin-components-config', 'origin-app-loc-dictionary', 'ngCookies']).get('BootstrapInitFactory');

        return bootstrapFactory.init();
    }

    /**
     * @function
     * @description kicks off the origin app
     */
    function bootstrapApplication() {
        //if the dom is ready, the  bootstrap call is made right away, otherwise once the dom is ready we will bootstrap angular
        angular.element(document).ready(function() {
            angular.bootstrap(document, ['originApp']);
        });
    }

    /**
     * @function
     * @description handles init exeception
     */
    function handleInitError(error) {
        var logger = angular.injector(['ng', 'bootstrap']).get('LogFactory');
        if (logger) {
            logger.error('[INIT:boostrap init failed]', error.stack);
        }
    }

    /**
     * @function
     * @description overrides the default exception handler
     */
    function overrideExceptionHandler($provide) {
       $provide.decorator('$exceptionHandler', ['$delegate', function($delegate) {
            return function(exception, cause) {
                $delegate(exception, cause);
            };
        }]);
    }

    /**
     * @ngdoc object
     * @name originApp
     * @requires ngCookies
     * @requires ngResource
     * @requires ngSanitize
     * @requires ngRoute
     * @requires origin-routing
     * @requires origin-components
     * @requires origin-i18n
     * @requires origin-app-config
     * @requires origin-app-loc-dictionary
     * @description
     *
     * The application entry point
     */
    angular.module('originApp', [
        'ngCookies',
        'ngResource',
        'ngSanitize',
        'ngRoute',
        'ngTouch',
        'origin-components',
        'origin-i18n',
        'origin-app-config',
        'origin-app-loc-dictionary',
        'origin-routing',
    ])
        .config(overrideExceptionHandler)
        .run(setComponentsBasePath)
        .run(GlobalErrorHandlingAndTelemetryInit)
        .run(postBootstrapInit);

    getBootstrapAsyncData()
        .then(bootstrapApplication)
        .catch(handleInitError);
}());