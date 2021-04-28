/**
 * Routing for the home application
 */

(function() {
    'use strict';

    function routing($stateProvider, APPCONFIG) {

        function waitForAuthReady(AuthFactory) {
            return AuthFactory.waitForAuthReady();
        }

        $stateProvider
            .state('app.home_home', {
                'url': '/home{trigger:string}',
                data: {
                    section: 'home'
                },
                'views': {
                    'content@': {
                        //we should look at moving these to a centralized place
                        'templateUrl': APPCONFIG.urls.homeUrl,
                        'resolve': {
                            'ensureAuth': waitForAuthReady
                        }
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());