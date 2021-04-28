/**
 * Routing for the profile page
 */

(function() {
    'use strict';


    function routing($stateProvider, NavigationCategories) {
        $stateProvider
            .state('app.profile', {
                url: '/profile',
                data: {
                    section: NavigationCategories.profile
                },
                'views': {
                    'content@': {
                        'templateUrl': '/views/profile.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            })
            .state('app.profile.topic', {
                url: '/:topic',
                data: {
                    pageTransitionEnabled: false
                },
                'views': {
                    'profiletabs': {
                        'templateUrl': '/views/profiletabs.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            })
            .state('app.profile.topic.details', {
                url: '/:detailsId',
                'views': {
                    'profiletabs@app.profile': {
                        'templateUrl': '/views/profiletabs.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            })
            .state('app.profile.user', {
                url: '/user/:id',
                'views': {
                    'content@': {
                        'templateUrl': '/views/profile.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            })
            .state('app.profile.user.topic', {
                url: '/:topic',
                data: {
                    pageTransitionEnabled: false
                },
                'views': {
                    'profiletabs': {
                        'templateUrl': '/views/profiletabs.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            })
            .state('app.profile.user.topic.details', {
                url: '/:detailsId',
                'views': {
                    'profiletabs@app.profile.user': {
                        'templateUrl': '/views/profiletabs.html',
                        'controller': 'ProfileCtrl'
                    }
                }
            })
            .state('app.wishlistredirector', {
                url: '/view-wishlist/:wishlistId',
                'views': {
                    'content@': {
                        'templateUrl': '/views/wishlistredirector.html',
                        'controller': 'WishlistRedirectorCtrl'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());
