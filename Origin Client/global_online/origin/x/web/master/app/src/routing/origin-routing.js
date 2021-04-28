/**
 * The routing ng module
 */
(function() {

    'use strict';

    function sendRouteChangeTelemetry(toState, fromState) {
        var params = {};
        params.toName = toState.name;
        params.fromState = fromState.url;
        params.fromName = fromState.name;
        Origin.telemetry.sendPageView(toState.url, params.toName, params);
    }

    /**
     * Initialize the origin-components compatibility module.
     * @param  {Object} $rootScope     instance of $rootScope from angular
     * @param  {Object} $state         instance of $state from ui.router
     * @param  {Object} AppCommFactory instance of AppCommFactory from origin-components
     */
    function initEventHandlers($rootScope, $state, AppCommFactory) {
        $rootScope.$on('$stateChangeStart', function(event, toState, toParams, fromState, fromParams) {
            AppCommFactory.events.fire('uiRouter:stateChangeStart', event, toState, toParams, fromState, fromParams);
        });

        $rootScope.$on('$stateChangeSuccess', function(event, toState, toParams, fromState, fromParams) {
            AppCommFactory.events.fire('uiRouter:stateChangeSuccess', event, toState, toParams, fromState, fromParams);
            sendRouteChangeTelemetry(toState, fromState);
        });

        $rootScope.$on('$stateChangeError', function(event, toState, toParams, fromState, fromParams, error) {
            AppCommFactory.events.fire('uiRouter:stateChangeError', event, toState, toParams, fromState, fromParams, error);
        });

        AppCommFactory.events.on('uiRouter:go', function(toName, toParams, toOptions) {
            $state.go(toName, toParams, toOptions);
        });
    }

    /**
     * Get the configuration for origin-routing
     * @param {Object} $urlRouterProvider an instance of $urlRouerProvider from ui.router
     */
    function getConfiguration($urlRouterProvider) {
        $urlRouterProvider
            .when('', '/home')
            .when('/', '/home')
            .when('/store', '/store/')
            .when('/game-library/ogd/:offerid', '/game-library/ogd/:offerid/overview')
            .otherwise('/404');
    }

    angular
        .module('origin-routing', ['ui.router', 'origin-app-config', 'origin-components'])
        .config(getConfiguration)
        .run(initEventHandlers);
}());