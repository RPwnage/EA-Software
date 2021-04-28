/**
 * Routing for the game library application
 */
(function() {
    'use strict';

    function routing($stateProvider) {

        function waitForAuthReady(AuthFactory) {
            return AuthFactory.waitForAuthReady();
        }

        $stateProvider
            .state('app.game_gamelibrary', {
                'url': '/game-library',
                data: {
                    section: 'gamelibrary'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/mygames.html',
                        'resolve': {
                            'ensureAuth': waitForAuthReady

                        }
                    }
                }
            })
            .state('app.game_gamelibrary.ogd', {
                'url': '/ogd/:offerid',
                'data': {
                    section: 'gamelibrary'
                },
                'views': {
                    'slideout': {
                        'templateUrl': 'views/ogd.html',
                        'controller': 'GameLibraryOgdCtrl'
                    }
                }
            })
            .state('app.game_gamelibrary.ogd.topic', {
                'url': '/:topic',
                'views': {
                    'ogdtabs': {
                        'templateUrl': 'views/ogdtabs.html',
                        'controller': 'GameLibraryOgdCtrl'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());