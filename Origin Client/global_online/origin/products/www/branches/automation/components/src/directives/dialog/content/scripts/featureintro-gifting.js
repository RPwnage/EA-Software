
/**
 * @file dialog/content/scripts/featureintro-gifting.js
 */
(function(){
    'use strict';

    var DIALOG_CONTEXT_NAMESPACE = 'origin-dialog-featureintro-gifting';

    function OriginDialogContentFeatureintroGiftingCtrl($scope, DialogFactory, UtilFactory) {

        $scope.strings = {
            titleStr: UtilFactory.getLocalizedStr($scope.titleStr, DIALOG_CONTEXT_NAMESPACE, 'title-str'),
            content: UtilFactory.getLocalizedStr($scope.content, DIALOG_CONTEXT_NAMESPACE, 'content'),
            imageUrl: UtilFactory.getLocalizedStr($scope.imageUrl, DIALOG_CONTEXT_NAMESPACE, 'image-url'),
            btnText: UtilFactory.getLocalizedStr($scope.btnText, DIALOG_CONTEXT_NAMESPACE, 'btn-text')
        };

        /**
         * Close the dialog
         */
        $scope.onBtnClick = function() {
            DialogFactory.close($scope.dialogId);
        };
    }

    function originDialogContentFeatureintroGifting(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                dialogId: '@',
                titleStr: '@',
                content: '@',
                imageUrl: '@',
                btnText: '@'
            },
            controller: OriginDialogContentFeatureintroGiftingCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/featureintro-gifting.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogFeatureintroGifting
     * @restrict E
     * @element ANY
     * @scope
     * @description Gifting/Wishlist feature intro dialog content
     *
     * @param {string} dialog-id Dialog id
     * @param {LocalizedString} title-str Title of the modal that shows
     * @param {LocalizedString} content Content text in the modal
     * @param {ImageUrl} image-url URL for image that shows in the modal content
     * @param {LocalizedString} btn-text Text for OK button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
         *     <origin-dialog-content-featureintro-gifting dialog-id="origin-featureintro-gifting-dialog" title-str="Introducing Gifting!" content="Lorem ipsum dolor sit amet." btn-text="Got it!" image-url="image.gif"></origin-dialog-content-featureintro-gifting>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentFeatureintroGiftingCtrl', OriginDialogContentFeatureintroGiftingCtrl)
        .directive('originDialogContentFeatureintroGifting', originDialogContentFeatureintroGifting);
}());
