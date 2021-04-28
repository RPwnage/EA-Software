/**
 * @file filter/filterpanel.js
 */
(function() {
    'use strict';

    function originFilterPanel(ComponentsConfigFactory, AnimateFactory, $timeout) {

        function originFilterPanelController ($scope, LayoutStatesFactory) {

            $scope.closeFilterPanel = function () {
                LayoutStatesFactory.setFilterOverlay(false);
            };
        }

        /**
         *
         * @param scope
         * @param element
         */
        function originFilterPanelLink(scope, element){
            var footer = document.querySelector('.origin-globalfooter');
            var $filterPanelElement = element.find('.origin-filterpanel');
            var filterPanelElement = $filterPanelElement.get(0);
            var filterPanelList = $filterPanelElement.find('.origin-filter-list').get(0);
            var filterPanelElementInitialOffset;

            /**
             * Checks if there is enough vertical space to display filter menu.
             * If so, set height to auto
             * If not, set wrapper height to visible height and make menu scrollable
             */
            function checkVerticalSpace(){
                var filterMenuEnd = filterPanelElement.offsetTop + filterPanelList.offsetHeight;
                if (filterMenuEnd > window.innerHeight){
                    $filterPanelElement.height(window.innerHeight - filterPanelElement.getBoundingClientRect().top);
                }else{
                    $filterPanelElement.height('auto');
                }
            }

            /**
             * Calculates the proper top position
             */
            function updateTopPosition(){
                var offset = filterPanelElementInitialOffset + window.scrollY;
                var transition = Math.max(offset, 0) + 'px';
                $filterPanelElement.css('top', transition);
            }

            /**
             * Checks if filter panel is completely visible
             * @returns {boolean}
             */
            function isFilterPanelCompletelyVisible(){
                return (filterPanelElement.offsetTop - window.scrollY - filterPanelElementInitialOffset) >= 0;
            }

            /**
             * Checks to see if filter has been scroll to footer.
             * If so, stop anchoring the filter
             * If not and there is enough room to display the filter, anchor it
             */
            function updateFilterPosition(){
                var filterMenuEnd = filterPanelElement.offsetTop + filterPanelElement.offsetHeight - window.scrollY;
                var footerTop = footer ? footer.offsetTop - window.scrollY : 0;
                //If filter menu is reaching the footer
                if (footer && filterMenuEnd >= footerTop){
                    if (isFilterPanelCompletelyVisible()){ //When there is enough room to display the filter menu
                        updateTopPosition();
                    }
                }else{
                    updateTopPosition();
                }
            }
            AnimateFactory.addScrollEventHandler(scope, updateFilterPosition, 100);
            AnimateFactory.addResizeEventHandler(scope, checkVerticalSpace, 300);
            filterPanelElementInitialOffset = filterPanelElement.offsetTop;
            $timeout(checkVerticalSpace, 0, false);
            updateFilterPosition();
        }

        return {
            restrict: 'E',
            transclude: true,
            link: originFilterPanelLink,
            controller: originFilterPanelController,
            scope: {
                isVisible: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filterpanel.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterPanel
     * @restrict E
     * @element ANY
     * @scope
     * @param {boolean} is-visible flag indicating whether the panel is open or closed
     * @description
     *
     * Slide-out panel for filters. This directive is a partial not directly merchandised.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filterpanel is-visible="isFilterPanelVisible"></origin-filterpanel>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originFilterPanel', originFilterPanel);
}());
