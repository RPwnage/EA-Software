/**
 * Routing for the store application
 */
(function() {
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app.store', {
            	data:{
                	section: 'store'
                },
                'views': {
                    'content@': {
                        templateUrl: 'views/store.html'
                    }
                }
            })
            .state('app.store.wrapper', {
                'views': {
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
            .state('app.store.wrapper.home', {
                'url': 'store',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/store-home.html'
                    }
                }
            })
            .state('app.store.wrapper.freegames', {
                'url': 'store/freegames',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/free-games.html'
                    }
                }
            })
            .state('app.store.wrapper.onthehouse', {
                'url': 'store/freegames/onthehouse',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/on-the-house.html'
                    }
                }
            })
            .state('app.store.wrapper.gametiles', {
                'url': 'store/storegametiles',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/storegametiles.html'
                    }
                }
            })
            .state('app.store.wrapper.packs', {
                'url': 'store/storepacks',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/storepacks.html'
                    }
                }
            })
            .state('app.store.wrapper.carousel', {
                'url': 'store/storeCarousel',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/carousel.html'
                    }
                }
            })
            .state('app.store.wrapper.promos', {
                'url': 'store/storePromos',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/promos.html'
                    }
                }
            })
            .state('app.store.wrapper.agegate', {
                'url': 'store/storeAgegate',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/agegate.html'
                    }
                }
            })
            .state('app.store.wrapper.socialmedia', {
                'url': 'store/socialmedia',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/socialmedia.html'
                    }
                }
            })
            .state('app.store.wrapper.productcarousel', {
                'url': 'store/storeProductCarousel',
                'views': {
                    'storecontent' : {
                    	templateUrl: 'views/store/productcarousel.html'
                    }
                }
            })
            .state('app.store.wrapper.pdp', {
                'url': 'store/pdpInfo/:offerid',
                'views': {
                    'storecontent' : {
                        templateUrl: 'views/store/pdpInfo.html'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
})();