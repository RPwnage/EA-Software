/**
 * Routing for the game library application
 */
(function() {
    'use strict';

    function routing($stateProvider, OriginConfigInjector, NavigationCategories) {

        $stateProvider
            .state('app.game_gamelibrary', {
                'url': '/game-library?searchString',
                data: {
                    section: NavigationCategories.gamelibrary,
                    pageTransitionEnabled: false,
                    scrollToTopOnLoad: false
                },
                'views': {
                    'content@': {
                        templateProvider: OriginConfigInjector.resolveTemplate('game-library'),
                        'resolve': {
                            'ensureAuth': OriginConfigInjector.waitForAuthReady(),
                            'ensureData': OriginConfigInjector.waitForGamesData()
                        },
                        'controller': 'GameLibraryCtrl'
                    }
                }
            })
            .state('app.game_gamelibrary.ogd', {
                'url': '/ogd/:offerid',
                'data': {
                    section: NavigationCategories.gamelibrary,
                    onOwnedGameDetails: true
                },
                'views': {
                    'slideout': {
                        'templateUrl': '/views/ogd.html',
                        'controller': 'GameLibraryOgdCtrl',
                        'resolve': {
                            'ensureData': OriginConfigInjector.checkForOgdRedirect()
                        }
                    }
                }
            })
            .state('app.game_gamelibrary.ogd.topic', {
                'url': '/:topic',
                'views': {
                    'ogdtabs': {
                        'templateUrl': '/views/ogdtabs.html',
                        'controller': 'GameLibraryOgdCtrl'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());
