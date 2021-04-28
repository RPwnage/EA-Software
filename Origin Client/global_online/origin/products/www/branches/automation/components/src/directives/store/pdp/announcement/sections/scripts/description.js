/**
 * @file store/pdp/announcement/sections/scripts/description.js
 */
(function(){
    'use strict';

    function originStoreAnnouncementDescription($sce, ComponentsConfigFactory) {

        function originStoreAnnouncementDescriptionLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            scope.model = {};
            if (scope.longDescription) {
                scope.model.longDescription = $sce.trustAsHtml(scope.longDescription);

                if (originStorePdpSectionWrapperCtrl && _.isFunction(originStorePdpSectionWrapperCtrl.setVisibility)) {
                    originStorePdpSectionWrapperCtrl.setVisibility(true);
                }
            }
        }

        return {
            restrict: 'E',
            require: '?^originStorePdpSectionWrapper',
            scope: {
                longDescription: '@'
            },
            link : originStoreAnnouncementDescriptionLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/description.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAnnouncementDescription
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP description block, retrieved from catalog
     * @param {LocalizedText} long-description game description html
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-announcement-description></origin-store-announcement-description>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStoreAnnouncementDescription', originStoreAnnouncementDescription);
}());
