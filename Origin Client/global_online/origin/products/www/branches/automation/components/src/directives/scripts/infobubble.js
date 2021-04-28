/**
 * @file /infobubble.js
 */
(function() {
    'use strict';

    function originInfobubble($compile, $timeout, UtilFactory, $window, AnimateFactory) {

        function originInfobubbleLink(scope, element, attrs) {

            var id, delayHideTimeout, hideTimeout, $container;
            var ignoreHide = false;
            var timeoutDelay = 100;
            var HALF_MODULE_WIDTH = 180;
            var INFOBUBBLE_HEIGHT_MINIMUM = 180;
            var cursorOnCta = false;

            function getContainer() {
                /* if it's not in the DOM, append it to the body */
                if ($('#infobubble-container').length === 0) {
                    $('body').append('<div id="infobubble-container"></div>');
                }

                return $('#infobubble-container');
            }

            function showInfoBubble(){
                // disable infobuble for touch and subscribers on access logo
                if(UtilFactory.isTouchEnabled() || (element.hasClass('origin-store-originaccess') && element.attr('hasinfobubble') !== 'true')) {
                    return false;
                }
                return true;
            }
            /**
             * Create, posistion and show the infobubble.
             * This will only toggle the 'isvisible' class if the DOM object exsists.
             * @return {void}
             * @method
             */
            function show() {
                if(!showInfoBubble()) {
                    return;
                }

                var top,
                    left,
                    height = element.outerHeight(),
                    width = element.outerWidth(),
                    offset = element.get(0).getBoundingClientRect(),
                    arrow = 0,
                    boxRight,
                    boxLeft,
                    oldLeft,
                    $infoBubble;

                /* Let your parents know your showing */
                scope.$emit('infobubble-show');

                /* ignore the 'hide' for a while.. */
                delayHide();

                $container = getContainer();

                /* If jQuery is unable to determine size of element, try native DOM function */
                if (height === 0 && width === 0) {
                    height = offset.height;
                    width = offset.width;
                }

                /* calculate the top/left of the new info bubble based offset */
                top = offset.top + height;
                left = offset.left + (width / 2);
                boxRight = left + HALF_MODULE_WIDTH;
                boxLeft = left - HALF_MODULE_WIDTH;

                /* reposition boxes to close to the right edge */
                if(boxRight > $window.innerWidth) {
                    oldLeft = left;
                    left = $window.innerWidth - HALF_MODULE_WIDTH;
                    arrow = oldLeft - left;

                    if(arrow > 120) {
                        arrow = 120;
                    }
                }

                /* reposition boxes to close to the left edge */
                if(boxLeft < 0) {
                    oldLeft = left;
                    left = 150;
                    arrow = oldLeft - left;
                }

                /* create a unique id we can use to idenitfy this instance of the infobubble */
                id = 'infobubble-' + top + '-' + left;

                /* if $container does not have the id, update it with a 'clone' of the infobubble */
                if ($container.data('id') !== id) {

                    /* add the unique id */
                    $container.data('id', id);

                    /* inject a compiled infobubble into $container */
                    $infoBubble = $compile(
                        '<div class="origin-infobubble">' +
                            '<div class="origin-infobubble-arrow origin-infobubble-arrow-up"></div>' +
                            '<div class="origin-infobubble-content otkc otkc-small">' +
                                attrs.originInfobubble +
                            '</div>' +
                        '</div>')(scope);

                    $container.empty().append($infoBubble);

                    /* bind to hover of the new infobubble */
                    $infoBubble.on('mouseenter', delayHide);
                    $infoBubble.on('mouseleave', onMouseLeave);
                }

                $infoBubble = $container.find(' > .origin-infobubble');

                /* position arrow */
                $container.find('.origin-infobubble-arrow').css('left', 'calc(50% + ' + arrow + 'px)');

                /* position $container to top/left */
                $container.css({top: top, left: left});

                // Determine if there is enough space below the element to display the infoBubble
                if ((top + INFOBUBBLE_HEIGHT_MINIMUM) > $(window).height() ) {
                    // If there is not enough space below the element, rotate and translate the infoBubble
                    $container.css({'transform': 'rotate(180deg) translateY('+height+'px)'});                    
                    $container.find('.origin-infobubble-content').addClass('origin-infobubble-content-flipped');
                } else {
                    $container.css({'transform': ''});                    
                    $container.find('.origin-infobubble-content').removeClass('origin-infobubble-content-flipped');
                }

                /* show the infobubble by adding the 'isvisible' class */
                $infoBubble.addClass('origin-infobubble-isvisible');
            }

            /**
             * Hides the infobubble by removing the 'isvisible' class
             * @return {void}
             * @method
             */
            function hide() {
                $container = $('#infobubble-container');

                /* confirm its ok to 'hide' the infobubble. */
                if (ignoreHide === false) {

                    /* Let your parents know your hiden */
                    scope.$emit('infobubble-hide');

                    /* confirm $container has our unique id. */
                    if ($container.data('id') === id && !cursorOnCta) {
                        $container
                            .css({top: 0, left: 0})
                            .find(' > .origin-infobubble')
                                .removeClass('origin-infobubble-isvisible');
                    }
               }
            }

            /**
             * Empties the html content of $container
             * @return {void}
             * @method
             */
            function remove() {
                if (typeof $container !== 'undefined') {
                    ignoreHide = false;
                    hide();
                    $container.empty();
                }
            }

            /**
             * Temporary disables the ability to 'hide' the infobubble
             * @return {void}
             * @method
             */
            function delayHide() {

                /* set to true */
                ignoreHide = true;

                /* cancel the promise, even the empty ones..DAD!! */
                $timeout.cancel(delayHideTimeout);

                /* set back to false after timeout */
                delayHideTimeout = $timeout(function(){
                    ignoreHide = false;
                }, timeoutDelay);
            }

            /**
             * Cancel the hideTimeout promise and show the bubble
             * @return {void}
             * @method
             */
            function onMouseEnter() {

                /* cancel the promise to hide */
                $timeout.cancel(hideTimeout);

                cursorOnCta = true;

                /* Show the bubble */
                show();
            }

            /**
             * Cancel the hideTimeout promise and hide the bubble after delay
             * @return {void}
             * @method
             */
            function onMouseLeave(e) {

                /* cancel the promise */
                $timeout.cancel(hideTimeout);

                if (element.is(e.currentTarget)){
                    cursorOnCta = false;
                }

                /* hide() after a delay */
                hideTimeout = $timeout(function() {
                    hide();
                }, timeoutDelay);
            }

            /**
             * Unbind and cancel timeouts on detroy
             * @return {void}
             * @method
             */
            function onDestroy() {
                /* hide element */
                hide();

                $container = getContainer();
                if ($container.data('id') === id) {
                    $container.data('id', null);
                }

                /* events */
                element.off();
                element.find(' > .origin-infobubble').off();

                /* timeouts */
                $timeout.cancel(hideTimeout);
                $timeout.cancel(delayHideTimeout);
            }

            /* bind to remove */
            scope.$on('infobubble-remove', remove);

            /* bind to mouseenter mouseleave */
            element.on('mouseenter', onMouseEnter);
            element.on('mouseleave', onMouseLeave);

            scope.$on('$destroy', onDestroy);

            AnimateFactory.addScrollEventHandler(scope, hide);
        }

        return {
            restrict: 'A',
            link: originInfobubbleLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originInfobubble
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * Info Bubble to show "More Info" about an item
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <div origin-infobubble="<h1>hello world!</h1>"></div>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originInfobubble', originInfobubble);
}());