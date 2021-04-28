angular.module('app')
    .controller('BadgesCtrl', function () {

        $(document).on("click", "#otkbadge-notification-button", function () {
            var value, $badge = $('#otkbadge-notification-button').prev('.otkbadge');

            // increment it
            value = Number($badge.text());
            value++;
            $badge.text(value);

            // trigger the animation
            $badge.removeClass('otkbadge-isnotifying');
            setTimeout( function() {
                $badge.addClass('otkbadge-isnotifying');
            },0);
        });

    });
