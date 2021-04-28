/**
 * The base shell for our applications
 */
(function() {
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app', {
                'views': {
                    'shell': {
                        templateUrl: 'views/shell.html'
                    },
                    'social': {
                        templateUrl: 'views/social.html'
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