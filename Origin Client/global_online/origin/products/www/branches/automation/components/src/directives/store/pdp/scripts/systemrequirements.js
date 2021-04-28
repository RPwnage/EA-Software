/**
 * @file store/pdp/systemrequirements.js
 */
(function(){
    'use strict';

    function originStorePdpSystemrequirements(ComponentsConfigFactory, PdpUrlFactory, OcdPathFactory) {

        function originStorePdpSystemrequirementsLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            var edition = PdpUrlFactory.getCurrentEditionSelector();

            if (edition.ocdPath) {
                OcdPathFactory.get(edition.ocdPath).attachToScope(scope, function(model) {
                    if (model.systemRequirements) {
                        scope.systemRequirements = model.systemRequirements;
                        originStorePdpSectionWrapperCtrl.setVisibility(true);
                    }
                });
            }
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            link: originStorePdpSystemrequirementsLink,
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/systemrequirements.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSystemrequirements
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * PDP system requirements block, retrieved from catalog
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-systemrequirements></origin-store-pdp-systemrequirements>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpSystemrequirements', originStorePdpSystemrequirements);
}());
