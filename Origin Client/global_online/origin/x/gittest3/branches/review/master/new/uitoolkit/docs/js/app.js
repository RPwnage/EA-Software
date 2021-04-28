angular.module('app', ['components', 'ngRoute'])
    .config(function($routeProvider) {
        $routeProvider
            .when('/', {
                templateUrl: 'templates/otk/main.html'
            })
            .when('/buttons', {
                templateUrl: 'templates/otk/buttons.html'
            })
            .when('/typography', {
                templateUrl: 'templates/otk/typography.html'
            })
            .when('/forms', {
                templateUrl: 'templates/otk/forms.html'
            })
            .when('/icons', {
                templateUrl: 'templates/otk/icons.html'
            })
            .when('/carousels', {
                templateUrl: 'templates/otk/carousels.html'
            })
            .when('/dropdowns', {
                templateUrl: 'templates/otk/dropdowns.html'
            })
            .when('/modals', {
                templateUrl: 'templates/otk/modals.html'
            })
            .when('/navs', {
                templateUrl: 'templates/otk/navs.html'
            })
            .when('/progress', {
                templateUrl: 'templates/otk/progress.html'
            })
            .when('/presence', {
                templateUrl: 'templates/otk/presence.html'
            })
            .when('/tooltips', {
                templateUrl: 'templates/otk/tooltips.html'
            })
            .when('/components', {
                templateUrl: 'templates/components.html'
            })
            .when('/avatars', {
                templateUrl: 'templates/otk/avatars.html'
            })
            .when('/badges', {
                templateUrl: 'templates/otk/badges.html',
                controller: 'BadgesCtrl'
            })
            .when('/builder', {
                templateUrl: 'templates/builder.html',
                controller: 'BuilderCtrl'
            })
    });