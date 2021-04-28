/**
 * Routing for unveiling gifted item(s)
 */
(function() {
    'use strict';

    function routing($stateProvider, NavigationCategories) {

        $stateProvider
            .state('app.gift_unveiling', {
                'url': '/gift/receive/:offerId?fromSPA',
                data: {
                    section: NavigationCategories.gamelibrary
                },
                views: {
                    'giftunveiling@': {
                        template: '<origin-unveiling></origin-unveiling>'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());
