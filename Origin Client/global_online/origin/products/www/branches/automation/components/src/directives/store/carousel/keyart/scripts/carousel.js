
/** 
 * @file store/carousel/keyart/scripts/carousel.js
 */ 
(function(){
    'use strict';

    var directionEnumeration = {
        "left": "left",
        "right": "right"
    };

    var SortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };

    var OrderByEnumeration = {
        "dynamicPrice": "dynamicPrice",
        "offerType": "offerType",
        "onSale": "onSale",
        "rank": "rank",
        "releaseDate": "releaseDate",
        "title": "title"
    };

    var typeEnumeration = {
        "merchandised": "merch",
        "solr": "solr"
    };

     /* jshint ignore:start */
    var shadeEnumeration = {
        "light": "light",
        "dark": "dark"
    };
    /* jshint ignore:end */

    var CAROUSEL_CLASS = 'origin-store-carousel-keyart-carousel';
    var SLIDES_CLASS = '.origin-store-carousel-keyart-slides';
    var SLIDE_CLASS = 'origin-store-carousel-keyart-slide';
    var SOLR_CLASS = '.origin-store-carousel-keyart-solr';
    var MERCH_CLASS = '.origin-store-carousel-keyart-merch';
    var KEYART_GAMETILE_SELECTOR = '.origin-store-game-keyart';
    var THROTTLE_DELAY_MS = 20;

    function originStoreCarouselKeyartCarousel(ComponentsConfigFactory, AnimateFactory, SearchFactory, ObjectHelperFactory, CSSUtilFactory, AppCommFactory) {

        function originStoreCarouselKeyartCarouselLink(scope, element) {
            var carousel,
                carouselMaxMovment = 0,
                slideCount = 0,
                slideItemWidth = 0,
                itemWidth = 0,
                userInteracted = false,
                firstItem;

            scope.totalWidth = 0;
            scope.pills = 0;
            scope.activePill = 0;

            // create a div with the slide class
            function createSlideElement(width) {
                var element = document.createElement('div');

                angular.element(element).addClass(SLIDE_CLASS);

                if(width){
                    angular.element(element).css('width', width);
                }

                return element;
            }

            function setChildValues(childElement) {
                // set carousel width variables
                scope.totalWidth += angular.element(childElement).outerWidth();
                scope.totalWidth += parseInt(angular.element(childElement).css('margin-left'), 10);
                scope.totalWidth += parseInt(angular.element(childElement).css('margin-right'), 10);

                // calculate the slides from the left if there is more/less than one keyart tile
                if(_.size(element.children().find(KEYART_GAMETILE_SELECTOR)) !== 1) {
                    // create slides to insert above the game cards and set their width,
                    // use these slides position().left to slide the carousel to the right position
                    // so we dont chop game tiles in half
                    itemWidth = angular.element(childElement).outerWidth() + 
                                parseInt(angular.element(childElement).css('margin-left'), 10) + 
                                parseInt(angular.element(childElement).css('margin-right'), 10);

                    // create first slide
                    if(slideCount === 0) {
                        slideCount++;
                        element.find(SLIDES_CLASS).html(createSlideElement());
                    }

                    slideItemWidth += itemWidth;

                    // game tile width does not excede maximum width, increase slide width
                    if(slideItemWidth <= element.outerWidth() || firstItem) {
                        firstItem = false;
                        angular.element(element.find('.' + SLIDE_CLASS)[slideCount-1]).css('width', slideItemWidth + 'px');
                    } else {
                        // game tile width excedes maximum width, add new slide and increase width
                        element.find(SLIDES_CLASS).append(createSlideElement());
                        slideItemWidth = itemWidth;
                        slideCount++;
                        angular.element(element.find('.' + SLIDE_CLASS)[slideCount-1]).css('width', slideItemWidth + 'px');
                    }
                }
            }

            // create slides that start from the center and add slides on either side
            function createSlidesFromCenter() {
                var carouselOuterWidth = element.outerWidth(),
                    remainingWidth = scope.totalWidth - carouselOuterWidth,
                    slideWidth,
                    slidesElement = element.find(SLIDES_CLASS);

                // create middle slide at full screen width
                slidesElement.html(createSlideElement(carouselOuterWidth));

                // create flanking slides if space still remains
                if(remainingWidth > 0 && carouselOuterWidth) {
                    // calculate number of flanking slides
                    var cells = Math.ceil(Math.ceil(remainingWidth/carouselOuterWidth)/2);

                    for(var i = 0; i < cells; i++) {
                        // calculate flanking slides width
                        slideWidth = remainingWidth/2 > carouselOuterWidth ? carouselOuterWidth : remainingWidth/2;

                        // create flanking slides and insert into dom
                        slidesElement.append(createSlideElement(slideWidth)).prepend(createSlideElement(slideWidth));

                        // update remaining space after creating flanking slides
                        remainingWidth = (remainingWidth - carouselOuterWidth*2);
                    }
                }

                slideCount = _.size(slidesElement.children());
            }

            function centerKeyart() {
                // calculate the center of the carousel
                var offset = -Math.abs((scope.totalWidth - element.outerWidth())/2);

                // select the middle pill
                scope.activePill = Math.floor(scope.pills/2);

                // move carousel to center tile
                moveCarousel(offset);
            }

            // check if there is an odd number of pills
            function hasMiddlePill() {
                return scope.pills%2 !== 0;
            }

            // is there a keyart tile in the carousel
            function hasKeyartGameTile() {
                return _.size(element.children().find(KEYART_GAMETILE_SELECTOR)) > 0;
            }

            // check if the middle pill is the currently selected pill
            function isMiddlePillSelected() {
                return scope.activePill === Math.floor(scope.pills/2);
            }

            function setupCarousel() {
                carousel = element.find('.' + CAROUSEL_CLASS);

                var items = carousel.children(),
                    carouselWidth = element.outerWidth();

                // reset to 0
                slideCount = 0;
                slideItemWidth = 0;
                itemWidth = 0;
                firstItem = true;

                scope.totalWidth = 0;

                _.forEach(items, setChildValues);

                // calculate the slides from the center if there is one keyart tile
                if(_.size(element.children().find(KEYART_GAMETILE_SELECTOR)) === 1) {
                    createSlidesFromCenter();
                }

                carouselMaxMovment = scope.totalWidth > carouselWidth ? carouselWidth - scope.totalWidth : 0;
                carousel.css('width', scope.totalWidth + 'px');
                element.find(SLIDES_CLASS).css('width', scope.totalWidth + 'px');

                // set number of pills to match number of slides
                scope.pills = slideCount;

                if((!userInteracted || isMiddlePillSelected()) && hasMiddlePill() && hasKeyartGameTile()) {
                    // center carousel if it has at leaset 1 keyart tile and the user has not interacted with it yet
                    centerKeyart();
                } else {
                    // jump to new slide if currently selected slide does not exist anymore (re-sizing screen)
                    var recalculatedSlide = scope.activePill >= slideCount ? slideCount - 1 : scope.activePill;
                    scope.jumpSlide(recalculatedSlide, false);
                }
            }

            // move the carousel translateX value
            function moveCarousel(newTranslateValue) {
                if(newTranslateValue > 0) {
                    newTranslateValue = 0;
                } else if(newTranslateValue < carouselMaxMovment) {
                    newTranslateValue = carouselMaxMovment;
                }

                carousel.css(CSSUtilFactory.addVendorPrefixes('transform', 'translateX(' + newTranslateValue + 'px)'));
            }

            // slide the carousel to the next/previous or first/last slide
            scope.slideCarousel = function(direction) {
                if(direction === directionEnumeration.left) {
                    if(scope.activePill === 0) {
                        scope.activePill = (scope.pills - 1);
                    } else if(scope.activePill === Math.floor(scope.pills/2) + 1 && hasMiddlePill() && hasKeyartGameTile()) {
                        //center the carousel if there is a middle pill and keyart tile
                        centerKeyart();
                        return;
                    } else {
                        scope.activePill--;
                    }
                } else if(direction === directionEnumeration.right) {
                    if(scope.activePill === (scope.pills - 1)) {
                        scope.activePill = 0;
                    } else if(scope.activePill === Math.floor(scope.pills/2) - 1 && hasMiddlePill() && hasKeyartGameTile()) {
                        //center the carousel if there is a middle pill and keyart tile
                        centerKeyart();
                        return;
                    } else {
                        scope.activePill++;
                    }
                }

                scope.jumpSlide(scope.activePill, true);
            };

            // create array the length of pills to ng-repeat
            scope.getPills = function(num) {
                return new Array(num);
            };

            // jump to a slide on pill click
            scope.jumpSlide = function($index, userInteraction) {
                var slide = $(element.find('.' + SLIDE_CLASS)[$index]);
                if(_.size(slide) > 0) {
                    var offset = -Math.abs(slide.position().left);
                    moveCarousel(offset);
                    scope.activePill = $index;
                }

                if(userInteraction) {
                    userInteracted = true;
                }
            };

            // recalculate slides after screen resize
            AnimateFactory.addResizeEventHandler(scope, setupCarousel, THROTTLE_DELAY_MS);

            // set ocdPaths scope value for ng-repeat in view for solr carousel
            function getOcdPaths(offers) {
                if (offers) {
                   scope.ocdPaths = offers;
                } else {
                    scope.ocdPaths = [];
                }
            }

            // set no games on solr search error
            function errorHandler() {
                scope.ocdPaths = [];
            }

            // if this is a hand merchandised carousel, remove the solr carousel div
            // then cacluate the carousel data after the dom has loaded
            if(scope.type === typeEnumeration.merchandised) {
                element
                    .find(SOLR_CLASS)
                    .remove();

                setTimeout(setupCarousel, 0);
            } else {
                // remove carousel class from hand merchandised div
                // cannot remove element with ng-transclude on it, or angular complains
                element
                    .find(MERCH_CLASS)
                    .removeClass(CAROUSEL_CLASS)
                    .empty();

                // set up carousel values when search completes
                scope.$watchOnce('ocdPaths', function(newValue, oldValue) {
                    if(newValue !== oldValue && !_.isEmpty(newValue)) {
                        setupCarousel();
                    }
                });

                // set search-parameter defaults
                scope.query = scope.query || '';
                scope.facet = scope.facet || '';
                scope.sort = OrderByEnumeration[scope.sort] || OrderByEnumeration.rank;
                scope.dir = scope.dir || SortDirectionEnumeration.ascending;
                scope.limit = scope.limit || 30;
                scope.offset = scope.offset || 0;

                var searchParams = {
                    filterQuery: scope.facet,
                    sort: scope.sort + ' ' + scope.dir,
                    start: scope.offset,
                    rows: scope.limit
                };

                // search for games
                SearchFactory.searchStore(scope.query, searchParams)
                    .then(ObjectHelperFactory.getProperty(['games', 'game']))
                    .then(getOcdPaths)
                    .catch(errorHandler);
            }

            // the zoom of page transition messes up the measurements of page width,
            // need to recalculate after transition is complete
            AppCommFactory.events.on('uiRouter:stateChangeAnimationComplete', setupCarousel);

            function onDestroy() {
                AppCommFactory.events.off('uiRouter:stateChangeAnimationComplete', setupCarousel);
            }

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                type: '@',
                limit: '@',
                offset: '@',
                sort: '@',
                dir: '@',
                facet: '@',
                query: '@',
                shade: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/keyart/views/carousel.html'),
            link: originStoreCarouselKeyartCarouselLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselKeyartCarousel
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {typeEnumeration} type The type of carousel
     * @param {Number} limit the offer limit
     * @param {Number} offset the search offset
     * @param {OrderByEnumeration} sort the search sort by value
     * @param {SortDirectionEnumeration} dir the search sorting direction
     * @param {String} facet the search facets
     * @param {String} query the search query string
     * @param {shadeEnumeration} shade the text shade
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-keyart />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselKeyartCarousel', originStoreCarouselKeyartCarousel);
}()); 
