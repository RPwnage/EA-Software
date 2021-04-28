/**
 * @file store/pdp/announcement/scripts/hero.js
 */

(function () {
    'use strict';

    /* jshint ignore:start */


    var PlatformsEnumeration = {
        "PCWIN": "PCWIN",
        "MAC": "MAC"
    };
    /* jshint ignore:end */

    /**
     * BooleanEnumeration
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-store-announcement-hero',
        ASPECT_RATIO = 9/16, //Video aspect ratio for 16:9
        PDP_HERO_MAIN_SELECTOR = '.origin-pdp-hero-main',
        PDP_HERO_VIDEO_SELECTOR = '.origin-pdp-hero-video';

    function originStoreAnnouncementHero(ComponentsConfigFactory) {

        function originStoreAnnouncementHeroLink(scope, element) {
            scope.updateMarginTop = function(){
                var videoHeight = element.find(PDP_HERO_VIDEO_SELECTOR).width() * ASPECT_RATIO;
                element.find(PDP_HERO_MAIN_SELECTOR).css('margin-top', videoHeight);
            };

            scope.removeInlineStyle = function(){
                element.find(PDP_HERO_MAIN_SELECTOR).removeAttr('style');
            };


            scope.isFacebookHidden = scope.hideFacebook === BooleanEnumeration.true;

            scope.init(element, CONTEXT_NAMESPACE);
        }


        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                packArt: '@',
                videoId: '@',
                matureContent: '@',
                shortDescription: '@',
                longDescription: '@',
                backgroundImage: '@',
                backgroundColor: '@',
                releaseDateText: '@',
                preloadDateText: '@',
                nowAvailableText: '@',
                readMoreText: '@',
                actionButtonText: '@',
                actionButtonHref: '@',
                restrictedMessage: '@',
                hideFacebook: '@',
                retireMessage: '@',
                learnMoreText: '@',
                gameRatingUrl: '@',
                platforms: '@',
                platformText: '@',
                gameRatingSystemIcon: '@',
                gameRatingSystemIconAlt: '@',
                gameRatingDescriptionShort: '@',
                gameRatingDescriptionLong: '@'
            },
            link: originStoreAnnouncementHeroLink,
            controller: 'originStorePdpHeroCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/announcement/views/hero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAnnouncementHero
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str (optional) title override
     * @param {ImageUrl} pack-art (optional) pack art override
     * @param {LocalizedString} short-description (optional) product subtitle override
     * @param {LocalizedString} long-description (optional) product description teaser override
     * @param {ImageUrl} background-image (optional) background image URL
     * @param {string} background-color (optional) background color for the hero image in hex format ('#ffffff')
     * @param {LocalizedString} release-date-text (optional) release date info string override
     * @param {LocalizedString} preload-date-text (optional) preload date info string override
     * @param {LocalizedString} now-available-text (optional) preload availability message override
     * @param {LocalizedString} read-more-text (optional) read more text override
     * @param {LocalizedString} action-button-text (optional) action button text override
     * @param {Url} action-button-href (optional) action button url override
     * @param {Video} video-id YT video id
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {PlatformsEnumeration} platforms (optional) game platform
     * @param {LocalizedString} platform-text (optional) platform label
     * @param {LocalizedString} restricted-message Message to show when age-restricted
     * @param {LocalizedString} learn-more-text (optional) learn more text override
     * @param {LocalizedString} retire-message (optional) retire game title message
     * @param {Url} game-rating-url (optional) game rating url
     * @param {BooleanEnumeration} hide-facebook Hide Facebook share button
     * @param {ImageUrl} game-rating-system-icon (optional) game rating icon
     * @param {LocalizedString} game-rating-system-icon-alt (optional) game rating icon text
     * @param {LocalizedString} game-rating-description-short (optional) game rating short description (| separated list)
     * @param {LocalizedText} game-rating-description-long (optional) game rating long description
     * @description
     *
     * PDP hero (header container)
     *
     * @example
     * <origin-store-row>
     *     <origin-store-announcement-hero offer-id="{{ offerId }}" background-image="https://docs.x.origin.com/proto/images/keyart/battlefront.jpg"></origin-store-announcement-hero>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStoreAnnouncementHero', originStoreAnnouncementHero);
}());
