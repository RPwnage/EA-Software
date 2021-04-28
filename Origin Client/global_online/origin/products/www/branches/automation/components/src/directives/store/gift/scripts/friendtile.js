/**
 * @file store/gift/scripts/friendtile.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-gift-friendtile';

    function originStoreGiftFriendtile(ComponentsConfigFactory, UtilFactory, GiftFriendListFactory) {

        function originStoreGiftFriendtileLink(scope) {

            scope.strings = {
                //keys have to match status coming back from server.
                'eligible': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'eligible-str'),
                'not_eligible': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'not-eligible-str'),
                'ineligible_to_receive': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'ineligible-to-receive-str'),
                'already_owned': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'owns-game-str'),
                'on_wishlist': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'on-wishlist-str'),
                'unknown': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'unknown-str')
            };


            scope.onClick = function () {
                scope.selected = !scope.selected;
                GiftFriendListFactory.selectUser(scope.selected ? {userInfo : scope.userInfo, eligibilityCallInProgress: scope.loading }  : null);
            };

            scope.onAvatarClick = function ($event) {
                if ($event) {
                    $event.preventDefault();
                }
            };

            function processUserInfo(userInfo) {
                if (userInfo) {

                    scope.userInfo = userInfo;
                    scope.userInfo.fullname = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'first-lastname-str', {
                        '%firstname%': scope.userInfo.firstname || '',
                        '%lastname%': scope.userInfo.lastname || ''
                    });
                }
            }

            function updateSelectedUser() {
                if (scope.selected) {
                    if (!GiftFriendListFactory.isFriendEligibleToReceive(scope.eligibility)) {
                        GiftFriendListFactory.selectUser(null);
                    } else {
                        GiftFriendListFactory.selectUser({userInfo : scope.userInfo, eligibilityCallInProgress: scope.loading });
                    }
                }
            }

            function processEligibility(eligibilityArray) {
                scope.eligibility = _.get(_.head(eligibilityArray), 'status', 'unknown').toLowerCase();
                scope.eligibilityStr = scope.strings[scope.eligibility];

                updateSelectedUser();
            }

            function handleEligibilityError() {
                GiftFriendListFactory.selectUser(null);
                scope.eligibility = 'unknown';
                scope.eligibilityStr = scope.strings[scope.eligibility];
            }

            function postInit() {
                scope.loading = false;
                updateSelectedUser();
                scope.$digest();
            }

            function updateSelectedStatus(userData) {
                var selectedNucleusId =  _.get(userData, ['userInfo', 'nucleusId']);
                scope.selected = ((scope.nucleusId === selectedNucleusId));
                scope.anotherFriendTileSelected = (!scope.selected && selectedNucleusId);
            }

            function init() {

                scope.loading = true;

                Promise.all([
                    GiftFriendListFactory.getFriendsGiftingEligibility(scope.offerId, [scope.nucleusId])
                        .then(processEligibility)
                        .catch(handleEligibilityError),
                    GiftFriendListFactory.getUserInfo({nucleusId: scope.nucleusId}).then(processUserInfo)
                ]).then(postInit);

                GiftFriendListFactory.getSelectedUserObservable().attachToScope(scope, updateSelectedStatus);
                updateSelectedUser();
            }

            init();
        }

        return {
            restrict: 'E',
            scope: {
                nucleusId: '@',
                offerId: '@',
                isSelected: '@',
                onFriendSelect: '&'
            },
            link: originStoreGiftFriendtileLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/gift/views/friendtile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGiftFriendtile
     * @restrict E
     * @element ANY
     * @scope
     * @description Friend to Gift to
     *
     * @param {Number} nucleus-id friend object
     * @param {string} offer-id desc
     * @param {LocalizedString} first-lastname-str first name/last name text, should have the following %firstname% %lastname%
     * @param {LocalizedString} eligible-str Eligible for gifting text
     * @param {LocalizedString} not-eligible-str Not Eligible for gifting text
     * @param {LocalizedString} ineligible-to-receive-str Not Eligible to recieve (Recieved gift but hasn't accepted) text
     * @param {LocalizedString} owns-game-str Owns the game text
     * @param {LocalizedString} on-wishlist-str On wishlist text
     * @param {LocalizedString} unknown-str unknown text (in case there is an issue with eligibility service)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-gift-friendtile nucleus-id="123456789"></origin-store-gift-friendtile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreGiftFriendtile', originStoreGiftFriendtile);
}());
