/**
 * Routing for the store PDP
 */
(function() {
    'use strict';

    function routing($stateProvider, OriginConfigInjector) {
        $stateProvider
            .state('app.store.wrapper.addon', {
                url: '/:franchise/:game/:type/:edition?:invoiceSource&:giftInvoiceSource&:wishlistInvoiceSource',
                'views' : {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('pdp')
                    }
                }
            })
            .state('app.store.wrapper.pdp', {
                url: '/:franchise/:game/:edition?:invoiceSource&:giftInvoiceSource&:wishlistInvoiceSource',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('pdp')
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
})();
