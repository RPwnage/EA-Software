/**
 * Routing for service/user error responses
 */
(function() {
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app.error_notfound', {
                'url': '404',
                data:{
                	section: 'error'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/notfound.html'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());