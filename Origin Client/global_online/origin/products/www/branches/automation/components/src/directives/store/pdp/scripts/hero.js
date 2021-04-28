/**
 * @file store/pdp/hero.js
 */

(function () {
    'use strict';
    /**
     * KeybeatIconTypeEnumeration list of keybeat icons
     * @enum {string}
     * @todo update icon list with appropriate icons
     */
    /* jshint ignore:start */
    var KeybeatIconTypeEnumeration = {
        "No Icon": "",
        "General Origin Keybeat": "originlogo",
        "New Release": "newrelease",
        "DLC": "dlc",
        "Trial": "play-with-circle",
        "Video Available": "video",
        "Pre-order Available": "preorder",
        "Early Access": "earlyaccess",
        "Achievement": "trophy",
        "Save": "store",
        "Expiring": "expiring",
        "Expired": "expired"
    };
    /**
     * BooleanTypeEnumeration list of allowed types
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var PDP_HERO_SELECTOR = '.origin-pdp-hero',
        PDP_HERO_MAIN_SELECTOR = '.origin-pdp-hero-main',
        PDP_HERO_VIDEO_FRAME_SELECTOR = '.origin-pdp-hero-video-frame',
        ASPECT_RATIO = 9/16, //Video aspect ratio for 16:9
        VIDEO_PLAY_EVENT = 'YoutubePlayer:Play',
        VIDEO_STOP_EVENT = 'YoutubePlayer:Stop',
        CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    /**
     * Controller for PDP hero and announcement hero
     */
    function originStorePdpHeroCtrl($scope, YoutubeFactory, DirectiveScope, CSSUtilFactory) {

        function setupBackgroundImage() {
            if (!$scope.backgroundImage) {
                $scope.backgroundImage = YoutubeFactory.getStillImage($scope.videoId);
            }
            $scope.videoReady = false;
            $scope.showVideo = false;
        }

        $scope.init = function(element, contextNamespace) {
            DirectiveScope.populateScope($scope, contextNamespace).then(function () {
                if ($scope.videoId) {
                    setupBackgroundImage();
                }
                $scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement($scope.backgroundColor, element.find(PDP_HERO_SELECTOR), contextNamespace);
                $scope.$digest();
            });
        };

        $scope.$on(VIDEO_STOP_EVENT, function(e){
            $scope.showVideo = false;
            e.stopPropagation();
            $scope.removeInlineStyle();
        });

        $scope.videoButtonHandler = function () {
            $scope.showVideo = true;
            $scope.$broadcast(VIDEO_PLAY_EVENT);
            $scope.updateMarginTop();
        };

        this.updateMarginTop = function() {
            $scope.updateMarginTop();
        };

    }

    function originStorePdpHero(ComponentsConfigFactory) {
        function originStorePdpHeroLink(scope, element) {

            scope.updateMarginTop = function(){
                var videoHeight = element.find(PDP_HERO_VIDEO_FRAME_SELECTOR).width() * ASPECT_RATIO;
                element.find(PDP_HERO_MAIN_SELECTOR).css('margin-top', videoHeight);
            };

            scope.removeInlineStyle = function(){
                element.find(PDP_HERO_MAIN_SELECTOR).removeAttr('style');
            };

            scope.init(element, CONTEXT_NAMESPACE);
        }


        return {
            restrict: 'E',
            scope: {
                activeTrial: '@',
                addToMyGames: '@',
                availableWithSubsText: '@',
                backgroundColor: '@',
                backgroundImage: '@',
                download: '@',
                expiredEarlyAccessTrialGameNotPreOrdered: '@',
                expiredEarlyAccessTrialGamePreOrdered: '@',
                expiredTrial: '@',
                getBeta: '@',
                getItNow: '@',
                getTrial: '@',
                inactiveTrial: '@',
                keybeatIcon: '@',
                keybeatMessage: '@',
                manualDiscountOverride: '@',
                manualSubscriberTrialOverride: '@',
                manualTrialOverride: '@',
                matureContent: '@',
                nonsubscriberAlternateEditionInVaultHeader: '@',
                nonsubscriberAlternateEditionInVaultText: '@',
                nonvaultDiscountUpsell: '@',
                nonVaultGameEarlyTrialAndGameReleased: '@',
                nonVaultGameEarlyTrialNotReleased: '@',
                nonVaultGameEarlyTrialReleased: '@',
                nonvaultTrialUpsellAfter: '@',
                nonvaultTrialUpsellBefore: '@',
                nonvaultTrialUpsellMiddle: '@',
                nonvaultupsellHeader: '@',
                nonvaultupsellLearnMore: '@',
                notExpiredEarlyAccessTrial: '@',
                oaDiscountApplied: '@',
                orText: '@',
                platformText: '@',
                playForZeroWith: '@',
                playNow: '@',
                preLoadedTextMessage: '@',
                preLoadText: '@',
                preOrderText: '@',
                readMoreText: '@',
                restrictedMessage: '@',
                seeDetails: '@',
                startNewTrial: '@',
                subscriberVaultGameHeader: '@',
                subscriberVaultUpgradeGameHeader: '@',
                tryDemo: '@',
                upgradeNow: '@',
                vaultEditionAvailableToSubscriber: '@',
                vaultEditionOwnedBySubscriber: '@',
                vaultGameOthHeader: '@',
                vaultGameOthText: '@',
                vaultupsellHeader: '@',
                vaultupsellLearnMore: '@',
                vaultupsellPointOne: '@',
                vaultupsellPointTwo: '@',
                videoId: '@',
                viewInLibrary: '@',
                youOwnText: '@',
                youPreorderedText: '@'
            },
            link: originStorePdpHeroLink,
            controller: 'originStorePdpHeroCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/hero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpHero
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} background-image (optional) background image URL
     * @param {KeybeatIconTypeEnumeration} keybeat-icon icon to use // should be enum
     * @param {LocalizedString} purchase-seperately *Only uses default value comment about being able to purchase this separately.
     * @param {LocalizedString|OCD} platform-text (optional) platform label
     * @param {LocalizedString|OCD} read-more-text (optional) read more text override
     * @param {LocalizedString} add-to-my-games add to my games text. Set up once as default. For Sub directive
     * @param {LocalizedString} badinput bad input text. Set up once as default. For Sub directive
     * @param {LocalizedString} basegamerequiredstr base game required text. Set up once as default. For Sub directive
     * @param {LocalizedString} errornotknown error not known text. Set up once as default. For Sub directive
     * @param {LocalizedString} keybeat-message text message
     * @param {LocalizedString} manual-discount-override Subscriptions discount upsell manual override
     * @param {LocalizedString} manual-subscriber-trial-override Subscriber trial upsell override
     * @param {LocalizedString} manual-trial-override Subscriptions trial upsell manual override
     * @param {LocalizedString} non-vault-game-early-trial-and-game-released Subscriber early access upsell after releases
     * @param {LocalizedString} non-vault-game-early-trial-not-released Subscriber early access upsell before releases
     * @param {LocalizedString} non-vault-game-early-trial-released Subscriber early access upsell between releases
     * @param {LocalizedString} oa-discount-applied oa discount applied text. Set up once as default. For Sub directive
     * @param {LocalizedString} oktext cta ok text. Set up once as default. For Sub directive pdp-main
     * @param {LocalizedString} pre-load-text pre load text. Set up once as default. For Sub directive
     * @param {LocalizedString} pre-loaded-text-message User has preloaded ownership string
     * @param {LocalizedString} pre-order-text pre order text. Set up once as default. For Sub directive
     * @param {LocalizedString} vault-edition-available-to-subscriber vault edition available to subscriber text. Set up once as default. For Sub directive
     * @param {LocalizedString} vault-edition-owned-by-subscriber vault edition owned by subscriber text. Set up once as default. For Sub directive
     * @param {LocalizedString} you-own-text you own this edition text. Set up once as default. For Sub directive ownership
     * @param {LocalizedString} you-preordered-text you preordered this text. Set up once as default. For Sub directive
     * @param {LocalizedString} youownthis you own this text. Set up once as default. For Sub directive
     * @param {LocalizedString} youownthisedition cta you own this edition text. Set up once as default. For Sub directive purchase dialog
     * @param {string} background-color (optional) background color for the hero image in hex format ('#ffffff')
     * @param {LocalizedString} save-text the save text
     *
     * Vault up sell
     * @param {LocalizedString} or-text or text. Set up once as default. For Sub directive
     * @param {LocalizedString} subscriber-vault-game-header subscriber vault game header text. Set up once as default. For Sub directive
     * @param {LocalizedString} vaultupsell-header vault up sell header text. Set up once as default. For Sub directive
     * @param {LocalizedString} vaultupsell-learn-more vault up sell learn more. Set up once as default. For Sub directive
     * @param {LocalizedString} vaultupsell-point-one vault up sell point one text. Set up once as default. For Sub directive
     * @param {LocalizedString} vaultupsell-point-two vault up sell point two text. Set up once as default. For Sub directive
     * @param {LocalizedTemplateString} play-for-zero-with play for zero with text. Set up once as default. For Sub directive
     *
     * Non vault up sell
     * @param {LocalizedString} download download text. Set up once as default. For Sub directive
     * @param {LocalizedString} expired-early-access-trial-game-not-pre-ordered expired early access trial game not pre ordered. Set up once as default. For Sub directive
     * @param {LocalizedString} expired-early-access-trial-game-pre-ordered expired early access trial game pre ordered text. Set up once as default. For Sub directive
     * @param {LocalizedString} get-it-now get it now text. Set up once as default. For Sub directive
     * @param {LocalizedString} nonsubscriber-alternate-edition-in-vault-header non subscriber alternate edition in vault header text. Set up once as default. For Sub directive
     * @param {LocalizedString} nonvault-discount-upsell non vault discount upsell text. Set up once as default. For Sub directive
     * @param {LocalizedString} nonvault-trial-upsell-after non vault trial upsell after text. Set up once as default. For Sub directive
     * @param {LocalizedString} nonvault-trial-upsell-before non vault up sell before text. Set up once as default. For Sub directive
     * @param {LocalizedString} nonvault-trial-upsell-middle non vault trial upsell middle text. Set up once as default. For Sub directive
     * @param {LocalizedString} nonvault-upsell-learn-more non vault upsell learn more text. Set up once as default. For Sub directive
     * @param {LocalizedString} not-expired-early-access-trial not expired early access trial text. Set up once as default. For Sub directive
     * @param {LocalizedString} play-now play now text. Set up once as default. For Sub directive
     * @param {LocalizedString} see-details see details text. Set up once as default. For Sub directive
     * @param {LocalizedString} subscriber-vault-upgrade-game-header subscriber vault upgrade game header text. Set up once as default. For Sub directive
     * @param {LocalizedString} upgrade-now upgrade now text. Set up once as default. For Sub directive
     * @param {LocalizedString} vault-game-oth-header vault game oth header text. Set up once as default. For Sub directive
     * @param {LocalizedTemplateString} nonsubscriber-alternate-edition-in-vault-text non subscriber alternate edition in vault text. Set up once as default. For Sub directive
     * @param {LocalizedTemplateString} nonvault-upsell-header non vault up sell header text. Set up once as default. For Sub directive
     * @param {LocalizedTemplateString} vault-game-oth-text vault game oth text. Set up once as default. For Sub directive
     *
     * origin-store-pdp-entitlementarea
     * @param {LocalizedString} active-trial active trial text. Set up once as default. For Sub directive
     * @param {LocalizedString} expired-trial expired trial text. Set up once as default. For Sub directive
     * @param {LocalizedString} get-beta get beta text. Set up once as default. For Sub directive
     * @param {LocalizedString} get-trial get trial text. Set up once as default. For Sub directive
     * @param {LocalizedString} start-new-trial start new trial text. Set up once as default. For Sub directive
     * @param {LocalizedString} try-demo try demo text. Set up once as default. For Sub directive
     * @param {LocalizedString} unknown-trial-time unknown trial time text. Set up once as default. For Sub directive
     * @param {LocalizedString} view-in-library view in library text. Set up once as default. For Sub directive
     *
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {LocalizedString} restricted-message Message to show when age-restricted
     * @param {Video} video-id YT video id
     * @description
     *
     * PDP hero (header container)
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-hero background-image="https://docs.x.origin.com/proto/images/keyart/battlefront.jpg"></origin-store-pdp-hero>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('originStorePdpHeroCtrl', originStorePdpHeroCtrl)
        .directive('originStorePdpHero', originStorePdpHero);
}());
