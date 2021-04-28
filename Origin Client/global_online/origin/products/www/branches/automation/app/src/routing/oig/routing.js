/**
 * Routing for the oig application
 */
(function() {
    'use strict';

    function routing($stateProvider, OriginConfigInjector) {


        $stateProvider
            .state('app.oig-addon-store', {
                url: '/oig-addon-store/:profile/:masterTitleId?:filterOfferIds',
                data: {
                    pageTransitionEnabled: false
                },
                views: {
                    'content@': {
                        'templateUrl': '/views/oig/addonstore.html'
                    }
                }
            })
            .state('app.oig-checkout', {
                url: '/oig-checkout/:profile/:masterTitleId/:offerId',
                data: {
                    pageTransitionEnabled: false
                },
                views: {
                    'content@': {
                        'templateUrl': '/views/oig/checkout.html'
                    }
                }
            })
            .state('app.oig-checkout-confirmation', {
                url: '/oig-checkout-confirmation/:offerIds',
                data: {
                    pageTransitionEnabled: false
                },
                views: {
                    'content@': {
                        'templateUrl': '/views/oig/checkoutconfirmation.html'
                    }
                }
            })
            .state('app.oig-subscription-nux', {
                url: '/oig-subscription-nux',
                data: {
                    pageTransitionEnabled: false
                },
                views: {
                    'content@': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oig-nux')
                    }
                }
            })
            .state('app.oig-download-manager', {
                url: '/oig-download-manager',
                data: {
                    pageTransitionEnabled: false
                },
                views: {
                    'content@': {
                        'templateUrl': '/views/oig/downloadmanager.html'
                    }
                }
            })
            .state('app.oig-subscription-interstitial', {
                url: '/oig-subscription-interstitial',
                data: {
                    pageTransitionEnabled: false
                },
                views: {
                    'content@': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-interstitial')
                    }
                }
            });

    }

    angular.module('origin-routing')
        .config(routing);
})();
