/**
 * @file store/pdp/scripts/description.js
 */
(function(){
    'use strict';


    function originStorePdpParagraph($sce, ComponentsConfigFactory) {

        function originStorePdpParagraphLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            var stopWatchingText = scope.$watch('text', function(newValue) {
                if (newValue) {
                    stopWatchingText();
                    scope.description = $sce.trustAsHtml(newValue);
                    originStorePdpSectionWrapperCtrl.setVisibility(true);
                }
            });
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {
                text: '@'
            },
            link: originStorePdpParagraphLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/paragraph.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpParagraph
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} text the paragraph body
     * @description
     *
     * PDP custom text block
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-paragraph text"Something <strong>really</strong> important"></origin-store-pdp-paragraph>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpParagraph', originStorePdpParagraph);
}());
