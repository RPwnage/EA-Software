/**
 * @file home/merchandise/area.js
 */
(function() {
    'use strict';

    /**
     * Layout type mapping
     * @enum {string}
     */
    var LayoutTypeEnumeration = {
            "one": "one",
            "two": "two",
            "carousel": "carousel"
    };

    function OriginHomeMerchandiseAreaCtrl($scope, DialogFactory, ComponentsLogFactory) {
        $scope.visible = true;
        $scope.isLoading = true;

        function handleFeedStoryError(error) {
            $scope.visible = false;
            $scope.isLoading = false;

            DialogFactory.openAlert({
                id: 'web-feed-server-error',
                title: 'Feed Server Error',
                description: 'The feed server is down. The \'Discover Something New\' section and \'Merchandise\' offer section will not show properly. If this continues for an extended period of time please email bryanchan@ea.com.',
                rejectText: 'OK'
            });
            ComponentsLogFactory.error('[OriginHomeMerchandiseAreaCtrl:retrieveMerchandiseStoryData]', error.stack);
        }

        this.renderCarouselPromotion = function() {
            //TODO: build the html to call the carousel merchandise tile
        };

        this.renderPromotions = function(numPromotions){
            Origin.feeds.retrieveStoryData('merchandise', 0, numPromotions, Origin.locale.locale()).then(function(response) {
                $scope.merchitems = response;
                $scope.isLoading = false;
            }, function(error) {
                handleFeedStoryError(error);
            }).catch(function(error) {
                handleFeedStoryError(error);
            });
        };
    }

    function originHomeMerchandiseArea(ComponentsConfigFactory) {

        function originHomeMerchandiseAreaLink(scope, element, args, ctrl) {
            switch (scope.layouttype) {
                case LayoutTypeEnumeration.one:
                    ctrl.renderPromotions(1);
                    break;
                case LayoutTypeEnumeration.two:
                    ctrl.renderPromotions(2);
                    break;
                case LayoutTypeEnumeration.carousel:
                    ctrl.renderCarouselPromotion();
                    break;
            }
        }

        return {
            restrict: 'E',
            scope: {
                config: '@',
                layouttype: '@'
            },
            controller: 'OriginHomeMerchandiseAreaCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/merchandise/views/area.html'),
            link: originHomeMerchandiseAreaLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeMerchandiseArea
     * @restrict E
     * @element ANY
     * @scope
     * @param {LayoutTypeEnumeration} layouttype the layouttype identifier
     * @description
     *
     * merchandise area
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-merchandise-area layouttype="two"></origin-home-merchandise-area>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeMerchandiseAreaCtrl', OriginHomeMerchandiseAreaCtrl)
        .directive('originHomeMerchandiseArea', originHomeMerchandiseArea);
}());