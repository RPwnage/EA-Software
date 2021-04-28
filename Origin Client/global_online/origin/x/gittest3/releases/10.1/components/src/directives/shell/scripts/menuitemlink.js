/**
 * @file shell/scripts/menuitemlink.js
 */
(function() {
    'use strict';

    /**
     * Disposition map
     * @enum {string}
     */
    var DispositionEnumeration = {
        "hide": "hide"
    };

    /**
     * For context map
     * @enum {string}
     */
    var ForEnumeration = {
        "client": "client",
        "web": "web"
    };

    /**
     * Icon context map
     * @enum {string}
     */
    var IconEnumeration = {
        "home": "home",
        "store": "store",
        "gamelibrary": "gamelibrary",
        "profile": "profile"
    };

    function OriginShellMenuItemLinkCtrl($scope, $rootScope, $location, AppCommFactory, $state) {

        /**
         * Determine if the current menu item is selected
         * @return {void}
         * @method determineIfSelected
         */
        function determineIfSelected() {
            $scope.isCurrentItem = $state.current.data.section === $scope.section;
        }

        /**
         * Set the icon class
         * @return {void}
         * @method setIcon
         */
        function setIcon() {
            if ($scope.icon && IconEnumeration[$scope.icon]) {
                $scope.iconClass = 'otkicon-' + $scope.icon;
            } else {
                $scope.icon = '';
            }
        }

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
                        hide = Origin.bridgeAvailable();
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
        $scope.onClick = function() {
            AppCommFactory.events.fire('menuitem:clicked');
        };

        /**
         * Constructor, set the icon class, determine if
         * selected and listen to events
         * @return {void}
         * @method init
         */
        this.init = function() {

            // determine if we should hide this element
            $scope.hide = shouldHide();

            if (!$scope.hide) {
                setIcon();
                determineIfSelected();

                // PJ: do i have to remove this?
                // update selection on location change
                $rootScope.$on('$locationChangeSuccess', determineIfSelected);
            }

        };


    }

    function originShellMenuItemLink(ComponentsConfigFactory) {

        function originShellMenuItemLinkLink(scope, elem, attrs, ctrl) {
            ctrl.init();
        }

        return {
            restrict: 'E',
            controller: 'OriginShellMenuItemLinkCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/menuitemlink.html'),
            replace: true,
            scope: {
                'titlestr': '@title',
                'href': '@href',
                'disposition': '@disposition',
                'for': '@for',
                'icon': '@icon',
                'section': '@'
            },
            link: originShellMenuItemLinkLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellMenuItemLink
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title text show on link
     * @param {DispositionEnumeration} disposition a link behavior
     * @param {ForEnumeration} for the behavior's context
     * @param {IconEnumeration} icon the optional icon to use
     * @param {string} section The section of the app this button links to. Used for highlighting.
     * @param {Url} href the link it will try to open
     * @description
     *
     * shell menu item link
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-menu-item-link section="home" titlestr="Home" href="#" icon="home"></origin-shell-menu-item-link>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginShellMenuItemLinkCtrl', OriginShellMenuItemLinkCtrl)
        .directive('originShellMenuItemLink', originShellMenuItemLink);
}());