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
        "externalurlSSO": "externalurlSSO",
        "selectlanguage": "selectlanguage",
        "help": "help"
    };

    /**
     * For context map
     * @enum {string}
     */
    var ForEnumeration = {
        "client": "client",
        "web": "web",
        "underAge": "underAge",
        "underAgeOrClient": "underAgeOrClient"
    };

    /* jshint ignore:start */
    var MenuCategoryEnumeration = {
        "store" : "store",
        "browse" : "browse",
        "freegames" : "freegames",
        "categorypage" : "categorypage",
        "dealcenter" : "dealcenter",
        "originaccess" : "originaccess",
        "gamelibrary" : "gamelibrary",
        "settings" : "settings",
        "search" : "search",
        "profile" : "profile",
        "home": "home",
        "homeloggedin" : "homeloggedin",
        "error" : "error"
    };
    /* jshint ignore:end */

    function OriginShellMenuItemCtrl($scope, $state, AppCommFactory, AuthFactory, NavigationFactory, ComponentsConfigHelper, DialogFactory) {

        var stateChangeSuccessHandle = null;

        /**
         * Determine if this disposition should be hidden
         * @return {Boolean}
         * @method isHiddenDisposition
         */
        function isHiddenDisposition(disposition) {
            return (!!disposition &&
                    (disposition === DispositionEnumeration.hide ||
                    disposition === DispositionEnumeration.externalurlSSO ||
                    disposition === DispositionEnumeration.help));
        }

        /**
         * Determine if we should hide this menu item
         * @return {Boolean}
         * @method shouldHide
         */
        function shouldHide() {
            if (isHiddenDisposition($scope.disposition)) {
                var hide;
                switch ($scope['for']) {
                    case ForEnumeration.client:
                        hide = Origin.client.isEmbeddedBrowser();
                        break;
                    case ForEnumeration.web:
                        hide = !Origin.client.isEmbeddedBrowser();
                        break;
                    case ForEnumeration.underAge:
                        hide = Origin.user.underAge();
                        break;
                    case ForEnumeration.underAgeOrClient:
                        hide = Origin.client.isEmbeddedBrowser() || Origin.user.underAge();
                        break;
                }
                return hide;
            }

            // hide this if you are in the client as you can set it in the application settings
            if ($scope.disposition && $scope.disposition === DispositionEnumeration.selectlanguage) {
                return Origin.client.isEmbeddedBrowser();
            }
            return false;
        }

        /**
         * Let the menu know what you were clicked on
         * @return {void}
         * @method
         */
        $scope.notifyClicked = function($event) {
            if ($event){
                $event.preventDefault();
            }

            AppCommFactory.events.fire('menuitem:clicked');

            if ($scope.href){
                NavigationFactory.openLink($scope.href);
            }
        };

        $scope.logout = function($event) {
            if ($event) {
                $event.preventDefault();
            }
            $scope.notifyClicked();
            AuthFactory.events.fire('auth:logout');
        };

        /**
        * Open the language selection dialog
        * @method openLanguageDialog
        */
        $scope.openLanguageDialog = function($event) {
            if ($event) {
                $event.preventDefault();
            }
            $scope.notifyClicked();
            DialogFactory.open({
                id: 'origin-dialog-content-selectlanguage',
                xmlDirective: '<origin-dialog-content-selectlanguage class="origin-dialog-content"></origin-dialog-content-selectlanguage>'
            });
        };

        $scope.openUrlSSO = function($event) {
            if ($event) {
                $event.preventDefault();
            }
            var url = ComponentsConfigHelper.getUrl($scope.href);
            if (!Origin.client.isEmbeddedBrowser()){
                url = url.replace('{locale}', Origin.locale.locale());
                url = url.replace('{access_token}', '');
            }
            NavigationFactory.asyncOpenUrlWithEADPSSO(url);
        };

        $scope.showOriginHelp = function($event) {
            if ($event) {
                $event.preventDefault();
            }
            if (!Origin.client.isEmbeddedBrowser()) {
                NavigationFactory.externalUrl(ComponentsConfigHelper.getHelpUrl(Origin.locale.locale(), Origin.locale.languageCode()));

            } else {
                Origin.client.navigation.showOriginHelp();
            }

        };

        /**
         * Determine if the current menu item is selected
         * @return {boolean}
         * @method determineIfSelected
         */
        $scope.determineIfSelected = function () {
            $scope.isActive = ($state.current.data.section === $scope.section);
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
                stateChangeSuccessHandle = AppCommFactory.events.on('uiRouter:stateChangeSuccess',  $scope.determineIfSelected);

                $scope.determineIfSelected();
            }
        };

        /**
         * clean up listeners
         */
        function onDestroy() {
            if(stateChangeSuccessHandle) {
                stateChangeSuccessHandle.detach();
            }
        }

        $scope.$on('$destroy', onDestroy);
        //need to wait for auth state to be set so we know whether to disable/enable item
        AuthFactory.waitForAuthReady()
            .then($scope.init);
    }

    function getMenuDirective(NavigationFactory) {

        function originShellMenuItemLink(scope, element) {
            function getSubItemCount() {
                var subMenuItems = element.find('.origin-shell-popout').children();
                return subMenuItems.length;
            }
            scope.hasSubmenu = (getSubItemCount() > 0);

            //we need to manually position the submenus so that it will work when the nav is 
            //showing a vertical scrollbar
            if (scope.hasSubmenu) {
                angular.element('.origin-shell-submenu-item').on('mouseover', function() {
                    var menuItem = angular.element(this),
                        submenuWrapper = angular.element('> .origin-shell-popout-container', menuItem),
                        isOverflowing = angular.element('.origin-navigation-isoverflowing').length,
                        LEFT_ADJUST = 0.85,
                        LEFT_ADJUST_OVERFLOW = 0.84;

                    // grab the menu item's position relative to its positioned parent
                    var menuItemPos = menuItem.position();

                    // place the submenu in the correct position relevant to the menu item
                    // 
                    // HACK: check if we are overflowing, if we are we cannot use the normal adjust because
                    // it will lose the hover as we are moving across menus
                    submenuWrapper.css({
                        top: menuItemPos.top,
                        left: menuItemPos.left + Math.round(menuItem.outerWidth() * (isOverflowing ? LEFT_ADJUST_OVERFLOW : LEFT_ADJUST))
                    });
                });
            }

            scope.setClickEvent = function() {
                scope.onClick = scope.notifyClicked;
                if (scope.disposition === DispositionEnumeration.logout) {
                    scope.href = '';
                    scope.hrefAbsoluteUrl = NavigationFactory.getAbsoluteUrl(scope.href);
                    scope.onClick = scope.logout;
                }else if (scope.disposition === DispositionEnumeration.externalurlSSO) {
                    scope.onClick = scope.openUrlSSO;
                }else if (scope.disposition === DispositionEnumeration.selectlanguage) {
                    scope.href = '';
                    scope.hrefAbsoluteUrl = NavigationFactory.getAbsoluteUrl(scope.href);
                    scope.onClick = scope.openLanguageDialog;
                }else if (scope.disposition === DispositionEnumeration.help) {
                    scope.onClick = scope.showOriginHelp;
                }
            };

            function onDestroy() {
                angular.element('.origin-shell-submenu-item').off('mouseover');
            }

            scope.$on('$destroy', onDestroy);

            scope.hrefAbsoluteUrl = NavigationFactory.getAbsoluteUrl(scope.href);
        }

        return {
            restrict: 'E',
            replace: true,
            controller: 'OriginShellMenuItemCtrl',
            transclude: true,
            scope: {
                'titleStr': '@',
                'href': '@',
                'disposition': '@',
                'section': '@',
                'for': '@'
            },
            link: originShellMenuItemLink
        };
    }

    function originShellMenuItem(ComponentsConfigFactory, NavigationFactory) {
        var template = {
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/menu/views/item.html')
        };

        return _.merge(getMenuDirective(NavigationFactory), template);
    }

    function originShellSubmenuItem(ComponentsConfigFactory, NavigationFactory) {
        var template = {
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/submenu/views/item.html')
        };

        return _.merge(getMenuDirective(NavigationFactory), template);
    }

    function originShellPopoutmenuItem(ComponentsConfigFactory, NavigationFactory) {
        var template = {
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/popoutmenu/views/item.html')
        };

        return _.merge(getMenuDirective(NavigationFactory), template);
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
         * @param {LocalizedString} title-str text show on link
         * @param {DispositionEnumeration} disposition a link behavior
         * @param {ForEnumeration} for the behavior's context
         * @param {Url} href the link it will try to open
         * @param {MenuCategoryEnumeration} section section the menu item belongs to. Must be set to enable menu hightlighting
         * @example
         * <example module="origin-components">
         *     <file name="index.html">
         *         <origin-shell-menu-item>
         *             <origin-shell-menu-item-link title-str="Home" href="#"></origin-shell-menu-item-link>
         *         </origin-shell-menu-item>
         *     </file>
         * </example>
         */
        .directive('originShellMenuItem', originShellMenuItem)
        .directive('originShellSubmenuItem', originShellSubmenuItem)
        .directive('originShellPopoutmenuItem', originShellPopoutmenuItem)
        .controller('OriginShellMenuItemCtrl', OriginShellMenuItemCtrl);
}());
