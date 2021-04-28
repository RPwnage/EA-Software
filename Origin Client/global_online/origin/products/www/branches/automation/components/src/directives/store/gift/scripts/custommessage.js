/**
 * @file gift/script/custommessage.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-gift-custommessage';
    var FRIEND_TILE_CONTEXT_NAMESPACE = 'origin-store-gift-friendtile';
    var MAX_MESSAGE_LENGTH = 256;

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStoreGiftCustommessage(ComponentsConfigFactory, UtilFactory, GamesDataFactory, GiftFriendListFactory, ObjectHelperFactory, GiftingWizardFactory) {

        function originStoreGiftCustommessageLink(scope, element) {

            var defaultPackArt = ComponentsConfigFactory.getImagePath('packart-placeholder.jpg');
            var textArea = element.find('textarea');
            scope.strings = {
                backStr: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'back-str'),
                nextStr: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'next-str'),
                yournameHint: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'yourname-hintstr'),
                fromStr: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'from-str'),
                titleStr: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'title-str'),
                yourMessageHint: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'your-message-hint-str'),
                nameRequiredError: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'fullname-required-error-str')
            };


            scope.maxMessageLength = MAX_MESSAGE_LENGTH;
            scope.message = '';
            scope.recipientName = Origin.user.originId();

            function setTextAreaCharactersRemaining(remainingCharacters) {
                scope.charsRemaining = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'chars-remaining-str', {'%count%': remainingCharacters});
            }

            function processUserInfo(userData) {
                scope.username = _.get(userData,'username');
            }

            function processLoggedInUserInfo(userData) {
                if (userData.firstname || userData.lastname) {
                    scope.recipientName = UtilFactory.getLocalizedStr(false, FRIEND_TILE_CONTEXT_NAMESPACE, 'first-lastname-str', {
                        '%firstname%': userData.firstname || '',
                        '%lastname%': userData.lastname || ''
                    });
                }
            }

            function processGameInfo(catalogData) {
                scope.packArtLarge = _.get(catalogData, ['i18n', 'packArtLarge'], defaultPackArt);
                scope.displayName = _.get(catalogData, ['i18n', 'displayName']);
            }

            function getGameInfo() {
                return GamesDataFactory.getCatalogInfo([scope.offerId])
                    .then(ObjectHelperFactory.getProperty(scope.offerId))
                    .then(processGameInfo);
            }

            function getUserInfo() {
                var userInfo = _.get(GiftFriendListFactory.getSelectedUser(), ['userData']);
                if ((scope.nucleusId === _.get(userInfo, 'nucleusId')) && userInfo.username) {
                    return Promise.resolve(userInfo)
                        .then(processUserInfo);
                } else {
                    return GiftFriendListFactory.getUserInfo({nucleusId: scope.nucleusId})
                        .then(processUserInfo);
                }
            }

            function getLoggedInUserInfo() {
                return GiftFriendListFactory.getUserInfo({nucleusId: Origin.user.personaId()})
                    .then(processLoggedInUserInfo);
            }

            function onPromiseAllCompletion() {
                scope.strings.sendingHint = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'sending-hint-str', {
                    '%displayName%': scope.displayName,
                    '%username%': scope.username
                });
                scope.onInputBlur();
                scope.$digest();
            }

            scope.onInputBlur = function () {
                scope.isValid = scope.recipientName ? scope.recipientName.trim().length > 0 : false;
            };


            scope.onTextAreaChange = function () {
                //account for new lines
                var text = textArea.val().replace(/\r\n|\n/g, '--');
                var charsRemaining = MAX_MESSAGE_LENGTH - text.length;
                setTextAreaCharactersRemaining(charsRemaining);
            };


            scope.onNext = function () {
                GiftingWizardFactory.continueToCheckout(scope.offerId, scope.nucleusId, scope.recipientName, scope.message);
            };

            scope.onBack = function () {
                GiftingWizardFactory.startFriendListGiftingFlow(scope.offerId);
            };

            scope.onInputBlur();
            setTextAreaCharactersRemaining(MAX_MESSAGE_LENGTH);
            Promise.all([getGameInfo(), getUserInfo(), getLoggedInUserInfo()])
                .then(onPromiseAllCompletion);

        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                nucleusId: '@nucleusid',
                hidebackbtn: '@'
            },
            link: originStoreGiftCustommessageLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/gift/views/custommessage.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGiftCustommessage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {string} offerid desc
     * @param {string} nucleusid desc
     * @param {string} firstname desc
     * @param {string} lastname desc
     * @param {string} username desc
     * @param {BooleanEnumeration} hidebackbtn desc
     *
     * @param {LocalizedString} back-str Back button text
     * @param {LocalizedString} next-str Next button text
     * @param {LocalizedString} yourname-hintstr Your name hint for input field
     * @param {LocalizedString} from-str From text
     * @param {LocalizedString} title-str Modal title text
     * @param {LocalizedString} your-message-hint-str Your message hint text for text area
     * @param {LocalizedString} sending-hint-str Sending text must include %displayName% and %username%
     * @param {LocalizedString} chars-remaining-str Characters left. Must include %count%
     * @param {LocalizedString} fullname-required-error-str error message when yourname input field is empty
     *
     * @example
     *  <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-gift-custommessage></origin-store-gift-custommessage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreGiftCustommessage', originStoreGiftCustommessage);
}());