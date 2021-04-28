/**
 * @file store/promo/scripts/timer.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */


    var RatingEnumeration = {
        "Light": "light",
        "Dark": "dark"
    };
    /* jshint ignore:end */

    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var LayoutMap = {
        "hero": "l-origin-store-column-hero l-origin-section-divider",
        "full-width": "l-origin-store-column-full",
        "half-width": "l-origin-store-column-half"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-store-promo-timer';

    function OriginStorePromoTimerCtrl($scope, $element, CSSUtilFactory, OriginStorePromoConstant, UtilFactory) {
        // enable the timer
        $scope.timer = true;
        $scope.timerVisible = true;
        $scope.type = 'timer';
        $scope.strings = {};

        // Handle the timer expiry
        $scope.$on('timerReached', function() {
            if($scope.hideTimer === BooleanEnumeration.true) {
                $scope.timerVisible = false;
            }
            $scope.timeReached = true;
            setTitleAfterTimerExpired();
        });

        function setTitleAfterTimerExpired() {
            if ($scope.timeReached) {
                $scope.titleStr = $scope.postTitle || $scope.titleStr;
            }
        }

        function getHref() {
            if (!$scope.href) {
                return  $scope.ocdPath ? 'store' + $scope.ocdPath : null;
            }
            return $scope.href;
        }

        this.onScopeResolved = function() {
            if ($scope.layout) {
                $element.addClass(LayoutMap[$scope.layout]).addClass(OriginStorePromoConstant.layoutClass);
                if ($scope.layout !== LayoutEnumeration.hero) {
                    $scope.badgeImage = null;
                }
            }
            setTitleAfterTimerExpired(); //need to call this again in case ocd params (title and postTitle) were not set when timer expired

            $scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement($scope.backgroundColor, $element, CONTEXT_NAMESPACE);
            $scope.strings.othText = UtilFactory.getLocalizedStr($scope.othText, CONTEXT_NAMESPACE, 'oth-text');
            $scope.href = getHref();

            $scope.useH1 = $scope.useH1Tag === BooleanEnumeration.true;
            $scope.useH2 = !$scope.useH1;

            $scope.isPackartShown = $scope.titleImage ? BooleanEnumeration.false : $scope.showPackart;
            $scope.$watchOnce('model', function(){
                $scope.isOth = $scope.model.oth;
            });
            $scope.showAccessLogo = $scope.showAccessBadge === BooleanEnumeration.true;
        };
    }

    function originStorePromoTimer(ComponentsConfigFactory, DirectiveScope) {
        function originStorePromoTimerLink (scope, element, attributes, controller) {
            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath)
                .then(controller.onScopeResolved);
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                layout: '@',
                image : '@',
                titleStr : '@',
                href : '@',
                ctaText: '@',
                videoid : '@',
                matureContent: '@',
                ocdPath: '@',
                showPackart: '@',
                goaldate: '@',
                postTitle: '@',
                hideTimer: '@',
                backgroundColor: '@',
                badgeImage: '@',
                ratingFontColor: '@',
                useH1Tag: '@',
                showAccessBadge: '@',
                titleImage: '@'
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
     * @param {ImageUrl} badge-image optional badge image. Only applicable to hero layout
     * @param {LocalizedString} title-str Promo title
     * @param {Url} href Url that this module links to
     * @param {LocalizedString} cta-text Text for the call to action. Ex. "Learn More" (Optional).
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {string} background-color Hex value of the background color
     * @param {OcdPath} ocd-path OCD Path
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from the ocdpath will be displayed
     * @param {DateTime} goaldate The timer end date and time. (UTC time)
     * @param {LocalizedString} post-title The new title to switch to after the timer expires (Optional).
     * @param {BooleanEnumeration} hide-timer Hide the timer when it expires
     * @param {RatingEnumeration} rating-font-color color of the ratings font (TW only)
     * @param {String} ctid Content tracking ID
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {BooleanEnumeration} show-access-badge shows access badge above title in the banner
     * @param {ImageUrl} title-image title image. Will be used only if title str is not merchandised
     * @param {LocalizedString} oth-text * on the house text.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-promo-timer
     *        layout="hero"
     *        image="https://docs.x.origin.com/proto/images/staticpromo/staticpromo_titanfall_@1x.jpg"
     *        title-str="Save Big On All Things Titanfall"
     *        href="app.store.wrapper.bundle-blackfriday"
     *        cta-text="Learn More"
     *        videoid="wS7gfJqFoA"
     *        mature-content="true"
     *        ocd-path="Origin.OFR.50.0000979"
     *        show-packart="true"
     *        goaldate="2016-02-01 09:10:30 UTC"
     *        post-title="This offer has ended"
     *        hide-timer="true">
     *      </origin-store-promo-timer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .constant('OriginStorePromoConstant', {
            layoutClass: 'origin-store-promo',
            clickableClass: 'origin-store-promo-isclickable'
        })
        .controller('OriginStorePromoTimerCtrl', OriginStorePromoTimerCtrl)
        .directive('originStorePromoTimer', originStorePromoTimer);
}());
