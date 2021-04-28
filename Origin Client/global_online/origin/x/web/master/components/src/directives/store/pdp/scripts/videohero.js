
/**
 * @file store/pdp/scripts/videohero.js
 */
(function(){
    'use strict';


    function OriginStorePdpSectionsVideoheroCtrl($scope, YoutubeFactory) {
        $scope.backgroundImage = YoutubeFactory.getStillImage($scope.videoId);
        $scope.videoReady = false;
        $scope.showVideo = false;
        $scope.videoButtonHandler = function(){
                $scope.showVideo = true;
                $scope.$broadcast('YoutubePlayer:Play');
            };
    }

    function originStorePdpVideohero(ComponentsConfigFactory) {

        return {
            scope: {
                offerId: '@',
                packArt: '@',
                shortDescription: '@',
                longDescription: '@',
                backgroundImage: '@',
                releaseDateText: '@',
                preloadDateText: '@',
                nowAvailableText: '@',
                readMoreText: '@',
                availableWithSubsText: '@',
                subsInfoText: '@',
                learnMoreText: '@',
                keybeatMessage: '@',
                keybeatIcon: '@',
                videoId: '@',
                restrictedAge: '@',
                restrictedMessage: '@'
            },
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/hero.html'),
            controller: 'OriginStorePdpSectionsVideoheroCtrl'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpVideohero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} offer-id product offer ID
     * @param {url|OCD} (optional) pack-art pack art override
     * @param {LocalizedString|OCD} short-description (optional) product subtitle override
     * @param {LocalizedString|OCD} long-description (optional) product description teaser override
     * @param {url|OCD} background-image (optional) background image URL
     * @param {LocalizedString|OCD} release-date-text (optional) release date info string override
     * @param {LocalizedString|OCD} preload-date-text (optional) preload date info string override
     * @param {LocalizedString|OCD} now-available-text (optional) preload availability message override
     * @param {LocalizedString|OCD} read-more-text (optional) read more text override
     * @param {string} keybeat-icon icon to use
     * @param {LocalizedString} keybeat-message text message
     * @param {string} video-id YT video id 
     * @param {boolean} restricted-age Whether the video is age-restricted 
     * @param {string} restricted-message Message to show when age-restricted
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-videohero offer-id="{{ offerId }}" background-image="http://docs.x.origin.com/origin-x-design-prototype/images/keyart/battlefront.jpg"></origin-store-pdp-videohero>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSectionsVideoheroCtrl', OriginStorePdpSectionsVideoheroCtrl)
        .directive('originStorePdpVideohero', originStorePdpVideohero);
}());
