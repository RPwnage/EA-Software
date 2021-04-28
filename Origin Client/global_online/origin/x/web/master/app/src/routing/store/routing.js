/**
 * Routing for the store application
 */
(function() {
    'use strict';

    function routing($stateProvider) {

        function waitForGamesDataReady(GamesDataFactory) {
            return GamesDataFactory.waitForGamesDataReady();
        }

        $stateProvider
            .state('app.store', {
                url: '/store',
                data:{
                    section: 'store'
                },
                views: {
                    'content@': {
                        templateUrl: 'views/store.html',
                        'resolve': {
                            'ensureGamesDataFactory': waitForGamesDataReady
                        }
                    }
                }
            })
            .state('app.store.wrapper', {
                views: {
                    'storewrapper' : {
                        template: '<div id="storecontent" ui-view="storecontent"></div>'
                    },
                    'storenav' : {
                        templateUrl: 'views/store/storenav.html'
                    },
                    'storemod' : {
                        templateUrl: 'views/store/sitestripes.html'
                    }
                }
            })
            .state('app.store.wrapper.browse', {
              'url': '/browse?searchString&fq&sort',
              'views': {
                  'storecontent': {
                    templateUrl: 'views/store/browse.html'
                  }
              }
            })
            .state('app.store.wrapper.home', {
                'url': '/',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/store-home.html'
                    }
                }
            })
            .state('app.store.wrapper.home.notakeover', {
                'url': '/',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/store-home.html'
                    }
                }
            })
            .state('app.store.wrapper.freegames', {
                url: '/freegames',
                data:{
                    section: 'store.freegames'
                },
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/free-games.html'
                    }
                }
            })
            .state('app.store.wrapper.onthehouse', {
                url: '/onthehouse',
                data: {
                    section: 'store.freegames'
                },
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/on-the-house.html'
                    }
                }
            })
            .state('app.store.wrapper.onthehouse2', {
                url: '/onthehouse-two-cards',
                data: {
                    section: 'store.freegames'
                },
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/on-the-house2.html'
                    }
                }
            })
            .state('app.store.wrapper.gametime', {
                url: '/gametime',
                data: {
                    section: 'store.freegames'
                },
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/game-time.html'
                    }
                }
            })
            .state('app.store.wrapper.demosbetas', {
                url: '/demosbetas',
                data: {
                    section: 'store.freegames'
                },
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/demos-betas.html'
                    }
                }
            })
            .state('app.store.wrapper.bundle-cybermonday', {
                url: '/bundle/cybermonday',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/cybermonday-bundle.html'
                    }
                }
            })
            .state('app.store.wrapper.bundle-blackfriday', {
                url: '/bundle/blackfriday',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/blackfriday-bundle.html'
                    }
                }
            })
            /*
            The following are just test pages and should go away soon
             */
            .state('app.store.wrapper.takeover1', {
                'url': '/takeover1',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/takeover1.html'
                    }
                }
            })
            .state('app.store.wrapper.takeover2', {
                'url': '/takeover2',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/takeover2.html'
                    }
                }
            })
            .state('app.store.wrapper.takeover3', {
                'url': '/takeover3',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/takeover3.html'
                    }
                }
            })
            .state('app.store.wrapper.takeover4', {
                'url': '/takeover4',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/takeover4.html'
                    }
                }
            })
            /*
            The following are just test pages and should go away soon
             */
            .state('app.store.wrapper.gametiles', {
                url: '/storegametiles',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/storegametiles.html'
                    }
                }
            })
            .state('app.store.wrapper.packs', {
                url: '/storepacks',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/storepacks.html'
                    }
                }
            })
            .state('app.store.wrapper.carousel', {
                url: '/storeCarousel',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/carousel.html'
                    }
                }
            })
            .state('app.store.wrapper.promos', {
                url: '/storePromos',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/promos.html'
                    }
                }
            })
            .state('app.store.wrapper.agegate', {
                url: '/storeAgegate',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/agegate.html'
                    }
                }
            })
            .state('app.store.wrapper.socialmedia', {
                url: '/socialmedia',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/socialmedia.html'
                    }
                }
            })
            .state('app.store.wrapper.dealsmodule', {
                'url': '/deals/module',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/deals-module.html'
                    }
                }
            })
            .state('app.store.wrapper.productcarousel', {
                url: '/storeProductCarousel',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/productcarousel.html'
                    }
                }
            })
			.state('app.store.wrapper.comparison', {
                url: '/comparison',
                views: {
                    'storecontent' : {
                        templateUrl: 'views/store/comparison.html'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
})();
