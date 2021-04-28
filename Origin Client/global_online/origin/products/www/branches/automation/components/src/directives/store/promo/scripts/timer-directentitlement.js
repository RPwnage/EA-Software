/**
 * @file store/promo/scripts/timer-directentitlement.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var RatingEnumeration = {
        "Light": "light",
        "Dark": "dark"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-promo-timer-directentitlement';

    function originStorePromoTimerDirectentitlement(ComponentsConfigFactory, DirectiveScope) {

        function originStorePromoTimerLink (scope, element, attributes, controller) {
            scope.isEntitlement = true;

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
                videoid : '@',
                matureContent: '@',
                ocdPath: '@',
                showPackart: '@',
                goaldate: '@',
                postTitle: '@',
                hideTimer: '@',
                offerId: '@',
                backgroundColor: '@',
                badgeImage: '@',
                ratingFontColor: '@',
                useH1Tag: '@',
                othText: '@',
                titleImage: '@'
            },
            controller: 'OriginStorePromoTimerCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/timer.html'),
            link: originStorePromoTimerLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoTimerDirectentitlement
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with a countdown clock as its content.
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} badge-image optional badge image. Only applicable to hero layout
     * @param {LocalizedString} title-str Promo title
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {string} background-color Hex value of the background color
     * @param {OcdPath} ocd-path OCD Path
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from the ocdpath will be displayed
     * @param {DateTime} goaldate The timer end date and time. (UTC time)
     * @param {LocalizedString} post-title The new title to switch to after the timer expires (Optional).
     * @param {BooleanEnumeration} hide-timer Hide the timer when it expires
     * @param {OfferId} offer-id desc
     * @param {RatingEnumeration} rating-font-color color of the ratings font (TW only)
     * @param {String} ctid Content tracking ID
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {LocalizedString} oth-text * on the house text.
     * @param {ImageUrl} title-image title image. Will be used only if title str is not merchandised
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-promo-timer-directentitlement
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
     *      </origin-store-promo-timer-directentitlement>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoTimerDirectentitlement', originStorePromoTimerDirectentitlement);
}());
