/**
 * @file store/pdp/termsandconditions.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-termsandconditions';

    function originStorePdpTermsandconditions(ComponentsConfigFactory, PdpUrlFactory, OcdPathFactory, DirectiveScope) {

        function originStorePdpTermsandconditionsLink(scope) {
            var edition = PdpUrlFactory.getCurrentEditionSelector();
            scope.model = {};
            if (edition.ocdPath) {
                OcdPathFactory.get(edition.ocdPath).attachToScope(scope, 'model');
            }

            //call to get legalNotes from ocd
            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE);
        }

        return {
            restrict: 'E',
            scope: {
                legalNotes: '@'
            },
            link: originStorePdpTermsandconditionsLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/termsandconditions.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpTermsandconditions
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {LocalizedText} legal-notes (optional) persistent legal notes
     *
     * @description PDP Terms and Conditions block, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-termsandconditions></origin-store-pdp-termsandconditions>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpTermsandconditions', originStorePdpTermsandconditions);
}());