/**
 * Routing for service/user error responses
 */
(function() {
    'use strict';

    function routing($stateProvider, OriginConfigInjector, NavigationCategories) {

        $stateProvider
            .state('app.error_notfound', {
                'url': '/404',
                data:{
                    section: NavigationCategories.error
                },
                'views': {
                    'content@': {
                        templateProvider: OriginConfigInjector.resolveTemplate('not-found'),
                        resolve: {
                            waitForGamesData: OriginConfigInjector.waitForGamesData(),
                            waitForAuthReady: OriginConfigInjector.waitForAuthReady()
                        }
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());
