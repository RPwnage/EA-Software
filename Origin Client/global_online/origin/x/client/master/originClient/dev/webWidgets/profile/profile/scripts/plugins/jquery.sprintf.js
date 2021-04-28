;(function ($) {

    $.extend({
        sprintf : function (string) {
            var args = arguments,
                pattern = new RegExp("%s", "g"),
                counter = 1;
            return string.replace(pattern, function (match, index) {
                return args[counter++];
            });
        }
    });
})(jQuery);
