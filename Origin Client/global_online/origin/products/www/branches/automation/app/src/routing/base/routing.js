/**
 * The base shell for our applications
 */
(function () {
    'use strict';

    function routing($stateProvider, OriginConfigInjector, OriginPassThroughQueryParams) {

        $stateProvider
            .state('app', {
                url: '?' + OriginPassThroughQueryParams.join('&'), // put all params that should be passed through here
                abstract: true,
                data: {
                    pageTransitionEnabled: true,
                    scrollToTopOnLoad: true
                },
                resolve : {
                    waitfordictionary : OriginConfigInjector.waitForDictionary()
                },
                views: {
                    sitestripes: {
                        templateProvider: OriginConfigInjector.resolveTemplate('global-sitestripe')
                        //For local changes to sitestripes.html route here
                        //templateUrl: '/views/sitestripes.html'
                    },
                    shell: {
                        templateProvider: OriginConfigInjector.resolveTemplate('shell-navigation'),
                        controller: 'ShellCtrl'
                    },
                    social: {
                        controller: 'SocialCtrl',
                        templateUrl: '/views/social.html'
                    },
                    socialmedia: {
                        templateUrl: '/views/socialmedia.html'
                    },
                    dialog: {
                        templateUrl: '/views/dialog.html'
                    },
                    offlineflyout: {
                        templateUrl: '/views/offlineflyout.html'
                    },
                    'footer@': {
                        templateProvider: function(TemplateResolver, $stateParams){
                            if (!Origin.client.isEmbeddedBrowser()) {
                                return TemplateResolver.getTemplate('footer-web', $stateParams);
                            }
                        }
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());
