/**
 * @file store/carousel/hero/scripts/timerbanner.js
 */
(function () {
    'use strict';
    /* jshint ignore:start */
    var ExpiredDateTime = {
        "format" : "F j Y H:i:s T"
    };
    /* jshint ignore:end */

    var hideEnumeration = {
        "true": true,
        "false": false
    };


    function OriginStoreCarouselHeroTimerbannerCtrl($scope, $location) {

       // This seems like common banner logic.
       $scope.onBtnClick = function() {
            $location.path($scope.href);
        };

        $scope.$on('timerReached', function() {
            if(hideEnumeration[$scope.hideTimer]) {
                $scope.timerActive = false;
            }
            $scope.title = $scope.postTitle ? $scope.postTitle : $scope.title;
        });

        $scope.hideTimer = false;
        $scope.timerActive = true;
        $scope.type = 'timer';
    }

    function originStoreCarouselHeroTimerbanner(ComponentsConfigFactory) {

    	function originStoreCarouselHeroTimerbannerLink(scope) {
            var alignment = scope.alignment;
            scope.contentClass = 'carousel-caption-';
            scope.contentClass += (alignment) ? alignment : 'left';
    	}

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
            	title: '@',
                subtitle : '@',
            	image: '@',
                href: '@',
                postTitle: '@',
                goalDate: '@',
                hideTimer: '@',
                videoid: '@',
                restrictedage: '@',
                videoDescription: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/hero/views/banner.html'),
            controller: OriginStoreCarouselHeroTimerbannerCtrl,
            link: originStoreCarouselHeroTimerbannerLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselHeroTimerbanner
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The title of the module
     * @param {ImageUrl} image  The banner image
     * @param {LocalizedString} subtitle The subtitle of the module shown after the timer expires
     * @param {string} href Url to something
     * @param {LocalizedString} post-title The title to show after the timer has expired
     * @param {hideEnumeration} hide-timer hide the clock and show the title after the timer is expired or show a timer with all zeros
     * @param {ExpiredDateTime} goal-date timer expire date
     * @param {string} videoid The youtube video ID of the video for this promo (Optional).
     * @param {Number} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {LocalizedString} video-description The text for the video cta
     *
     */
    angular.module('origin-components')
    	.controller('OriginStoreCarouselHeroTimerbannerCtrl', OriginStoreCarouselHeroTimerbannerCtrl)
        .directive('originStoreCarouselHeroTimerbanner', originStoreCarouselHeroTimerbanner);
}());
