/*jshint strict: false */
/*jshint unused: false */

define([
    'core/utils',
    'core/logger',
    'generated/jssdkconfig.js'
], function(utils, logger, jssdkconfig) {
    /**
     * helper functions for managing jssdk urls
     * @module module:urls
     * @memberof module:Origin
     * @private
     */

    return /** @lends module:Origin.module:urls */ {

        init: function() {
            utils.replaceTemplatedValuesInConfig(jssdkconfig);
            logger.info('JSSDK URLS', jssdkconfig.urls);
        },

        /**
         * endpoints used
         */
        endPoints: jssdkconfig.urls
    };
});