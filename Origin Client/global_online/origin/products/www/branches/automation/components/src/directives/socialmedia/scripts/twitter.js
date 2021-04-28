/**
 * @file socialmedia/scripts/twitter.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-socialmedia-twitter';

    function OriginSocialmediaTwitterCtrl($scope, SocialSharingFactory, UtilFactory) {
        $scope.socialNetworkType = 'twitter';

        var shareText = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'message', {
            '%gamename%': $scope.displayName
        });

        $scope.openShareWindow = function () {
            SocialSharingFactory.openTwitterShareWindow(shareText);
        };

        $scope.$watchOnce('displayName', function(){
            shareText = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'message', {
                '%gamename%': $scope.displayName
            }); 
        });
    }


    function originSocialmediaTwitter(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                displayName: '@'
            },
            controller: 'OriginSocialmediaTwitterCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/socialmedia/views/share.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialmediaTwitter
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {LocalizedString} display-name game name
     * @param {LocalizedString} message checkout text. Set up once as default.
     *
     * @description
     *
     * Social Media directive to hold buttons
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-socialmedia-twitter display-name="displayName"></origin-socialmedia-twitter>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginSocialmediaTwitterCtrl', OriginSocialmediaTwitterCtrl)
        .directive('originSocialmediaTwitter', originSocialmediaTwitter);
}());
