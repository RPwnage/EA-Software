
/**
 * @file /scripts/featureintro/giftingcallout.js
 */
(function(){
    'use strict';

    var GIFTING_FEATUREINTRO_DIALOG_ID = "origin-dialog-featureintro-gifting",
        LOCALSTORAGE_KEY = "origin-featureintro-gifting-seen";

    function OriginFeatureintroGiftingcalloutCtrl($scope, DialogFactory, FeatureConfig, LocalStorageFactory) {
        var isDismissed = LocalStorageFactory.get(LOCALSTORAGE_KEY, false),
            isGiftingEnabled = FeatureConfig.isGiftingEnabled(),
            isLoggedIn;

        /**
         * Once modal is closed, set LocalStorage key and hide callout button
         */
        function finishFeatureIntro() {
            isDismissed = true;
            updateVisibleState();
            LocalStorageFactory.set(LOCALSTORAGE_KEY, true);
        }

        /**
         * Open modal upon clicking on the pulsing circle button
         */
        $scope.startAction = function() {
            DialogFactory.open({
                id: GIFTING_FEATUREINTRO_DIALOG_ID,
                size: 'large',
                directives: [{
                    name: 'origin-dialog-content-featureintro-gifting',
                    data: {
                        'dialog-id': GIFTING_FEATUREINTRO_DIALOG_ID,
                        'title-str': $scope.titleStr,
                        'content': $scope.content,
                        'image-url': $scope.imageUrl,
                        'btn-text': $scope.btnText
                    }
                }],
                callback: finishFeatureIntro
            });
        };

        /**
         * Set visibility of callout button based on whether logged in, gifting feature enabled and modal has been dismissed
         */
        function updateVisibleState() {
            $scope.isVisible = (isLoggedIn && isGiftingEnabled && !isDismissed);
        }

        /**
         * Update logged-in state and refresh visible state
         * @param loginObj Response object from 'myauth:change' event fire containing logged-in state
         */
        this.updateLoggedInState = function(loginObj) {
            isLoggedIn = loginObj.isLoggedIn;
            updateVisibleState();
        };

        /**
         * If the feature intro dialog was not previously seen, and was just closed after being opened via queryString param,
         * we will store that the dialog was seen so the callout no longer shows up.
         * @param dialogData
         */
        this.dialogClosed = function(dialogData) {
            var dialogId = _.get(dialogData, 'id');
            if (dialogId && dialogId === GIFTING_FEATUREINTRO_DIALOG_ID) {
                finishFeatureIntro();
            }
        };

    }

    function originFeatureintroGiftingcallout(AppCommFactory, AuthFactory, ComponentsConfigFactory) {

        function originFeatureintroGiftingcalloutLink($scope, $elem, $attrs, OriginFeatureintroGiftingcalloutCtrl) {
            function onDestroy() {
                AuthFactory.events.off('myauth:change', OriginFeatureintroGiftingcalloutCtrl.updateLoggedInState);
                AppCommFactory.events.off('dialog:hide', OriginFeatureintroGiftingcalloutCtrl.dialogClosed);
            }
            $scope.$on('$destroy', onDestroy);

            AuthFactory.events.on('myauth:change', OriginFeatureintroGiftingcalloutCtrl.updateLoggedInState);
            AuthFactory.waitForAuthReady().then(OriginFeatureintroGiftingcalloutCtrl.updateLoggedInState);

            // Listen for the queryParam-triggered modal being closed
            AppCommFactory.events.on('dialog:hide', OriginFeatureintroGiftingcalloutCtrl.dialogClosed);
        }

        return {
            restrict: 'E',
            controller: OriginFeatureintroGiftingcalloutCtrl,
            link: originFeatureintroGiftingcalloutLink,
            scope: {
                titleStr: '@',
                content: '@',
                imageUrl: '@',
                btnText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('/featureintro/views/giftingcallout.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originFeatureintroGiftingcallout
     * @restrict E
     * @element ANY
     *
     * @param {LocalizedString} title-str Title of the modal that shows (override defaults)
     * @param {LocalizedString} content Content text in the modal (override defaults)
     * @param {ImageUrl} image-url URL for image that shows in the modal content (override defaults)
     * @param {LocalizedString} btn-text Text for OK button (override defaults)
     *
     * @description
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-featureintro-giftingcallout title-str="Introducing Gifting!" content="Lorem ipsum dolor sit amet." btn-text="Sounds great!" image-url="image.gif"></origin-featureintro-giftingcallout>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originFeatureintroGiftingcallout', originFeatureintroGiftingcallout);
}());
