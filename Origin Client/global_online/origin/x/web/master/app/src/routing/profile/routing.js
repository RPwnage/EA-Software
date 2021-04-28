/**
 * Routing for the profile page
 */

(function() {
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app.profile', {
                url: '/profile',

                data: {
                    section: 'profile'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/profile.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            })
            .state('app.profile.user', {
                url: '/user/:id',
                data: {
                    section: 'profile'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/profile.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            });

    }

    angular.module('origin-routing')
        .config(routing);
}());