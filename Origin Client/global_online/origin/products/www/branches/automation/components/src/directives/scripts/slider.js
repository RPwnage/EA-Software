
/**
 * @file scripts/slider.js
 */
(function() {
    'use strict';
    function originSlider(ComponentsConfigFactory) {

        function originSliderLink(scope, elem, attrs, ctrl) {

            var slider = {
                    track: elem.find('.otkslider-track'),
                    fill: elem.find('.otkslider-fill'),
                    thumb: elem.find('.otkslider-thumb'),
                    value: attrs.init || 0,
                    dragging: false,
                    trackWidth: elem.width(),
                    thumbWidth: elem.find('.otkslider-thumb').outerWidth(),
                    thumbXMax: elem.width() - elem.find('.otkslider-thumb').outerWidth()
            };

            /*
             * Fill the lower part of the track
             */
            function fill(value) {
                slider.fill.css('width', value + '%');
            }

            /*
             * Handle dragging
             */
            function drag(event) {
                if (event.button !== 0) {
                    stopDrag();
                    return;
                }

                // get dynamic info
                var trackX = parseInt(slider.track.offset().left),
                    mouseX = event.pageX,
                    thumbX = mouseX - trackX;

                // Keep it in range
                thumbX = thumbX-(slider.thumbWidth/2);
                if (thumbX < 0) {
                    thumbX = 0;
                }
                if (thumbX > slider.thumbXMax) {
                    thumbX = slider.thumbXMax;
                }

                slider.value = 100 * thumbX / slider.thumbXMax;
                fill(slider.value);
            }

            /*
             * Stop dragging
             */
            function stopDrag() {
                if (slider.dragging) {
                    slider.dragging = false;

                    // update ng-model
                    ctrl.$setViewValue(slider.value);

                }
                $('body').off('mousemove', drag);
                $('body').off('mouseup', stopDrag);
            }

            /*
             * Start dragging
             */
            function startDrag(event) {
                if (event.button !== 0) { return; }

                slider.dragging = true;

                // get dynamic info
                var trackX = parseInt(slider.track.offset().left),
                    mouseX = event.pageX,
                    thumbX = mouseX - trackX;

                // do not allow clicking outside of track
                if (thumbX < 0 || thumbX > slider.trackWidth) { return; }

                thumbX = thumbX-(slider.thumbWidth/2);
                if (thumbX < 0) {
                    thumbX = 0;
                }
                if (thumbX > slider.thumbXMax) {
                    thumbX = slider.thumbXMax;
                }

                slider.value = 100 * thumbX / slider.thumbXMax;
                fill(slider.value);

                // handle dragging of slider thumb
                $('body').on('mousemove', drag);

                // stop dragging the slider thumb on mouseup
                $('body').on('mouseup', stopDrag);
            }

            // initialize slider position
            fill(slider.value);

            // start dragging the slider thumb on mousedown
            angular.element(elem).on('mousedown', function(event) {
                if (event.button !== 0) { return; }

                event.preventDefault();

                startDrag(event);
            });

            // execute ng-change
            ctrl.$viewChangeListeners.push(function() {
                scope.$eval(attrs.ngChange);
            });

            //Watch to see if the model value changes post init.
            scope.$watch("model", function(value) {
                var fValue = parseFloat(slider.value);
                if(fValue !== value)
                {
                    slider.value = value;
                    fill(slider.value);
                }
            });
        }

        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/slider.html'),
            require: 'ngModel',
            scope: {
                model: '=ngModel'
            },
            link: originSliderLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSlider
     * @restrict E
     * @element ANY
     * @description
     * @param {init} initial value of slider (0-100)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-slider init="26"/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originSlider', originSlider);
}());
