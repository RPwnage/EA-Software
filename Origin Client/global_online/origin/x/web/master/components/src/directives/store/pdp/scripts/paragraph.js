/**
 * @file store/pdp/scripts/description.js
 */
(function(){
    'use strict';

    function originStorePdpParagraphCtrl($scope, $sce) {
        $scope.description = $sce.trustAsHtml($scope.text);
    }

    function originStorePdpParagraph(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                text: '@'
            },
            controller: 'originStorePdpParagraphCtrl',
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
        .controller('originStorePdpParagraphCtrl', originStorePdpParagraphCtrl)
        .directive('originStorePdpParagraph', originStorePdpParagraph);
}());
