/*jshint strict: false */
/*jshint unused: false */
define([
], function () {

    function init() {}

    return {
        init: init,

        supported: function () {
            /* voice not yet supported on web */
            return false;
        },

        isSupportedBy: function() {
            return Promise.resolve(false);
        }
    };
});
