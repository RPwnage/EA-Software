/**
 * The base shell for our applications
 */
(function() {
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app', {
                'url': '/',
                'views': {
                    'shell': {
                        templateUrl: 'views/shell.html'
                    },
                    'social': {
                        templateUrl: 'views/social.html'
                    },
                    'tracking': {
                        templateUrl: 'views/tracking.html'
                    },
                    'socialmedia': {
                        templateUrl: 'views/socialmedia.html'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());