/**
 * Routing for the home application
 */

(function() {
    'use strict';

    function routing($stateProvider, OriginConfigInjector, NavigationCategories) {

        $stateProvider
            .state('app.home_home', { 
                url: '/my-home?source',
                data: {
                    section: NavigationCategories.home
                },
                'views' : {
                    'content@': {
                        'resolve' : {
                            'ensureAuth' : OriginConfigInjector.waitForAuthReady()
                        },
                        'controller': 'HomeCtrl'
                    }
                }
            })
            .state('app.home_loggedin', {
                url: '/my-home?source',
                data: {
                    section: NavigationCategories.homeloggedin
                },
                'views': {
                    'content@': {
                        templateProvider: OriginConfigInjector.resolveTemplate('home'),
                        //For local changes to home.html route here
                        //'templateUrl': '/views/home.html',
                        'resolve': {
                            'ensureAuth': OriginConfigInjector.waitForAuthReady()
                        },
                        'controller': 'HomeloggedinCtrl'
                    }
                },
            });
    }
    angular.module('origin-routing')
        .config(routing);
}());