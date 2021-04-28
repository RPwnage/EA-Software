
/** 
 * @file store/access/trials/scripts/mediaprop.js
 */ 
(function(){
    'use strict';

    /* jshint ignore:start */
    var ShadeEnumeration = {
        "light": "light",
        "dark": "dark",
        "twoTone": "twoTone"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-access-trials-mediaprop';

    function OriginStoreAccessTrialsMediapropCtrl($scope, UtilFactory) {
        $scope.strings = {
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            description: UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description'),
            propOneHeader: UtilFactory.getLocalizedStr($scope.propOneHeader, CONTEXT_NAMESPACE, 'prop-one-header'),
            propOneDescription: UtilFactory.getLocalizedStr($scope.propOneDescription, CONTEXT_NAMESPACE, 'prop-one-description'),
            propTwoHeader: UtilFactory.getLocalizedStr($scope.propTwoHeader, CONTEXT_NAMESPACE, 'prop-two-header'),
            propTwoDescription: UtilFactory.getLocalizedStr($scope.propTwoDescription, CONTEXT_NAMESPACE, 'prop-two-description'),
            propThreeHeader: UtilFactory.getLocalizedStr($scope.propThreeHeader, CONTEXT_NAMESPACE, 'prop-three-header'),
            propThreeDescription: UtilFactory.getLocalizedStr($scope.propThreeDescription, CONTEXT_NAMESPACE, 'prop-three-description'),
            propFourHeader: UtilFactory.getLocalizedStr($scope.propFourHeader, CONTEXT_NAMESPACE, 'prop-four-header'),
            propFourDescription: UtilFactory.getLocalizedStr($scope.propFourDescription, CONTEXT_NAMESPACE, 'prop-four-description')
        };
    }

    function originStoreAccessTrialsMediaprop(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                background: '@',
                header: '@',
                ocdPath: '@',
                description: '@',
                shade: '@',
                propOneHeader: '@',
                propOneDescription: '@',
                propTwoHeader: '@',
                propTwoDescription: '@',
                propThreeHeader: '@',
                propThreeDescription: '@',
                propFourHeader: '@',
                propFourDescription: '@'
            },
            controller: OriginStoreAccessTrialsMediapropCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/trials/views/mediaprop.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessTrialsMediaprop
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} background the background image
     * @param {LocalizedString} header the header content
     * @param {OcdPath} ocd-path OCD Path
     * @param {ShadeEnumeration} shade The shade of the text
     * @param {LocalizedString} description paragraph in the top right of the module
     * @param {LocalizedString} prop-one-header the first prop header
     * @param {LocalizedString} prop-one-description the first prop content
     * @param {LocalizedString} prop-two-header the second prop header
     * @param {LocalizedString} prop-two-description the second prop content
     * @param {LocalizedString} prop-three-header the third prop header
     * @param {LocalizedString} prop-three-description the third prop content
     * @param {LocalizedString} prop-four-header the fourth prop header
     * @param {LocalizedString} prop-four-description the fourth prop content
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-trials-mediaprop />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessTrialsMediaprop', originStoreAccessTrialsMediaprop);
}()); 
