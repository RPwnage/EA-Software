/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define(['core/windows'], function(windows) {

    /**
     * Handle external links
     * @param {string} url the url to visit in the new window
     */
    function externalUrl(url) {
        if (Origin.client.isEmbeddedBrowser() && url && url.indexOf('http') !== -1) {
            Origin.client.desktopServices.asyncOpenUrl(url);
        } else if (Origin.client.isEmbeddedBrowser() && url) {
            var sanitizedUrl = window.location.protocol + '//' + window.document.domain + url;
            Origin.client.desktopServices.asyncOpenUrl(sanitizedUrl);
        } else {
            window.open(url);
        }
    }

    return /** @lends Origin.module:windows */ {
        /**
         * Handle external links
         * @method
         * @static
         * @param {string} url the url to visit in the new window
         */
        externalUrl: externalUrl
    };
});
