/**
 * @file dialog/content/sharewishlist.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-sharewishlist',
        COPY_BUTTON_TIMEOUT_MS = 5000,
        COPY_BUTTON_OLD_BROWSER_TIMEOUT_MS = 15000;

    function originDialogContentSharewishlist(NavigationFactory, ComponentsConfigFactory, UtilFactory, DialogFactory, ScopeHelper, SocialSharingFactory) {
        function originDialogContentSharewishlistLink(scope) {
            scope.shareDialogHeadingLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sharedialogheading');
            scope.shareFacebookHeadingLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sharefacebookheading');
            scope.shareFacebookDescriptionLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sharefacebookdescription');
            scope.shareLinkHeadingLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sharelinkheading');

            scope.shareFacebookMessageDescriptionLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sharefacebookmessagedescription', {'%originId%' : Origin.user.originId()});
            scope.shareFacebookMessageTitleLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sharefacebookmessagetitle', {'%originId%' : Origin.user.originId()});
            scope.shareFacebookMessageImage = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sharefacebookmessagepicture');
            scope.copyLinkToClipboardCtaLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'copylinktoclipboardcta');
            scope.linkCopiedLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'linkcopied');
            scope.linkCopiedOldBrowserLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'linkcopiedoldbrowser');
            scope.cancelCtaLoc = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'cancelcta');
            scope.wishlistLink = NavigationFactory.getHrefFromState('app.wishlistredirector', {wishlistId: scope.wishlistid}, {absolute: true});

            function restoreCopyButton() {
                scope.isCopied = false;
                scope.isCopiedOldBrowser = false;
                ScopeHelper.digestIfDigestNotInProgress(scope);
            }

            scope.openShareWindow = function ($event) {
                $event.stopPropagation();
                $event.preventDefault();

                scope.$emit('cta:clicked', 'self_share_wishlist_facebook');

                var shareConfig = {
                    title: scope.shareFacebookMessageTitleLoc,
                    description: scope.shareFacebookMessageDescriptionLoc,
                    u: scope.wishlistLink
                };

                if (scope.shareFacebookMessageImage) {
                    shareConfig.picture = scope.shareFacebookMessageImage;
                }

                SocialSharingFactory.openFacebookShareWindow(shareConfig);
            };

            scope.copyLink = function(e) {
                scope.$emit('cta:clicked', 'self_copy_wishlist_link');
                scope.isCopied = true;
                e.clearSelection();
                ScopeHelper.digestIfDigestNotInProgress(scope);
                setTimeout(restoreCopyButton, COPY_BUTTON_TIMEOUT_MS);
            };

            scope.copyLinkOldBrowser = function() {
                scope.$emit('cta:clicked', 'self_copy_wishlist_link_old_browser');
                scope.isCopiedOldBrowser = true;
                setTimeout(restoreCopyButton, COPY_BUTTON_OLD_BROWSER_TIMEOUT_MS);
                ScopeHelper.digestIfDigestNotInProgress(scope);
            };

            scope.closeDialog = function() {
                DialogFactory.close(CONTEXT_NAMESPACE);
            };
        }

        return {
            restrict: 'E',
            scope: {
                wishlistid: '@'
            },
            link: originDialogContentSharewishlistLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/sharewishlist.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentSharewishlist
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} sharedialogheading the dialog heading for the share modal
     * @param {LocalizedString} sharefacebookheading the heading for the share on facebook tile
     * @param {LocalizedString} sharefacebookdescription the description text for she share on facebook tile
     * @param {LocalizedString} sharelinkheading the heading for the share link tile
     * @param {LocalizedString} copylinktoclipboardcta the copy to clipboard action
     * @param {LocalizedString} linkcopied message confirming that a user has copied a link to their clipboard
     * @param {LocalizedString} linkcopiedoldbrowser copy to clipboard action for outdated browsers that don't support clipboard access
     * @param {LocalizedString} cancelcta the cancel button for the modal
     * @param {LocalizedString} sharefacebookmessagedescription Description for facebook share message, should include %originId%
     * @param {LocalizedString} sharefacebookmessagetitle Title for facebook share message, should include %originId%
     * @param {ImageUrl} sharefacebookmessagepicture Image to include for facebook share message
     * @param {string} wishlistid the wishlist identifier
     * @description
     * Share personal wishlist dialog and facebook integration
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-sharewishlist
     *            sharedialogheading="How do you want to share your wishlist?"
     *            sharefacebookheading="Share on Facebook"
     *            sharefacebookdescription="You'll be able to customize your post in the next step"
     *            sharelinkheading="Or you can just share the link"
     *            copylinktoclipboardcta="Copy Link"
     *            linkcopied="Copied!"
     *            linkcopiedoldbrowser = "Press CTRL+C/Command+C to copy"
     *            cancelcta="Cancel"
     *            wishlistid="ABC123"></origin-dialog-content-sharewishlist>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentSharewishlist', originDialogContentSharewishlist);

}());