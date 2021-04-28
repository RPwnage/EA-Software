angular.module('app')
    .controller('NavCtrl', function($scope, $location) {
        $scope.navItems = [
            {'href': '#/badges', 'label': 'Badges'},
            {'href': '#/buttons', 'label': 'Buttons'},
            {'href': '#/forms', 'label': 'Forms'},
            {'href': '#/typography', 'label': 'Typography'},
            {'href': '#/dropdowns', 'label': 'Dropdowns'},
            {'href': '#/icons', 'label': 'Icons'},
            {'href': '#/navs', 'label': 'Navs'},
            {'href': '#/modals', 'label': 'Modals'},
            {'href': '#/progress', 'label': 'Progress'},
            {'href': '#/presence', 'label': 'Presence'},
            {'href': '#/tooltips', 'label': 'Tooltips'},
            {'href': '#/avatars', 'label': 'Avatars'}
        ];
        $scope.navClass = function(page) {
            return (page.substring(2) === $location.path().substring(1) ? 'active' : '');
        }
    });