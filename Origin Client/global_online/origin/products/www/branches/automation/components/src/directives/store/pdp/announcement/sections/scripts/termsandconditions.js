/**
 * @file store/pdp/announcement/sections/scripts/termsandconditions.js
 */
(function(){
    'use strict';
    
    
    var CONTEXT_NAMESPACE = 'origin-store-announcement-termsandconditions';

    function originStoreAnnouncementTermsandconditions(ComponentsConfigFactory, DirectiveScope) {

        function originStoreAnnouncementTermsandconditionsLink(scope) {
            scope.model = {};

            function scopeResolved() {
                if (scope.termsAndConditions) {
                    //bind the result to model object to reuse existing view
                    scope.model.onlineDisclaimer = scope.termsAndConditions;
                }
            }

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE)
                .then(scopeResolved);
        }

        return {
            restrict: 'E',
            scope: {
                legalNotes: '@',
                termsAndConditions: '@' //function since we are getting html
            },
            link: originStoreAnnouncementTermsandconditionsLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/termsandconditions.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAnnouncementTermsandconditions
     * @restrict E
     * @element ANY
     * @scope
     * @description Announcement Terms and Conditions block, retrieved from catalog
     * @param {LocalizedText} terms-and-conditions terms and conditions html
     *
     * @example
     * <origin-store-row>
     *     <origin-store-announcement-termsandconditions terms-and-conditions="Terms and conditions text"></origin-store-announcement-termsandconditions>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStoreAnnouncementTermsandconditions', originStoreAnnouncementTermsandconditions);
}());