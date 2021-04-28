/** 
 * @file store/promo/scripts/timer.js
 */ 
(function(){
    'use strict';

    /* jshint ignore:start */
    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };
    /* jshint ignore:end */
    
    var LayoutMap = {
        "hero": "l-origin-store-column-hero",
        "full-width": "l-origin-store-column-full",
        "half-width": "l-origin-store-column-half"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    function OriginStorePromoTimerCtrl($scope) {
        // enable the timer
        $scope.timer = true;
        $scope.timerVisible = true;
        $scope.type = 'timer';

        // Handle the timer expiry
        $scope.$on('timerReached', function() {
            if(BooleanEnumeration[$scope.hideTimer] === "true") {
                $scope.timerVisible = false;
            }
            $scope.title = $scope.postTitle || $scope.title;
        });
    }

    function originStorePromoTimer(ComponentsConfigFactory) {
        function originStorePromoTimerLink (scope, element) {
            element.addClass(LayoutMap[scope.layout]).addClass('origin-store-promo');
        }

        return {
            restrict: 'E',
            scope: {
                layout: '@',
                image : '@',
                title : '@',
                href : '@',
                ctaText: '@',
                videoid : '@',
                restrictedage: '@',
                startColor: '@',
                endColor: '@',
                offerid: '@',
                showPackart: '@',
                goaldate: '@',
                postTitle: '@',
                hideTimer: '@'
            },
            controller: 'OriginStorePromoTimerCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/timer.html'),
            link: originStorePromoTimerLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoTimer
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with a countdown clock as its content. 
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} title Promo title
     * @param {Url} href Url that this module links to
     * @param {LocalizedString} cta-text Text for the call to action. Ex. "Learn More" (Optional).
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {string} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {string} start-color The start of the fade color for the background
     * @param {string} end-color The end color/background color of the module
     * @param {OfferId} offerid the offer id
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from the offerid will be displayed
     * @param {DateTime} goaldate The timer end date and time.
     * @param {LocalizedString} post-title The new title to switch to after the timer expires (Optional).
     * @param {BooleanEnumeration} hide-timer Hide the timer when it expires
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-promo-timer
     *        layout="hero"
     *        image="http://docs.x.origin.com/origin-x-design-prototype/images/staticpromo/staticpromo_titanfall_@1x.jpg"
     *        title="Save Big On All Things Titanfall"
     *        href="app.store.wrapper.bundle-blackfriday"
     *        cta-text="Learn More"
     *        videoid="wS7gfJqFoA"
     *        restrictedage="18"
     *        start-color="rgba(30, 38, 44, 0)"
     *        end-color="rgba(30, 38, 44, 1)"
     *        offerid="Origin.OFR.50.0000979"
     *        show-packart="true"
     *        goaldate="2016-02-01 09:10:30 UTC"
     *        post-title="This offer has ended"
     *        hide-timer="true">
     *      </origin-store-promo-timer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePromoTimerCtrl', OriginStorePromoTimerCtrl)
        .directive('originStorePromoTimer', originStorePromoTimer);
}()); 
