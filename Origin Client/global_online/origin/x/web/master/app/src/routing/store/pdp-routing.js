/**
 * Routing for the store PDP
 */
(function() {
    'use strict';

    function routing($stateProvider) {
        $stateProvider
            .state('app.store.wrapper.videopdp', {
                url: '/videopdp/:franchise/:game/:edition?subscriber',
                views: {
                    'storecontent': {
                        templateUrl: 'views/store/pdp2.html'
                    }
                }
            })
            .state('app.store.wrapper.pdp', {
                url: '/:franchise/:game/:edition?subscriber',
                views: {
                    'storecontent': {
                        templateUrl: 'views/store/pdp.html'
                    }
                }
            })
            .state('app.store.wrapper.noimagepdp', {
                url: '/noimage/:franchise/:game/:edition?subscriber',
                views: {
                    'storecontent': {
                        templateUrl: 'views/store/pdp3.html'
                    }
                }
            })
            .state('app.store.wrapper.skupdp', {
                url: '/pdp/:offerid?subscriber',
                views: {
                    'storecontent': {
                        templateUrl: 'views/store/pdp.html'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
})();
