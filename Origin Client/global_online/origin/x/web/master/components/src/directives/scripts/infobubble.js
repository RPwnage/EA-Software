/**
 * @file /infobubble.js
 */

(function() {
    'use strict';

    function originInfobubble($compile, $timeout, UtilFactory) {

        function originInfobubbleLink(scope, element, attrs) {

            var id, delayHideTimeout, hideTimeout, $container;
            var ignoreHide = false;
            var timeoutDelay = 100;

            /**
             * Create, posistion and show the infobubble.
             * This will only toggle the 'isvisible' class if the DOM object exsists.
             * @return {void}
             * @method
             */
            function show() {
                if(UtilFactory.isTouchEnabled()) {
                    return;
                }

                var top, left, height, width, offset;

                /* Let your parents know your showing */
                scope.$emit('infobubble-show');

                /* ignore the 'hide' for a while.. */
                delayHide();

                /* if it's not in the DOM, append it to the body */
                if ($('#infobubble-container').length === 0) {
                    $('body').append('<div id="infobubble-container"></div>');
                }

                $container = $('#infobubble-container');

                offset = element.offset();
                height = element.outerHeight();
                width = element.outerWidth();

                /* calculate the top/left of the new info bubble based offset */
                top = offset.top + height;
                left = offset.left + (width / 2);

                /* create a unique id we can use to idenitfy this instance of the infobubble */
                id = "infobubble-" + top + "-" + left;

                /* if $container does not have the id, update it with a 'clone' of the infobubble */
                if ($container.data('id') !== id) {

                    /* add the unique id */
                    $container.data('id', id);

                    /* position $container to top/left */
                    $container.offset({ top: top, left: left });

                    /* inject a compiled infobubble into $container */
                    var content = $compile(
                        '<div class="origin-infobubble">' +
                            '<div class="origin-infobubble-arrow origin-infobubble-arrow-up"></div>' +
                            '<div class="origin-infobubble-content otkc otkc-small">' +
                                attrs.originInfobubble +
                            '</div>' +
                        '</div>')(scope);

                    $container.empty().append(content);

                    /* bind to hover of the new infobubble */
                    $container.find(' > .origin-infobubble').on('mouseenter', delayHide);
                    $container.find(' > .origin-infobubble').on('mouseleave', onMouseLeave);
                }

                /* show the infobubble by adding the 'isvisible' class */
                $container.find(' > .origin-infobubble').addClass('origin-infobubble-isvisible');
            }

            /**
             * Hides the infobubble by removing the 'isvisible' class
             * @return {void}
             * @method
             */
            function hide() {

                /* confirm its ok to 'hide' the infobubble. */
                if (ignoreHide === false) {

                    /* Let your parents know your hiden */
                    scope.$emit('infobubble-hide');

                    /* confirm $container has our unique id. */
                    if ($container.data('id') === id) {
                        $container.find(' > .origin-infobubble').removeClass('origin-infobubble-isvisible');
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

            /* bind to remove */
            scope.$on('infobubble-remove', remove);

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

                /* Show the bubble */
                show();
            }

            /* bind to mouseenter */
            element.on('mouseenter', onMouseEnter);

            /**
             * Cancel the hideTimeout promise and hide the bubble after delay
             * @return {void}
             * @method
             */
            function onMouseLeave() {

                /* cancel the promise */
                $timeout.cancel(hideTimeout);

                /* hide() after a delay */
                hideTimeout = $timeout(function() {
                    hide();
                }, timeoutDelay);
            }

            /* bind to mouseleave */
            element.on('mouseleave', onMouseLeave);

            /**
             * Unbind and cancel timeouts on detroy
             * @return {void}
             * @method
             */
            function onDestroy() {

                /* events */
                element.off();
                element.find(' > .origin-infobubble').off();

                /* timeouts */
                $timeout.cancel(hideTimeout);
                $timeout.cancel(delayHideTimeout);
            }

            scope.$on('$destroy', onDestroy);
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