/**
 * Routing for the home application
 */

(function() {
    /* globals ensureAuthenticatedProvider: true */
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app.home_home', {
                'url': 'home{trigger:string}',
                data:{
                	section: 'home'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/home.html',
                        'controller': 'HomeCtrl',
                        'resolve': {
                            'ensureAuth': ensureAuthenticatedProvider
                        }
                    }
                }
            })
            .state('app.home_welcome', {
                'url': 'welcome',
                data:{
                	section: 'home'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/homeanon.html',
                        'controller': 'HomeAnonCtrl'
                    }
                }
            })
            .state('app.home_offline', {
                'url': 'offline',
                data:{
                	section: 'home'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/homeoffline.html',
                        'controller': 'HomeOfflineCtrl'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());