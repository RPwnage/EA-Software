/**
 * @file
 */
(function () {
    'use strict';


    /**
     * BooleanTypeEnumeration list of allowed types
     * @enum {string}
     */
    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStorePdpSectionWrapperCtrl($scope, ScopeHelper) {

        $scope.isVisible = false;
        this.setVisibility = function(visible) {
            if ($scope.isVisible !== visible) {
                $scope.isVisible = visible;
                $scope.notifyHeroParent(visible);
                ScopeHelper.digestIfDigestNotInProgress($scope);
            }
        };
        
        this.setHeaderClass = function(className){
            $scope.headerCSSClass = className;
        };
    }

    function originStorePdpSectionWrapper(ComponentsConfigFactory) {


        function originStorePdpSectionWrapperLink(scope, element, attributes, controllers) {

            var controller = _.first(_.compact(controllers)),
                hasParentCtrl = controller && _.isFunction(controller.setHasContent);

            scope.notifyHeroParent = function(hasContent) {
                if (hasParentCtrl) {
                    controller.setHasContent(hasContent);
                }
            };

        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@'
            },
            require: ['^?originStorePdpFeatureHeroWrapper', '^?originStorePdpFeatureVideoWrapper', '^?originStorePdpFeatureContainerWrapper', '^?originStorePdpFeaturePremierWrapper'],
            controller: originStorePdpSectionWrapperCtrl,
            link: originStorePdpSectionWrapperLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/section-wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     *
     * @example
     *
     */
    angular.module('origin-components')
        .directive('originStorePdpSectionWrapper', originStorePdpSectionWrapper);
}());
