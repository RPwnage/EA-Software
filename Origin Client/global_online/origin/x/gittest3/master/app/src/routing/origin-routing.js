/**
 * The routing ng module
 */
(function() {
    'use strict';


    // Prototype run level idea for first load
    // where can we factor this into the codebase?
    // Since this is a reflection of origin.auth.login, maybe that promise can
    // work for this use case
    Origin.appSession = {
        //becasue there's no way to get the firsttimejssdk login state,
        //app.js sets this var at the auth:ready time
        ensureAuth: false
    };
    // might be better to have ensureRunlevel('Authenticated') promise
    window.ensureAuthenticatedProvider = function() {
        return new Promise(function(resolve) {
            if (Origin.appSession.ensureAuth === true) {
                resolve();
            } else {
                var handle = setInterval(function() {
                    if (Origin.appSession.ensureAuth === true) {
                        resolve();
                        clearInterval(handle);
                    }
                }, 100);
            }

        });
    };

    angular
        .module('origin-routing', ['ui.router'])
        .config(function($urlRouterProvider) {
            $urlRouterProvider.otherwise('/home');
        });
}());