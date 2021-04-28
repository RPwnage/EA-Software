/**
 * Routing for the settings application
 */

(function() {
    'use strict';
    function routing($stateProvider, OriginConfigInjector, NavigationCategories) {

		$stateProvider
            .state('app.settings', {
                'url': '/settings',
                reloadOnSearch: true,
                data: {
                	section: NavigationCategories.settings
                },
                'views': {
                    'content@': {
                        'templateUrl': '/views/settings.html',
                        'resolve': {
                            'ensureAuth': OriginConfigInjector.waitForAuthReady()
                        }
                    }
                }
            })
            .state('app.settings.page', {
                'url': '/:page',
                'data': {
                    section: NavigationCategories.settings,
                    pageTransitionEnabled: false
                },
                'views': {}
            })
            .state('app.settings_unavailable', {
            	data:{
                	section: NavigationCategories.settings
                },
                'url': '/unavailable',
                'views': {
                    'content@': {
                        'templateUrl': '/views/settings.html',
                        'controller': 'SettingsCtrl'
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
}());
