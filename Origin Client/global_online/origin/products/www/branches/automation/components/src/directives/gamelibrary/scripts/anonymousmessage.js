/**
 * @file gamelibrary/anonymousmessage.js
 */
(function() {

    'use strict';

    function OriginGameLibraryAnonymousMessageCtrl($scope, $sce, UtilFactory) {
        var CONTEXT_NAMESPACE = 'origin-gamelibrary-anonymous-message';

        $scope.titleStr = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str'));
        $scope.descriptionStr = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description'));
    }


    function originGamelibraryAnonymousMessage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                description: '@description',
                image: '@image',
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/anonymousmessage.html'),
            controller: OriginGameLibraryAnonymousMessageCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryAnonymousMessage
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedText} title-str the title header text
     * @param {LocalizedText} description the description text
     * @param {ImageUrl} image the URL to the background image
     *
     * @description
     *
     * Display a message that prompts a user to login on the game library o create an account
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-anonymous>
     *             <origin-gamelibrary-anonymous-message
     *                 title-str="You must be signed in to see your game library."
     *                 description="&lt;a href='javascript:void()'&gt;Learn More&lt;a&gt;&nbsp;about what Origin has to offer."
     *                 image="https://www.example.com/images/foo.jpg"
     *                 >
     *                     <origin-cta-login></origin-cta-login>
     *                     <origin-cta-register></origin-cta-register>
     *                 </origin-gamelibrary-anonymous-message>
     *         </origin-gamelibrary-anonymous>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
          .controller('OriginGameLibraryAnonymousMessageCtrl', OriginGameLibraryAnonymousMessageCtrl)
          .directive('originGamelibraryAnonymousMessage', originGamelibraryAnonymousMessage);
})();
