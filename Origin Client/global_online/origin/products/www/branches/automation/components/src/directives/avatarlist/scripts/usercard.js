/**
 * @file avatarlist/scripts/usercard.js
 */
(function() {
    'use strict';

    // classes
    var CLS_AVATARLIST_USERCARD = 'origin-avatarlist-usercard',
        CLS_AVATARLIST_USERCARD_FADEIN = 'origin-avatarlist-usercard-fadein',
        CLS_AVATARLIST_USERCARD_VISIBLE = 'origin-avatarlist-usercard-isvisible',
        CLS_USERCARD = 'origin-usercard',
        CLS_USERCARD_ARROW = 'origin-usercard-arrow';

    // constants
    var HALF_AVATAR_WIDTH = 20.5;

    /**
     * Friends Playing Game directive
     * @directive originGameFriendsplayingDisplay
     */
    function originAvatarlistUsercard(ComponentsConfigFactory, UtilFactory) {

        /**
         * Friends Playing Game Link
         * @link originGameFriendsplayingDisplayLink
         */
        function originAvatarlistUsercardLink(scope, element) {

            var avatarListUserCardElm = element.find('.' + CLS_AVATARLIST_USERCARD),
                avatarListUserCard = avatarListUserCardElm[0],
                avatarListItem = angular.element(avatarListUserCard.parentNode.parentNode),
                usercard,
                arrow,
                timeoutHandle;


            /**
            * Get the new position of the usercard
            * @return {Number} new position of usercard
            * @method
            */
            function getUsercardPos() {
                var dims = avatarListUserCard.getBoundingClientRect(),
                    dimsUsercard = usercard.getBoundingClientRect(), // these shouldn't change
                    screenWidth = window.innerWidth;
                if (dimsUsercard.left < 0) {
                    return (-1 * (dimsUsercard.left - HALF_AVATAR_WIDTH));
                } else if (dims.right > screenWidth) {
                    return (HALF_AVATAR_WIDTH - (dims.right - screenWidth));
                } else {
                    return null;
                }
            }

            /**
            * Ensure that the usercard is within the bounds of the screen
            * @return {void}
            * @method
            */
            function bindUserCardToScreen() {
                var usercardLeft = getUsercardPos();
                if (usercardLeft !== null) {
                    avatarListUserCard.style.left = usercardLeft + 'px';
                    arrow.style.marginLeft = (-1 * usercardLeft + HALF_AVATAR_WIDTH) + 'px';
                }
                avatarListUserCardElm.addClass(CLS_AVATARLIST_USERCARD_FADEIN);
            }

            /**
            * Show the usercard
            * @return {void}
            * @method
            */
            function onmouseenter() {
                avatarListUserCardElm.addClass(CLS_AVATARLIST_USERCARD_VISIBLE);
                bindUserCardToScreen();
            }

            /**
            * Fade the usercard out
            * @return {void}
            * @method
            */
            function onmouseleave() {
                avatarListUserCardElm.removeClass(CLS_AVATARLIST_USERCARD_FADEIN);
                UtilFactory.onTransitionEnd(avatarListUserCardElm, function() {
                    if (!avatarListUserCardElm.hasClass(CLS_AVATARLIST_USERCARD_FADEIN)) {
                        avatarListUserCardElm.removeClass(CLS_AVATARLIST_USERCARD_VISIBLE);
                    }
                });
            }

            function initUserCardDomReferences() {
                usercard = element.find('.' + CLS_USERCARD)[0];
                arrow = element.find('.' + CLS_USERCARD_ARROW)[0];
            }

            function onDestroy() {
                clearTimeout(timeoutHandle);
                avatarListItem.off('mouseenter', onmouseenter);
                avatarListItem.off('mouseleave', onmouseleave);
            }

            scope.$on('$destroy', onDestroy);
            timeoutHandle = setTimeout(initUserCardDomReferences, 0);
            avatarListItem.on('mouseenter', onmouseenter);
            avatarListItem.on('mouseleave', onmouseleave);

        }

        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('avatarlist/views/usercard.html'),
            link: originAvatarlistUsercardLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originAvatarlistUsercard
     * @restrict E
     * @element ANY
     * @scope
     *
     * Avatarlist Usercard
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-avatarlist-usercard>
     *              <origin-avatar></origin-avatar>
     *          </origin-avatarlist-usercard>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originAvatarlistUsercard', originAvatarlistUsercard);

}());