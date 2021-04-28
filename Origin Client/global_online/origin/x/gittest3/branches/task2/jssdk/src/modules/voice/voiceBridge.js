/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
], function () {

    /** @namespace
     * @name voice
     * @memberof Origin
     */
    return {

        /**
         * @method
         * @memberof Origin.voice
         * @returns {boolean}
         */
        supported: function () {
            return OriginVoice.supported;
        }
    };
});
