/**
 *  * @file htmltransformer.js
 */
(function () {
    'use strict';

    var OTK_LINK_CLASS = 'otka';
    var EXTERNAL_LINK_CLASS = 'external-link';

    function HtmlTransformer(UtilFactory) {
        var globalReplacements = {
            '%url-locale%': Origin.locale.languageCode() + '-' + Origin.locale.countryCode().toLowerCase(),
            '%url-country%': Origin.locale.threeLetterCountryCode().toLowerCase(),
            '%url-country-code%': Origin.locale.countryCode().toLowerCase(),
            '%country-code%': Origin.locale.countryCode(),
            '%url-language%': Origin.locale.languageCode(),
            '%locale%': Origin.locale.locale()
        };

        function getHref(linkTag) {
            return  linkTag.getAttribute('href');
        }

        function setHref(linkTag, href) {
            return  linkTag.setAttribute('href', href);
        }


        /**
         * Add external-link class to any link that's external
         * @param linkTag
         * @param href
         */
        function addExternalLinkClass(linkTag, href) {
            if (linkTag.className.indexOf(EXTERNAL_LINK_CLASS) === -1 && UtilFactory.isAbsoluteUrl(href)){
                linkTag.classList.add(EXTERNAL_LINK_CLASS);
            }
        }

        /**
         * Replaces global tokens for any link in GEO merchandized html
         * @param linkTag a tag
         * @param href href
         */
        function replaceGlobalTokens(linkTag, href) {
            href = UtilFactory.replaceAll(href, globalReplacements);
            if (href !== getHref(linkTag)) {
                setHref(linkTag, href);
            }
        }

        /**
         * Processes link tag and sets appropriate class for client to open the link in external window
         * @param {HTMLElement} linkTag
         */
        function processLink(linkTag){
            var href = getHref(linkTag);
            if (href){
               addExternalLinkClass(linkTag, href);
               replaceGlobalTokens(linkTag, href);
            }
        }


        /**
         * Take HTML string, convert to DOM, add 'otka' class to link tags, return transformed HTML
         * as string, because OTK link styles only apply to OTK-specific class
         * @param htmlString
         * @returns {string}
         */
        function addOtkClasses(htmlString) {
            if (htmlString) {
                // only process this HTML if there are <a> tags to update, otherwise pass htmlString through
                if (htmlString.indexOf('<a ') > -1) {
                    var htmlContent = document.createElement('div');
                    htmlContent.innerHTML = htmlString;
                    var linkTags = htmlContent.getElementsByTagName('a');
                    var linkTag;
                    for (var i = 0, l=linkTags.length; i < l; i++) {
                        linkTag = linkTags[i];
                        if (!linkTag.className) {
                            linkTag.className = OTK_LINK_CLASS;
                        }
                        processLink(linkTag);
                    }
                    return htmlContent.innerHTML;
                } else {
                    return htmlString;
                }
            } else {
                return '&lt;!-- HtmlTransformer error --&gt;';
            }
        }


        return {
            addOtkClasses: addOtkClasses
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.HtmlTransformer
     * @description
     *
     * HtmlTransformer
     */
    angular.module('originApp')
        .factory('HtmlTransformer', HtmlTransformer);
}());
