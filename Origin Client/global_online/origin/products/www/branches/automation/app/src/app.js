/**
 * @file app.js
 */

/* globals ANGULARDEBUG: false */
(function() {
    'use strict';

    var TRACKER_GA = 'UA-49146401-5';
    var HQ_WIDTH = 1920;
    var DOMAIN  = '.origin.com';
    var PERCENTAGE_PLACEHOLDER = '{{PERCENTAGE}}';
    var PERCENTAGE_CHARACTER = '%';
    var PERCENTAGE_REPLACEMENT_REGEX = new RegExp(PERCENTAGE_CHARACTER, 'g');
    var PERCENTAGE_PLACEHOLDER_REPLACEMENT_REGEX = new RegExp(PERCENTAGE_PLACEHOLDER, 'g');

    /**
     * @function
     * @requires ComponentsConfigFactory
     * @description sets the base path for where to find component templates
     */
    function setComponentsBasePath(ComponentsConfigFactory) {
        ComponentsConfigFactory.setBasePath('/bower_components/origin-components/');
    }

    /**
     * @function
     * @description hook ourselves into the global error handler so that we can send telemetry
     */
    function globalErrorHandlingAndTelemetryInit($window, $exceptionHandler, TelemetryFactory) {

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

        TelemetryFactory.init();

        // send boot start
        TelemetryFactory.sendBootStartEvent();
    }


    /**
     * @function
     * @description finishes up any initialization needed after angular has bootstrapped
     */
    function postBootstrapInit(AuthFactory, CPPAppDialogFactory, ReloginFactory, PopOutFactory, ExperimentFactory, APPCONFIG) {
        ExperimentFactory.init(APPCONFIG.overrides.env, APPCONFIG.overrides.cmsstage, originLocaleApi.getLocale().toLowerCase(), originLocaleApi.getThreeLetterLanguageCode())
            .catch(function(error) {
                Origin.telemetry.sendErrorEvent('[EXPERIMENT ERROR]: ExperimentFactory.init failed', error.message, {type: 'GLOBAL'});
            });
        AuthFactory.init(APPCONFIG.auth);
        CPPAppDialogFactory.init();
        ReloginFactory.init();
        PopOutFactory.init();
    }

    /**
     * @function
     * @description goes and fetches any asynchronous data we need and puts it in the angular constant
     */
    function getBootstrapAsyncData() {
        return OriginKernel.Bootstrap.init().catch(handleInitError);
    }

    /**
     * @function
     * @description kicks off the origin app
     */
    function bootstrapApplication() {
        //if the dom is ready, the  bootstrap call is made right away, otherwise once the dom is ready we will bootstrap angular
        angular.element(document).ready(function() {
            Origin.performance.setMarker(OriginPerfConstant.markers.ANGULAR_BOOTSTRAP_START);
            Origin.performance.setMeasure(OriginPerfConstant.measure.HEAD_TO_ANGULAR_START, OriginPerfConstant.markers.HEAD_PARSED, OriginPerfConstant.markers.ANGULAR_BOOTSTRAP_START);
            angular.bootstrap(document, ['originApp']);
        });
    }

    /**
     * @function
     * @description send GA event on the event that an error happens at getBootstrapAsyncData() or bootstrapApplication().  At this time, TelemetryFactory is
     * not initialized and cannot send telemetry data so we have hard code GA to send the event.
     */
    function sendGAInitError(error) {
        if (window.ga) {
            window.ga('create', TRACKER_GA, 'auto');
            window.ga('send', 'General', 'InitError', error.stack);
        }
    }

    /**
     * @function
     * @description handles init exeception
     */
    function handleInitError(error) {
        sendGAInitError(error);
        console.error('[INIT:boostrap init failed]', error.stack);
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
     * Escapes percentage and sanitize the HTML string. In IE, % breaks sanitizer
     *
     * @param {function} $sanitizer
     * @param {string} htmlString
     * @returns {string}
     */
    function escapePercentageAndSanitize($sanitizer, htmlString){
        var percentageEscapedHtml = htmlString.replace(PERCENTAGE_REPLACEMENT_REGEX, PERCENTAGE_PLACEHOLDER);
        percentageEscapedHtml = $sanitizer(percentageEscapedHtml);
        percentageEscapedHtml = percentageEscapedHtml.replace(PERCENTAGE_PLACEHOLDER_REPLACEMENT_REGEX, PERCENTAGE_CHARACTER);
        return percentageEscapedHtml;
    }

    /**
     * @function
     * @description When parsing invalid HTML for page content, catch the exception and log it
     */
    function catchNgSanitizeExceptions($provide) {
        $provide.decorator('$sanitize', ['$delegate', '$injector', function ($delegate, $injector) {
            var LogFactory = $injector.get('LogFactory');
            var HtmlTransformer = $injector.get('HtmlTransformer');

            return function(htmlText) {
                try {
                    var sanitizedHtml = escapePercentageAndSanitize($delegate, htmlText);
                    return HtmlTransformer.addOtkClasses(sanitizedHtml);
                }
                catch (e) {
                    if (LogFactory) {
                        LogFactory.error('[APP: ng-sanitize exception]', e.stack);
                    }
                    return '&lt;!-- HTML parse error --&gt;';
                }
            };
        }]);
    }

    /**
     * @function
     * @description turns on apply async for the app
     */
    function useApplyAsync($httpProvider) {
        $httpProvider.useApplyAsync(true);
    }

    /**
     * @function
     * @description disable debug info
     */


    function disableDebugInfo($compileProvider) {

        //uglify will define the ANGULARDEBUG constant, when it is defined and set to false
        //we will disable angular debug info
        //
        //if we are running unminified code, ANGULARDEBUG is not defined
        //and we will NOT disable angular debug mode
        if((typeof(ANGULARDEBUG) !== 'undefined') && !ANGULARDEBUG) {
            $compileProvider.debugInfoEnabled(false);
        }
    }


    /**
     * Handle click event for external links
     * @param  {object} event Click event passed from browser
     */
    function handleExternalLinkClick(event) {
        event.preventDefault();
        if (event.currentTarget.href) {
            Origin.windows.externalUrl(event.currentTarget.href);
        }
    }

    /**
     * Set global click binding for external links as specified by
     * a CSS class "external-link"
     */
    function initExternalLinkBinding() {
        angular.element('body').on('click','a.external-link', handleExternalLinkClick);
    }

    /**
     * Handle click event for external links with sso
     * @param  {object} event Click event passed from browser
     */
    function handleExternalSSOLinkClick(event) {
        event.preventDefault();
        if (event.currentTarget.href) {
            if (Origin.client.isEmbeddedBrowser()) {
                Origin.client.desktopServices.asyncOpenUrlWithEADPSSO(event.currentTarget.href);
            } else {
                window.open(event.currentTarget.href);
            }
        }
    }

    /**
     * Set global click binding for external links with sso as specified by
     * a CSS class "external-link-sso"
     */
    function initExternalSSOLinkBinding() {
        angular.element('body').on('click','a.external-link-sso', handleExternalSSOLinkClick);
    }

    /**
    * Add the language to the html element
    */
    function initHtmlLang() {
        var originLocaleApi = OriginLocale.parse(window.location.href, 'en-us'); // jshint ignore:line
        angular.element('html').attr('lang', originLocaleApi.getLanguageCode());
    }

    /**
     * Angular app boot complete
     */
    function bootDone(TelemetryFactory) {
        TelemetryFactory.bootDone();
    }

    /**
    * Enable HTML5 Mode
    * @param {object} $locationProvider
    */
    function enableHtml5Mode($locationProvider) {
        $locationProvider.html5Mode(true);
    }
    /**
     * Set a cookie to related to the screen resolution
     */
    function setResolutionCookie($cookies) {
        function isMobile() {
            return angular.isDefined(window.orientation);
        }

        function isHighQuality() {
            var width = screen.width > screen.height ? screen.width : screen.height;
            var pixelRatio = (window.devicePixelRatio || 1);

            width *= pixelRatio;
            return width > HQ_WIDTH && pixelRatio > 1;
        }

        if (screen) { // Headless browsers may not have a screen
            var value = isHighQuality() ? 'rc' : 'lc';
            value += isMobile() ? 'm' : 'g';
            var options = {
                domain: DOMAIN,
                path: '/'
            };
            $cookies.put('iq', value, options);
        }
    }

    /**
     * extends the $rootScope with a given function
     * @param $rootScope    The $rootScope
     * @param fnName        The name of the new function
     * @param fnFactory     A factory function that returns the implementation of the new function
     */
    function extendScopePrototype ($rootScope, fnName, fnFactory) {
        function addFnToScope(scope) {
            var fn = fnFactory(scope);
            if (!scope[fnName] && angular.isFunction(fn)) {
                scope[fnName] = fn;
            }
        }

        // force all $newed up scopes to have new function
        var scopePrototype = Object.getPrototypeOf($rootScope);
        var oldNew = scopePrototype.$new;
        scopePrototype.$new = function $new() {
            var scope = oldNew.apply(this, arguments);
            addFnToScope(scope);
            return scope;
        };

        // add to $rootScope
        addFnToScope($rootScope);
    }

    // Extend $scope with $watchOnce functionality
    function addWatchOnce($rootScope) {
        // Returns a function that implements $watchOnce
        function watchOnceFactory (scope) {
            /**
             * Watches the scope on a given attribute. When the watched attribute is populated with
             * a valid value the watch is unbound and the is executed with the new and old values.
             * An optional condition function can be passed that determines when the new attribute value 'is valid',
             * otherwise a default condition is used.
             *
             * ex. $scope.$watchOnce('offerId', function (offerId) {console.log(offerId);});
             *
             * @param {string} attr             Scope attribute to be watched
             * @param {function} callback       The function to be called when watched attr is valid
             * @param {boolean}  objectEquality (Optional) Compare for object equality using angular.equals instead of comparing for reference equality.
             * @param {function} conditionFn    (Optional) function that tests whether the new value is 'valid' and the watch can stop
             */
            function watchOnce (attr, callback, objectEquality, conditionFn) {
                    var unbind = scope.$watch(attr, function (newValue, oldValue) {
                        // use condition function if its a valid function, else use default condition
                        if (angular.isFunction(conditionFn)) {
                            if (conditionFn(newValue, oldValue)) {
                                unbind();
                                callback(newValue, oldValue);
                            }
                        } else if ( angular.isDefined(newValue) &&
                                    newValue !== '' &&
                                    // either newValue is not an object or that object is not empty
                                    (!angular.isObject(newValue) || !_.isEmpty(newValue))) {
                            unbind();
                            callback(newValue, oldValue);
                        }
                    }, objectEquality);
            }

            return watchOnce;
        }

        if (!$rootScope.$watchOnce){
            extendScopePrototype($rootScope, '$watchOnce', watchOnceFactory);
        }
    }

    function initProductNavFactory(ProductNavigationFactory) {
        ProductNavigationFactory.init();
    }

    function initTelemetry(TelemetryTrackerFactory) {
        TelemetryTrackerFactory.init();
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
     * @description
     *
     * The application entry point
     */
    angular.module('originApp', [
        'ngResource',
        'ngSanitize',
        'ngRoute',
        'ngTouch',
        'ngIdle',
        'ngclipboard',
        'origin-components',
        'origin-i18n',
        'origin-app-version',
        'origin-routing',
        'origin-telemetry-config'
    ])
        .constant('APPCONFIG', OriginKernel.APPCONFIG)
        .config(enableHtml5Mode)
        .config(overrideExceptionHandler)
        .config(catchNgSanitizeExceptions)
        .config(useApplyAsync)
        .config(disableDebugInfo)
        .run(setResolutionCookie)
        .run(setComponentsBasePath)
        .run(globalErrorHandlingAndTelemetryInit)
        .run(postBootstrapInit)
        .run(initExternalLinkBinding)
        .run(initExternalSSOLinkBinding)
        .run(initHtmlLang)
        .run(addWatchOnce)
        .run(initProductNavFactory)
        .run(initTelemetry)
        // should run last
        .run(bootDone);

    getBootstrapAsyncData()
        .then(bootstrapApplication)
        .catch(handleInitError);
}());
