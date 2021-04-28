/**
 * @file store/scripts/carousel/hero/scripts/carousel.js
 */
(function(){
    'use strict';

    // Enumerations
    /**
     * Whether auto-rotation should be on or off
     * @readonly
     * @enum {string}
     */
    var autoRotateEnumeration = {
        "true": true,
        "false": false
    };

    /* jshint ignore:start */
    /**
     * How the carousel text should be aligned
     * @readonly
     * @enum {string}
     */
    var alignmentEnumeration = {
        "right": "carousel-caption-right",
        "center": "carousel-caption-center",
        "left": "carousel-caption-left"
    };
    /* jshint ignore:end */

    /**
     * Show or Hide The social media icons
     * @readonly
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    // Class Constants describing slides positions and animations
    var CAROUSEL_CLASSES = {
        'next': 'carousel-item-next',
        'previous': 'carousel-item-prev',
        'current': 'carousel-item-active',
        'left': 'carousel-item-left',
        'right': 'carousel-item-right'
    };

    /**
    * The controller
    */
    function OriginStoreCarouselHeroCarouselCtrl($scope, $interval, UtilFactory) {
        $scope.carouselState = {};
        $scope.carouselState.current = 0;
        $scope.carouselState.slides = undefined;
        $scope.carouselState.animationRunning = false;
        $scope.carouselState.focus = false;
        $scope.carouselState.autoRotate = true;
        $scope.carouselState.stationaryPeriod = 5000;

        $scope.stopAutoRotate = function() {
            $interval.cancel($scope._autoRotate);
        };

        $scope.autoRotateOn = function() {
            $scope.stopAutoRotate();
            if($scope.carouselState.slides.length > 1 && $scope.carouselState.autoRotate) {
                $scope._autoRotate = $interval(function(){
                        $scope.goTo($scope.carouselState.current + 1);
                }, $scope.carouselState.stationaryPeriod);
            }
        };


        /**
         * Determine which direction to animate
         * @param  {int}    startIndex     The index of the current slide
         * @param  {int}    endIndex       The index of the next slide
         * @param  {int}    numberOfSlides The number of slides that we have
         * @return {char}                  Either 'l' for left or 'r' for right
         */
        function determineDirection(startIndex, endIndex, numberOfSlides) {
            var direction = (startIndex < endIndex)? 'l': 'r';
            if (endIndex > numberOfSlides) {
                direction = 'r';
            }
            return direction;
        }
        /**
         * Animate from one slide to another
         * @param  {object} $scope    The scope object
         * @param  {int} index        Id of the slide
         * @param  {string} placement The position of the new slide. IE coming from left then position is left
         * @param  {string} direction The animation direction. IE animating from left to right direct is right
         * @return {void}
         */
        function animate($scope, index, placement, direction){
           $scope.carouselState.slides.eq(index).addClass(placement);
            UtilFactory.onTransitionEnd($scope.carouselState.slides.eq($scope.carouselState.current), $scope.postSlide);
            setTimeout(function() {
                $scope.carouselState.slides.eq(index).addClass(direction);
                $scope.carouselState.slides.eq($scope.carouselState.current).addClass(direction);
                $scope.carouselState.current = index; // Update the scope so the pills respond
                $scope.$digest();
            }, 25);
        }

        /**
         * Manage classes after a slide animation completes
         * @return {void}
         */
        $scope.postSlide = function() {
            $scope.carouselState.animationRunning = false;
            // Remove all the classes that cause animations
            $scope.carouselState.slides.each(function(index, element){
                var removeClasses = [
                    CAROUSEL_CLASSES.next,
                    CAROUSEL_CLASSES.previous,
                    CAROUSEL_CLASSES.left,
                    CAROUSEL_CLASSES.right,
                    CAROUSEL_CLASSES.current
                ].join(' ');
                $(element).removeClass(removeClasses);
            });
            // Add the active class back on
            $scope.carouselState.slides.eq($scope.carouselState.current).addClass(CAROUSEL_CLASSES.current);
            $scope.autoRotateOn();
        };

        /**
         * Change to a slide
         * @param  {int} index the id of the slide we wish to move to.
         * @return {void}
         */
        $scope.goTo = function(index) {
            if(index === undefined || index === $scope.carouselState.current || $scope.carouselState.slides.length === 1 || $scope.carouselState.animationRunning) {
                return;
            }

            var direction = determineDirection($scope.carouselState.current, index, $scope.carouselState.slides.length);

            if(index < 0) {
                index = $scope.carouselState.slides.length - 1;
            }
            if(index >= $scope.carouselState.slides.length) {
                index = 0;
            }
            // start the animation
            $scope.carouselState.animationRunning = true;
            // cancel the autoRotate
            if($scope._autoRotate) {
                $interval.cancel($scope._autoRotate);
            }
            var placement;
            if(direction === 'l') {
                // Show in the next postion
                placement = CAROUSEL_CLASSES.next;
                direction = CAROUSEL_CLASSES.left;
            } else {
                placement = CAROUSEL_CLASSES.previous;
                direction = CAROUSEL_CLASSES.right;
            }
            animate($scope, index, placement, direction);
        };

        this.goToNext = function() {
            $scope.goTo($scope.carouselState.current + 1);
        };

        this.goToPrev = function() {
            $scope.goTo($scope.carouselState.current - 1);
        };
        
        $scope.$on('$destroy', $scope.stopAutoRotate);
    }

    /**
    * The directive
    */
    function originStoreCarouselHeroCarousel(UtilFactory, $compile, $timeout) {
        /**
        * The directive link
        */
        function originStoreCarouselHeroCarouselLink(scope, elem, attrs, ctrl) {


            function hoverIn() {
                scope.carouselState.focus = true;
                scope.stopAutoRotate();
                scope.$digest();
            }

            function hoverOut() {
                scope.carouselState.focus = false;
                scope.autoRotateOn();
                scope.$digest();
            }

            /**
             * Called after a timer expired event this function ensures that we are on the correct slide when making the other slides visible.
             * @return {[type]} [description]
             */
            function timerExpired() {
                scope.carouselState.slides = scope.carouselState.hiddenSlides;
                // Set the current slide to be the timer slide
                scope.carouselState.hiddenSlides.each(function(index, element){
                    if($(element).attr('goal-date')) {
                        scope.carouselState.current = index;
                    }
                });
                scope.carouselState.timers = [];
                setup(scope);
            }

            /**
             * This function runs to create the carousel. This function needs to be only run once at the initial load of the
             * carousel
             *
             * @return {void}
             */
            function initialize() {
                // This will need to be done in an iterval to make sure that you find the correct slides
                var container = elem.find('.origin-store-carousel-container'),
                    slides = container.children().addClass('origin-store-carousel-content');
                scope.carouselState.timers = slides.filter('[goal-date]');
                scope.carouselState.hiddenSlides = slides;
                scope.carouselState.timers.each(function(index, element) {
                    var date = $(element).attr('goal-date');
                    scope.carouselState.timerEnd = (new Date(date)).getTime() - (new Date()).getTime();
                    $timeout(timerExpired, scope.carouselState.timerEnd);
                });
                // Move configuration parameters into the carousel state object
                scope.carouselState.autoRotate = autoRotateEnumeration[scope.autoRotate] !== undefined ? autoRotateEnumeration[scope.autoRotate] : true;
                delete scope.autoRotate;
                scope.carouselState.stationaryPeriod = scope.stationaryPeriod !== undefined ? scope.stationaryPeriod : scope.carouselState.stationaryPeriod;
                delete scope.stationaryPeriod;

                addControls();
                if (scope.showSocialMedia === BooleanEnumeration.true) {
                    addSocialMedia();
                }
                setup();
            }

            /**
             * Insert the pills and the left and right navigation for the carousel
             */
            function addControls() {
                var indicator = $compile('<origin-store-carousel-hero-indicators carousel-state="carouselState"></origin-store-carousel-hero-indicators>')(scope);
                var controls = $compile('<origin-store-carousel-hero-controls carousel-state="carouselState"></origin-store-carousel-hero-controls>')(scope);

                elem.append(indicator);
                elem.append(controls);
            }

            /**
             * Insert social media icons into the carousel
             */
            function addSocialMedia() {
                scope.isFacebookHidden = scope.hideFacebook === BooleanEnumeration.true;
                var socialMedia = $compile('<origin-socialmedia theme="lightWithBackground" title-str="{{::socialMediaTitle}}" description="{{::socialMediaDescription}}" hide-facebook="{{::isFacebookHidden}}" image="{{::socialMediaImage}}"></origin-socialmedia>')(scope);
                elem.append(socialMedia);
            }

            /**
             * A function to setup the carousel after major changes have been made to it's state.  This function can be run multiple
             * times to set the carousel in the correct state after major events ie number of slides change after a timer expires.
             *
             * @return {void}
             */
            function setup() {
                if(scope.carouselState.timers.length <= 0) { // No Timers
                    scope.carouselState.slides = scope.carouselState.hiddenSlides;
                } else { // has timers
                    scope.carouselState.slides = scope.carouselState.timers;
                }

                // Turn on the first slide.
                scope.carouselState.slides.eq(scope.carouselState.current).addClass('carousel-item-active');

                // Setup Events
                scope.$on('goTo', function(event, data){
                    event.stopPropagation();
                    scope.goTo(data.slideNumber);
                });

                scope.autoRotateOn();

                // Bind hover events - only on non-touch devices
                if (UtilFactory.isTouchEnabled()) {
                    scope.carouselState.focus = true;
                    elem.on('swipeleft', ctrl.goToNext);
                    elem.on('swiperight', ctrl.goToPrev);
                } else {
                    elem.hover(hoverIn, hoverOut);
                }
            }

            setTimeout(initialize, 0);
        }
        return {
            restrict: 'A',
            scope: {
                    alignment: '@',
                    stationaryPeriod: '@',
                    autoRotate : '@',
                    socialMediaTitle: '@',
                    socialMediaDescription: '@',
                    hideFacebook: '@',
                    socialMediaImage: '@',
                    showSocialMedia : '@showsocialmedia'
            },
            controller: 'OriginStoreCarouselHeroCarouselCtrl',
            link: originStoreCarouselHeroCarouselLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselHeroCarousel
     * @restrict A
     * @scope
     * @description
     *
     * @param {alignmentEnumeration} alignment - How the carousel text should be aligned affects every slide
     * @param {number} stationary-period - The number of milliseconds to sit on a slide without changing to the next slide
     * @param {autoRotateEnumeration} auto-rotate - boolean as to if auto rotation should be on or off.
     * @param {BooleanEnumeration} showsocialmedia - If social media icons should be shown.
     * @param {BooleanEnumeration} hide-facebook - hide facebook share button.
     * @param {LocalizedString} social-media-description social media description for facebook
     * @param {LocalizedString} social-media-title social media title for facebook
     * @param {ImageUrl} social-media-image social media image for facebook
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-hero-carousel />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreCarouselHeroCarouselCtrl', OriginStoreCarouselHeroCarouselCtrl)
        .directive('originStoreCarouselHeroCarousel', originStoreCarouselHeroCarousel);
}());
