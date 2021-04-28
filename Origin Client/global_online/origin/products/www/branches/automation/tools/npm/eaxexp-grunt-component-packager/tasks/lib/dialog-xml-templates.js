/**
 * A collection of the JSON rperesented xml partials used in CQ/AEM dialog generation
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

"use strict";

module.exports = function() {

    if (!String.prototype.startsWith) {
        String.prototype.startsWith = function(searchString, position) {
          position = position || 0;
          return this.substr(position, searchString.length) === searchString;
      };
    }

    function formatFieldLabel(name, description, isTmeplate) {
        if (description.startsWith("*")) {
            return "<span style='color:red;'>" + name.replace(/^\*/,"") + "</span>";
        } else if (isTmeplate) {
            return "<span style='color:green;'>" + name.replace(/^\*/,"") + "</span>";
        } else {
            return name;
        }
    }

    function formatFieldDescription(description) {
        return description.replace(/^\*/,"");
    }

    /**
     * Create the dialog boilerplate
     *
     * @param {string} directiveName - the name of the directive
     * @return {Object} a javascript representation of the xml file
     */
    function dialogContainer(directiveName) {
        return {
            "jcr:root": {
                "@xmlns:cq": "http://www.day.com/jcr/cq/1.0",
                "@xmlns:jcr": "http://www.jcp.org/jcr/1.0",
                "@xmlns:nt": "http://www.jcp.org/jcr/nt/1.0",
                "@jcr:primaryType": "cq:Dialog",
                "@Title": directiveName,
                "@xtype": "dialog",
                "items": {
                    "@jcr:primaryType": "cq:Widget",
                    "@xtype": "tabpanel",
                    "items": {
                        "@jcr:primaryType": "cq:WidgetCollection",
                        "general": {
                            "@jcr:primaryType": "cq:Panel",
                            "@title": "General",
                            "items":{
                                "@jcr:primaryType": "cq:WidgetCollection"
                            }
                        }
                    }
                }
            }
        };
    }

    /**
     * Create a string entry
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function string(parameter) {
        var crxLocation = "./" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "textfield"
        };
    }

    /**
     * Create a url resource (generates a path field)
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function url(parameter) {
        var crxLocation = "./" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "pathfield"
        };
    }

    /**
     * Create a localized string entry of a short length (generates an input box)
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function localizedString(parameter) {
        var crxLocation = "./i18n/" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "textfield"
        };
    }

     /**
     * Create a localized teplate string entry of a short length (generates an input box)
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function localizedTemplateString(parameter) {
        var crxLocation = "./i18n/" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description, true),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "textfield"
        };
    }

    /**
     * Create a multiline text field (generates a text area xtype)
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function localizedText(parameter) {
        var crxLocation = "./i18n/" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "textarea"
        };
    }

    /**
     * Create a multiline template %temlate string% text field (generates a text area xtype)
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function localizedTemplateText(parameter) {
        var crxLocation = "./i18n/" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description, true),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "textarea"
        };
    }

    /**
     * Create a number entry
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function number(parameter) {
        var crxLocation = "./" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "textfield"
        };
    }

    /**
     * Create a string-encoded boolean entry
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function boolean(parameter) {
        var crxLocation = "./" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@type": "checkbox",
            "@xtype": "selection"
        };
    }

    /**
     * Create an enumeration entry
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @param {Object} options - a hashmap of options
     * @return {Object} a javascript representation of the XML
     */
    function enumeration(parameter, options) {
        var crxLocation = "./" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@type": "select",
            "@xtype": "selection",
            "options": {
                "@jcr:primaryType": "cq:WidgetCollection",
                "#list": Object.keys(options).map(function(key, index) {
                    var value = options[key];
                    var nodeName = "o" + index;
                    var obj = {};
                    obj[nodeName] = mapOption(key, value);

                    return obj;
                })
            }
        };
    }

    /**
     * Map the options into selection xml
     *
     * @param {string} key - the left side of the option
     * @param {string} value - the right side of the option
     * @return {Object} a javascript representation of the XML
     */
    function mapOption(key, value) {
        if (typeof(value) !== 'string') {
            value = key;
        }

        return {
            "@jcr:primaryType": "nt:unstructured",
            "@text": key,
            "@value": value
        };
    }

    /**
     * Create a date picker entry
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function dateTime(parameter) {
        var crxLocation = "./" + parameter.name;

        return {
            "@fieldDescription": formatFieldDescription(parameter.description),
            "@fieldLabel": formatFieldLabel(parameter.name, parameter.description),
            "@jcr:primaryType": "cq:Widget",
            "@name": crxLocation,
            "@referencedValue": getDefaultValue(parameter),
            "@xtype": "datetime",
            "@dateFormat": "Y-m-d"
        };
    }

    /**
     * Create an image entry
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function imageTab(parameter) {
        var crxLocation = "./" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Panel",
            "@title": parameter.name + ": Image management",
            "items": {
                "@jcr:primaryType": "cq:WidgetCollection",
                "image_selector": {
                    "@jcr:primaryType": "cq:Widget",
                    "@collapsible": "{Boolean}true",
                    "@title": "Image Management: Select images and their targeted locales",
                    "@xtype": "dialogfieldset",
                    "items": {
                        "@jcr:primaryType": "cq:WidgetCollection",
                        "images": {
                            "@jcr:primaryType": "cq:Widget",
                            "@fieldDescription": "Click the '+' to add a new image for geo-targeting",
                            "@fieldLabel": "Images",
                            "@name": crxLocation,
                            "@xtype": "multifield",
                            "fieldConfig": {
                                "@jcr:primaryType": "cq:Widget",
                                "@path": "/apps/originx/components/management/imagetab/image_target/items/image_selector/items/images/fieldConfig.infinity.json",
                                "@xtype": "cqinclude"
                            }
                        }
                    }
                }
            }
        };
    }

    /**
     * Create a video picker tab
     *
     * @return {Object} a javascript representation of the XML
     */
    function videoTab() {
        return {
            "@jcr:primaryType": "cq:Widget",
            "@path": "/apps/originx/components/management/videotab/video_target.infinity.json",
            "@xtype": "cqinclude"
        };
    }

    /**
    * Create a link management tab
    * @return {Object} a javascript representation of the XML
    */

    function linkTab() {
        return {
            "@jcr:primaryType": "cq:Widget",
            "@path": "/apps/originx/components/management/linktab/link_target.infinity.json",
            "@xtype": "cqinclude"
        };
    }

    /**
     * Create an offer id picker tab
     *
     * @return {Object} a javascript representation of the XML
     */
    function offerTab() {
        return {
            "@jcr:primaryType": "cq:Widget",
            "@path": "/apps/originx/components/management/offerselectiontab/offer_management.infinity.json",
            "@xtype": "cqinclude"
        };
    }

    /**
     * Create an geo targeting tab entry
     *
     * @return {Object} a javascript representation of the XML
     */
    function geoTargetingTab() {
        return {
            "@jcr:primaryType": "cq:Widget",
            "@path": "/apps/originx/components/management/contenttargettab/content_target.infinity.json",
            "@xtype": "cqinclude"
        };
    }

    /**
     * Create a content reference tab entry
     *
     * @return {Object} a javascript representation of the XML
     */
    function contentReferenceTab() {
        return {
            "@jcr:primaryType": "cq:Widget",
            "@path": "/apps/originx/components/management/referencetargettab/reference_target.infinity.json",
            "@xtype": "cqinclude"
        };
    }

    /**
    * create a facetopiton tab used for big sale
    * @return {Object} a javascript representation of the XML
    **/
    function facetOptionTab() {
        return {
            "@jcr:primaryType": "cq:Widget",
            "@path": "/apps/originx/components/management/facetoptiontab/facetoption.infinity.json",
            "@xtype": "cqinclude"
        };
    }


    /**
     * Create an offer targeting tab
     *
     * @return {Object} a javascript representation of the XML
     */
    function offerTargetingTab() {
        return {
            "@jcr:primaryType": "cq:Widget",
            "@path": "/apps/originx/components/management/offertargettab/offer_target.infinity.json",
            "@xtype": "cqinclude"
        };
    }

    /**
     * Add an Xpath query to have the dialog reference a default value under catalog
     *
     * @param {Object} parameter the parameter to analyze
     */
    function getDefaultValue(parameter) {
        if (parameter.types && parameter.types.indexOf('OCD') > -1) {
            return '/apps/originx/components/imported/directives/ocd/ocd@' + parameter.name;
        } else {
            return '';
        }
    }

    /**
     * Create a OCD path ,the path to the OCD game content)
     *
     * @param {Object} parameter - a parameter from the source-extractor
     * @return {Object} a javascript representation of the XML
     */
    function ocdPath(parameter) {
        var crxLocation = "./" + parameter.name;

        return {
            "@jcr:primaryType": "cq:Widget",
            "@fieldDescription": parameter.description,
            "@fieldLabel": "Product",
            "@name": crxLocation,
            "@rootPath": "/content/web/app/games",
            "@xtype": "ocdpathfield",
            // jscs:disable
            "@validator" : 'function(){return Origin.Util.ocdPathValidator(this);}'
            // jscs:enable
        };
    }

    return {
        dialogContainer: dialogContainer,
        string: string,
        url: url,
        localizedString: localizedString,
        localizedText: localizedText,
        number: number,
        boolean: boolean,
        enumeration: enumeration,
        dateTime: dateTime,
        imageTab: imageTab,
        videoTab: videoTab,
        offerTab: offerTab,
        geoTargetingTab: geoTargetingTab,
        contentReferenceTab: contentReferenceTab,
        offerTargetingTab: offerTargetingTab,
        ocdPath: ocdPath,
        facetOptionTab: facetOptionTab,
        localizedTemplateText: localizedTemplateText,
        localizedTemplateString: localizedTemplateString,
        linkTab: linkTab
    };
};
