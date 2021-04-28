/**
 * Routing for the settings application
 */

(function() {
    'use strict';
	
    function routing($stateProvider) {

        function waitForAuthReady(AuthFactory) {
            return AuthFactory.waitForAuthReady();
        }

		$stateProvider
            .state('app.settings', {
                'url': '/settings?page',
                reloadOnSearch: true,
                data: {
                	section: 'settings'
                },
                'views': {
                    'content@': {
                        'templateUrl': 'views/settings.html',
                        'resolve': {
                            'ensureAuth': waitForAuthReady
                        }
                    }
                }
            })
            .state('app.settings_unavailable', {
            	data:{
                	section: 'settings'
                },
                'url': '/unavailable',
                'views': {
                    'content@': {
                        'templateUrl': 'views/settings.html',
                        'controller': 'SettingsCtrl'
                    }
                }
            });			
    }

    angular.module('origin-routing')
        .config(routing);
}());
