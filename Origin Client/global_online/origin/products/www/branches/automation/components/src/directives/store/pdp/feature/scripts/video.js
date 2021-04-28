/**
 * @file store/pdp/feature/video.js
 */
(function () {
    'use strict';

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    /**
     * list text alignment types
     * @readonly
     * @enum {string}
     */
    var TextAlignEnumeration = {
        "Left": "left",
        "Center": "center",
        "Right": "right"
    };

    /**
     * A list of text colors
     * @readonly
     * @enum {string}
     */
    var TextColorsEnumeration = {
        "dark": "dark",
        "light": "light"
    };

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "none": "",
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };

    var VIDEO_IFRAME_SELECTOR = '.origin-background-video-player';
    var VIDEO_CONTAINER_SELECTOR = '.origin-background-video-container';

    var VIDEO_ASPECT_RATIO = 0.5625; // 9 / 16;
    var SCROLL_RESIZE_THROTTLE_MS = 300;

    function sharedLinkFunction(scope, element, attributes, originStorePdpSectionWrapperCtrl, ScreenFactory, AnimateFactory, StoreAgeGateFactory, AuthFactory, UtilFactory) {
        scope.isXSmall = ScreenFactory.isXSmall();
        var $videoContainer = element.find(VIDEO_CONTAINER_SELECTOR),
            $videoElement = element.find(VIDEO_IFRAME_SELECTOR);

        /**
         * Change the visibility of the
         */
        function updateVisibility() {
            scope.isTextInView = ScreenFactory.isOnOrAboveScreen(element);
            scope.$digest();
        }

        /**
         * Should the text alignement be right justified
         * @return {Boolean} true/false
         */
        function isRight() {
            return !scope.isXSmall && (scope.textAlign === TextAlignEnumeration.Right);
        }

        /**
         * Should the text alignement be left justified
         * @return {Boolean} true/false
         */
        function isLeft() {
            return !scope.isXSmall && (scope.textAlign === TextAlignEnumeration.Left);
        }

        /**
         * Should the text alignement be center justified
         * @return {Boolean} true/false
         */
        function isCenter() {
            return !scope.isXSmall && (!scope.isRight && !scope.isLeft);
        }

        /**
         * Set the video container height to its aspect ratio height.
         * This allows us to span the video edge to edge
         *
         */
        function setVideoContainerHeight(){
            var width,
                aspectRatioHeight;
            if (!$videoContainer.length){
                $videoContainer = element.find(VIDEO_CONTAINER_SELECTOR);
            }

            if (!$videoElement.length) {
                $videoElement = element.find(VIDEO_IFRAME_SELECTOR);
            }

            if ($videoContainer.length && $videoElement.length) {
                width = $videoContainer.width();
                aspectRatioHeight = width * VIDEO_ASPECT_RATIO;
                $videoElement.height(aspectRatioHeight);
            }
        }

        /**
         * Recalculate all the UI elements that should be responsive.
         * @return {void} Nata
         */
        function adjustSize() {
            scope.isXSmall = ScreenFactory.isXSmall();
            scope.isRight = isRight();
            scope.isLeft = isLeft();
            scope.isCenter = isCenter();
            scope.$digest();
        }

        if (scope.videoId) {

            originStorePdpSectionWrapperCtrl.setVisibility(true);

            scope.ctaType = scope.ctaType || ButtonTypeEnumeration.none;
            scope.isExternalCta = (scope.ctaHref && UtilFactory.isAbsoluteUrl(scope.ctaHref));

            scope.isRight = isRight();
            scope.isLeft = isLeft();
            scope.isCenter = isCenter();
            scope.isDarkColor = (scope.textColor === TextColorsEnumeration.dark);

            if (!scope.isXSmall) {
                scope.isTextInView = ScreenFactory.isOnOrAboveScreen(element);
                scope.textSlide = BooleanEnumeration[scope.textSlide] === BooleanEnumeration.true;
                scope.tSlide = BooleanEnumeration[scope.textSlide] === BooleanEnumeration.true;
                AnimateFactory.addScrollEventHandler(scope, updateVisibility);
            }
            AnimateFactory.addScrollEventHandler(scope, adjustSize, SCROLL_RESIZE_THROTTLE_MS);
        }

        function checkAge(loginObj){
            var isLoggedIn = (loginObj && loginObj.isLoggedIn);
            scope.ageGateActive = isLoggedIn && StoreAgeGateFactory.isUserUnderAgeForMediaLocale();
        }

        function detachAuthEvents(){
            AuthFactory.events.off('myauth:change', checkAge);
        }
        scope.$on('$destroy', detachAuthEvents);

        AuthFactory.events.on('myauth:change', checkAge);
        AuthFactory.waitForAuthReady()
            .then(checkAge)
            .catch(angular.noop);
        requestAnimationFrame(setVideoContainerHeight);
        AnimateFactory.addResizeEventHandler(scope, setVideoContainerHeight, SCROLL_RESIZE_THROTTLE_MS);
    }


    function originStorePdpFeatureVideo(ComponentsConfigFactory, ScreenFactory, AnimateFactory, StoreAgeGateFactory, AuthFactory, UtilFactory) {

        function originStorePdpFeatureVideoLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            sharedLinkFunction(scope, element, attributes, originStorePdpSectionWrapperCtrl, ScreenFactory, AnimateFactory, StoreAgeGateFactory, AuthFactory, UtilFactory);
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {
                videoId: '@',
                smallScreenImageurl: '@',
                matureContent: '@',
                videoImage: '@',
                header: '@',
                text: '@',
                textAlign: '@',
                textColor: '@',
                textSlide: '@',
                ctaType: '@',
                ctaDescription: '@',
                ctaHref: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/video.html'),
            link: originStorePdpFeatureVideoLink
        };
    }

    function originStorePdpFeatureAccoladeVideo(ComponentsConfigFactory, ScreenFactory, AuthFactory, AnimateFactory, StoreAgeGateFactory, UtilFactory) {

        function originStorePdpFeatureVideoLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            scope.textAlignment = TextAlignEnumeration.Center;
            scope.textSlide = BooleanEnumeration.false;
            sharedLinkFunction(scope, element, attributes, originStorePdpSectionWrapperCtrl, ScreenFactory, AnimateFactory, StoreAgeGateFactory, AuthFactory, UtilFactory);
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {
                quote1Str: '@',
                quote1SourceStr: '@',
                quote2Str: '@',
                quote2SourceStr: '@',
                quote3Str: '@',
                quote3SourceStr: '@',
                videoId: '@',
                smallScreenImageurl: '@',
                matureContent: '@',
                videoImage: '@'
            },
            link: originStorePdpFeatureVideoLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/accoladevideo.html')
        };
    }

    function originStorePdpFeatureBadgeVideo(ComponentsConfigFactory, ScreenFactory, AuthFactory, AnimateFactory, StoreAgeGateFactory, UtilFactory) {

        function originStorePdpFeatureBadgeVideoLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            scope.textAlignment = TextAlignEnumeration.Center;
            scope.textSlide = BooleanEnumeration.false;
            sharedLinkFunction(scope, element, attributes, originStorePdpSectionWrapperCtrl, ScreenFactory, AnimateFactory, StoreAgeGateFactory, AuthFactory, UtilFactory);
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {
                quoteStr: '@',
                badgeImage: '@',
                videoId: '@',
                smallScreenImageurl: '@',
                matureContent: '@',
                videoImage: '@'
            },
            link: originStorePdpFeatureBadgeVideoLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/badgevideo.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureVideo
     * @restrict E
     * @element ANY
     * @scope
     * @param {Video} video-id youtube id of video
     * @param {ImageUrl} small-screen-imageurl image to replace background video on small devices (600px or less)
     * @param {TextColorsEnumeration} text-color text color: light or dark
     * @param {TextAlignEnumeration} text-align text alignment
     * @param {BooleanEnumeration} text-slide boolean to determine if text slides in or not
     * @param {LocalizedString} header (optional) header text
     * @param {LocalizedString} text (optional) main paragraph
     * @param {ButtonTypeEnumeration} cta-type type of CTA button
     * @param {LocalizedString} cta-description CTA description text
     * @param {Url} cta-href URL of CTA link
     * @param {ImageUrl} video-image will be displayed if video has mature content and user is under age
     * @param {BooleanEnumeration} mature-content if video has mature content
     *
     * @description
     *
     * Large feature video PDP section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-video
     *         video-id="3eVF9uBbuqc"
     *         small-screen-imageurl="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/store/pdp/sw_feature5.jpg"
     *         text-color="light"
     *         text-align="center"
     *         text-slide="true"
     *         header="Immerse yourself in your Star Wars Battlefront fantasies"
     *         text="Photorealistic visuals and authentic sound design from the talented team at DICE. Prepate to be transported to a galaxy far, far away."
     *         cta-type="transparent"
     *         cta-description="Watch the E3 Gameplay Trailer"
     *         cta-href="#"
     *     ></origin-store-pdp-feature-video>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureVideo', originStorePdpFeatureVideo)
        .directive('originStorePdpFeatureAccoladeVideo', originStorePdpFeatureAccoladeVideo)
        .directive('originStorePdpFeatureBadgeVideo', originStorePdpFeatureBadgeVideo);
}());
