/**
 * @file shell/menu/scripts/menuitem.js
 */
(function() {
    'use strict';

    /**
     * Disposition map
     * @enum {string}
     */
    var DispositionEnumeration = {
        "hide": "hide",
        "logout": "logout",
        "externalurlSSO": "externalurlSSO"
    };

    /**
     * For context map
     * @enum {string}
     */
    var ForEnumeration = {
        "client": "client",
        "web": "web"
    };

    function OriginShellMenuItemCtrl($scope, $rootScope, $location, AppCommFactory, $state, AuthFactory, ComponentsUrlsFactory, NavigationFactory) {

        /**
         * Determine if the current menu item is selected
         * @return {boolean}
         * @method determineIfSelected
         */
        $scope.determineIfSelected =  function() {
            $scope.isCurrentItem = $state.current.data.section === $scope.section;
            return $scope.isCurrentItem;
        };

        /**
         * Determine if we should hide this menu item
         * @return {Boolean}
         * @method shouldHide
         */
        function shouldHide() {
            if ($scope.disposition && $scope.disposition === DispositionEnumeration.hide) {
                var hide;
                switch ($scope['for']) {
                    case ForEnumeration.client:
                        hide = Origin.client.isEmbeddedBrowser();
                        break;
                }
                return hide;
            }
            return false;
        }

        /**
         * Let the menu know what you were clicked on
         * @return {void}
         * @method
         */
        $scope.notifyClicked = function() {
            AppCommFactory.events.fire('menuitem:clicked');
        };

        $scope.goToLocation = function() {
            AppCommFactory.events.fire('uiRouter:go', $scope.href);
        };

        $scope.logout = function() {
            $scope.notifyClicked();
            AuthFactory.events.fire('auth:logout');
        };

        $scope.openUrlSSO = function() {
            var url = ComponentsUrlsFactory.getUrl($scope.href);
            if (!Origin.client.isEmbeddedBrowser()){
                url = url.replace("{locale}", Origin.locale.locale());
                url = url.replace("{access_token}", "");
            }
            NavigationFactory.asyncOpenUrlWithEADPSSO(url);
        };
                
        $scope.setClickEvent = function() {
            $scope.onClick = function() {
                $scope.notifyClicked();
                $scope.goToLocation();
            };
            if ($scope.disposition === DispositionEnumeration.logout) {
                $scope.onClick = $scope.logout;
            }
            if ($scope.disposition === DispositionEnumeration.externalurlSSO) {
                $scope.onClick = $scope.openUrlSSO;
            }
        };

        /**
         * Initializer, determine if selected and setup listeners to events
         * @return {void}
         * @method init
         */
        $scope.init = function() {
            // determine if we should hide this element
            $scope.hide = shouldHide();

            $scope.setClickEvent();

            if (!$scope.hide) {
                $scope.determineIfSelected();

                // TODO This should use Routing or maybe a location factory
                $rootScope.$on('$stateChangeSuccess', $scope.determineIfSelected);

            }
        };

        setTimeout($scope.init, 300);
    }

    function originShellMenuItemLink(scope, elem) {
        function getSubItemCount() {
            var subMenuItems = elem.find('.origin-shell-popout').children();
            return subMenuItems.length;
        }
        scope.hasSubmenu = (getSubItemCount() > 0);
    }

    function createMenuDirective(tempaltePath){
        return function originShellSubmenuItem(ComponentsConfigFactory) {
            return {
                restrict: 'E',
                templateUrl: ComponentsConfigFactory.getTemplatePath(tempaltePath),
                replace: true,
                controller: 'OriginShellMenuItemCtrl',
                transclude: true,
                scope: {
                    'titlestr': '@title',
                    'href': '@',
                    'disposition': '@',
                    'for': '@',
                    'section': '@'
                },
                link: originShellMenuItemLink
            };
        };
    }

    angular.module('origin-components')
        /**
         * @ngdoc directive
         * @name origin-components.directives:originShellMenuItem
         * @restrict E
         * @element ANY
         * @scope
         * @description
         *
         * menu item
         * @param {LocalizedString} title text show on link
         * @param {DispositionEnumeration} disposition a link behavior
         * @param {ForEnumeration} for the behavior's context
         * @param {string} section The section of the app this button links to. Used for highlighting.
         * @param {Url} href the link it will try to open
         * @example
         * <example module="origin-components">
         *     <file name="index.html">
         *         <origin-shell-menu-item>
         *             <origin-shell-menu-item-link titlestr="Home" href="#"></origin-shell-menu-item-link>
         *         </origin-shell-menu-item>
         *     </file>
         * </example>
         *
         */
        .directive('originShellMenuItem', ['ComponentsConfigFactory', createMenuDirective('shell/menu/views/item.html')])
        .directive('originShellSubmenuItem', ['ComponentsConfigFactory', createMenuDirective('shell/submenu/views/item.html')])
        .directive('originShellPopoutmenuItem', ['ComponentsConfigFactory', createMenuDirective('shell/popoutmenu/views/item.html')])
        .controller('OriginShellMenuItemCtrl', OriginShellMenuItemCtrl);
}());