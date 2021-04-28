/**
 * @file filter/filteritem.js
 */
(function() {
    'use strict';

    /**
     * A list of visibility options
     * @readonly
     * @enum {string}
     */
    var VisibilityEnumeration = {
        "always": "always",
        "embedded": "embedded",
        "online": "online"
    };

    function originGamelibraryFilterItem(ComponentsConfigFactory, GamesFilterFactory, AuthFactory) {
        function originGamelibraryFilterLink(scope, element, attributes) {
            scope.filterId = attributes.filterId;
            scope.model = GamesFilterFactory.getFilter(scope.filterId);

            /**
             * Returns filter item visibility based on the current state:
             * some filter items are only available to online users.
             * @return {boolean}
             */
            scope.isVisible = function () {
                var isEmbeddedBrowser = Origin.client.isEmbeddedBrowser(),
                    visible = false;

                switch(attributes.isVisible) {
                    case VisibilityEnumeration.always:
                        visible = true;
                        break;
                    case VisibilityEnumeration.embedded:
                        if (isEmbeddedBrowser) {
                            visible = true;
                        }
                        break;
                    case VisibilityEnumeration.online:
                        visible = AuthFactory.isClientOnline();
                        break;
                }

                return visible;
            };

            /**
             * Filter items with no game matching are disabled.
             * @return {boolean}
             */
            scope.isEnabled = function () {
                return scope.model.count > 0;
            };

            AuthFactory.events.on('myauth:clientonlinestatechanged', scope.$digest);

            scope.$on('$destroy', function() {
                //disconnect listening for events
                AuthFactory.events.off('myauth:clientonlinestatechanged', scope.$digest);
            });
        }

        return {
            restrict: 'E',
            controller: 'OriginGamelibraryFilterCtrl',
            scope: true,
            link: originGamelibraryFilterLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/filteritem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryFilterItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} filter-id filter identifier as provided by the GamesFilterFactory
     * @param {VisibilityEnumeration} is-visible flag indicating whether the filter is visible or not for the current user state
     * @description
     *
     * Game library filter panel item. This directive is a partial not directly merchandised.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-filter-item filter-id="isSomething" is-visible="always"></origin-gamelibrary-filter-item>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryFilterItem', originGamelibraryFilterItem);
}());
