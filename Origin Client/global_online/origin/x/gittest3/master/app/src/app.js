/**
 * @file app.js
 */
(function() {
    'use strict';


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
     * @requires AppCommFactory
     * @requires AuthFactory
     * @requires UserDataFactory
     * @description sets the base path for where to find component templates
     */
    function setAuthListeners(AppCommFactory) {
        AppCommFactory.events.on('auth:ready', function(){
            Origin.appSession.ensureAuth = true;
        });
    }

    /**
     * @function
     * @description finishes up any initialization needed after angular has bootstrapped
     */
    function postBootstrapInit(GamesDataFactory, AuthFactory, LocFactory, LOCDICTIONARY, CPPAppDialogFactory) {
        AuthFactory.init();
        LocFactory.init(LOCDICTIONARY.defaults.dictionary);
        CPPAppDialogFactory.init();
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
        if(logger) {
            logger.error('[INIT:boostrap init failed]', error.stack);
        }
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
            'origin-routing',
            'origin-components',
            'origin-i18n',
            'origin-app-config',
            'origin-app-loc-dictionary',
        ])
        .run(setComponentsBasePath)
        .run(setAuthListeners)
        .run(postBootstrapInit);
    console.time('timeToFirstAction');
    getBootstrapAsyncData()
        .then(bootstrapApplication)
        .catch(handleInitError);
}());