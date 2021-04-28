/**
 * @file socialmedia/scripts/facebookshare.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-socialmedia-facebookshare';

    function OriginSocialmediaFacebookshareCtrl($scope, SocialSharingFactory, DirectiveScope, UtilFactory) {
        $scope.socialNetworkType = 'facebook';
        $scope.openShareWindow = function () {
            var shareConfig = {};
            DirectiveScope.populateScope($scope, CONTEXT_NAMESPACE)
                .then(function() {
                    shareConfig.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
                    shareConfig.description = UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description');
                    shareConfig.picture = $scope.image;
                    SocialSharingFactory.openFacebookShareWindow(shareConfig);
                });
        };
    }

    function originSocialmediaFacebookshare(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                description: '@',
                image: '@'
            },
            controller: 'OriginSocialmediaFacebookshareCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/socialmedia/views/share.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialmediaFacebookshare
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title-str Title of facebook share
     * @param {LocalizedString} description Desciption of facebook share
     * @param {ImageUrl} image Image to share on facebook
     *
     * Social Media directive to hold buttons
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-socialmedia-facebookshare></origin-socialmedia-facebookshare>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginSocialmediaFacebookshareCtrl', OriginSocialmediaFacebookshareCtrl)
        .directive('originSocialmediaFacebookshare', originSocialmediaFacebookshare);
}());