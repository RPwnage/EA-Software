/**
 * @file store/pdp/scripts/description.js
 */
(function(){
    'use strict';

    function originStorePdpDescription(ComponentsConfigFactory, PdpUrlFactory, OcdPathFactory) {

        function originStorePdpDescriptionLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {

            OcdPathFactory.get(PdpUrlFactory.buildPathFromUrl()).attachToScope(scope, function(model) {
                if (model.longDescription) {
                    scope.longDescription = model.longDescription;
                    originStorePdpSectionWrapperCtrl.setVisibility(true);
                }
            });
        }

        return {
            require: '^originStorePdpSectionWrapper',
            restrict: 'E',
            scope: {},
            link: originStorePdpDescriptionLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/description.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDescription
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP description block, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-description></origin-store-pdp-description>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpDescription', originStorePdpDescription);
}());
