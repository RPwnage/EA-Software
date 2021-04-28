/**
 * Routing for the store application
 */
(function() {
    'use strict';

    function routing($stateProvider, NavigationCategories, OriginConfigInjector) {
        $stateProvider
            .state('app.search', {
                'url': '/search?searchString&category',
                data: {
                    section: NavigationCategories.search,
                    pageTransitionEnabled: false
                },
                'views': {
                    'content@': {
                        templateProvider: OriginConfigInjector.resolveTemplate('search')
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
})();
