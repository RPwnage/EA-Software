/**
 * Routing for the store application
 */
(function() {
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app.search', {
                'url': '/search?searchString&category',
                data: {
                    section: 'search'
                },
                'views': {
                    'content@': {
                        templateUrl: 'views/search.html'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
})();