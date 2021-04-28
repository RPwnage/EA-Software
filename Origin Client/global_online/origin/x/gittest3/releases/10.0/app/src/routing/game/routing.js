/**
 * Routing for the game library application
 */
(function() {
    /* globals ensureAuthenticatedProvider: true */
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app.game_gamelibrary', {
                'url': 'game-library',
                data:{
                	section: 'gamelibrary'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/mygames.html',
                        'controller': 'GameLibraryCtrl',
                        'resolve': {
                            'ensureAuth': ensureAuthenticatedProvider
                        }
                    }
                }
            })
            .state('app.game_gamelibrary.ogd', {
                'url': '/ogd/:offerid',
                data:{
                	section: 'gamelibrary'
                },
                'views': {
                    'slideout': {
                        'templateUrl': 'views/ogd.html',
                        'controller': 'GameLibraryOgdCtrl'
                    }
                }
            })
            .state('app.game_unavailable', {
            	data:{
                	section: 'gamelibrary'
                },
                'url': 'unavailable',
                'views': {
                    'content@': {
                        'templateUrl': 'views/mygamesanon.html',
                        'controller': 'GameLibraryAnonCtrl'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());