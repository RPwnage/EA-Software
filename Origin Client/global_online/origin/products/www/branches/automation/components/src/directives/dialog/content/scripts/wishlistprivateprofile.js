/**
 * @file dialog/content/wishlistprivateprofile.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-wishlistprivateprofile';

    function originDialogContentWishlistprivateprofile(ComponentsConfigFactory, ComponentsConfigHelper, NavigationFactory, UtilFactory, DialogFactory, $compile) {
        function originDialogContentWishlistprivateprofileLink(scope, elem) {
            scope.headingLoc = UtilFactory.getLocalizedStr(scope.heading, CONTEXT_NAMESPACE, 'heading');
            scope.cancelLoc = UtilFactory.getLocalizedStr(scope.cancelcta, CONTEXT_NAMESPACE, 'cancelcta');

            function appendDescription() {
                var descriptionLoc = UtilFactory.getLocalizedStr(scope.description, CONTEXT_NAMESPACE, 'description'),
                    descriptionContainer = elem.find('.wishlistprivateprofiletext'),
                    descriptionContent = $compile(['<span>', descriptionLoc, '</span>'].join(''))(scope);

                descriptionContainer.append(descriptionContent);
            }

            scope.closeDialog = function() {
                DialogFactory.close(CONTEXT_NAMESPACE);
            };

            scope.goToAccount = function($event) {
                $event.stopPropagation();
                $event.preventDefault();

                var accountsUrl = ComponentsConfigHelper.getUrl('accountPrivacySettings')
                    .replace("{locale}", Origin.locale.locale());

                NavigationFactory.externalUrl(accountsUrl, true);

                window.requestAnimationFrame(scope.closeDialog);
            };

            appendDescription();
        }

        return {
            restrict: 'E',
            scope: {
                heading: '@',
                description: '@',
                closecta: '@',
            },
            link: originDialogContentWishlistprivateprofileLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/wishlistprivateprofile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentWishlistprivateprofile
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} heading the dialog heading for the share modal
     * @param {LocalizedString} description the heading for the share on facebook tile
     * @param {LocalizedString} cancelcta the description text for she share on facebook tile
     * @description
     *
     * Link the user to their personal profile settings in order to share their wishlist with othe players
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-wishlistprivateprofile
     *            heading="Share on Facebook"
     *            description="You'll be able to customize your post in the next step"
     *            cancelcta="Cancel"></origin-dialog-content-sharewishlist>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentWishlistprivateprofile', originDialogContentWishlistprivateprofile);

}());