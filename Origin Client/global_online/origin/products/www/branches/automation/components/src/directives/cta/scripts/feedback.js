/**
 * @file cta/feedback.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-cta-feedback',
        CLIENT_PLATFORM_QUERY_PARAM = 'Platform=c',
        WEB_PLATFORM_QUERY_PARAM = 'Platform=w';

    function OriginCtaFeedbackCtrl($scope, ComponentsLogFactory, UtilFactory, NavigationFactory) {

        var platform = (Origin.client.isEmbeddedBrowser())? CLIENT_PLATFORM_QUERY_PARAM : WEB_PLATFORM_QUERY_PARAM;

        $scope.isUnderAge = Origin.user.underAge();
        $scope.btnText = UtilFactory.getLocalizedStr($scope.btnText, CONTEXT_NAMESPACE, 'description');

        $scope.onBtnClick = function() {
            if($scope.href) {
                if($scope.href.indexOf('?') >= 0) {
                    NavigationFactory.externalUrl($scope.href+'&'+platform, true);
                }
                else {
                    NavigationFactory.externalUrl($scope.href+'?'+platform, true);
                }
            }
        };
    }

    function originCtaFeedback(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                href: '@',
                btnText: '@description'
            },
            controller: 'OriginCtaFeedbackCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/feedback.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaFeedback
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description "Send Feedback"
     * @param {Url} href the external url for survey you want to load
     *
     * @description
     *
     * Load feedback URL in an external browser
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-feedback href="http://decipher.somelink.com" description="Send Feedback" type="transparent"></origin-cta-feedback>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaFeedbackCtrl', OriginCtaFeedbackCtrl)
        .directive('originCtaFeedback', originCtaFeedback);
}());
