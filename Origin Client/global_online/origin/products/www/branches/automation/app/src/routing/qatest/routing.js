/**
 * Routes for QA test pages
 */
(function () {
    'use strict';
    function routing($stateProvider, OriginConfigInjector) {
        $stateProvider
            .state('app.cq-path', {
                'url': '/{cqPath:.*}?searchString&fq&sort',
                views: {
                    'content@': {
                        templateProvider: OriginConfigInjector.resolveTemplate('cq-path'),
                        'resolve': {
                            waitForGamesData: OriginConfigInjector.waitForGamesData(),
                            ensureAuth : OriginConfigInjector.waitForAuthReady()
                        }
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());
