/**
 * @file preload.js
 *
 * this file is not part of the angular application and is used to preload items needed for offline mode in the Origin Client
 */

 /* globals preloadHelper */ //C++ object that gets created once webchannel is intialized

(function(originWrapper) {
    'use strict';

    var HTTP_REQUEST_TIMEOUT = 30000, //milliseconds
        //we don't hash for templates
        basedata =  'https://{envdot}data1.origin.com/{cmsstage}',
        countrylocale,
        mustLoad = {
        app: [
            '{storefrontlocale}game-library/{queryparams}',
            'views/search.html',
            'views/store/nux.html',
            'views/ogd.html',
            'views/ogdtabs.html',
            'views/profiletabs.html',
            'views/store.html',
            'views/store/store-home.html',
            'views/store/storenav.html',
            'views/profile.html',
            'views/settings.html',

            //OIG
            'profile{queryparams}{oigcontext}',
            'settings/oig{queryparams}{oigcontext}',
            'oig-download-manager{queryparams}{oigcontext}',
            'views/oig/downloadmanager.html',

/*
            'views/notfound.html',
            'views/footerclient.html',
            'views/home.html',
            'views/mygames.html',
            'views/store/about.html',
            'views/store/sitestripes.html',
*/
            'configs/app-config.json',
            'configs/components-config.json',
            'configs/environment/{env}/components-nonorigin-config.json',
            'configs/jssdk-origin-config.json',
            'configs/environment/{env}/jssdk-nonorigin-config.json',

        ],
        component: [
            //images
            'images/empty-library.jpg',
            'images/loader.png',
            'images/packart-placeholder.jpg',
            'images/user-placeholder.jpg',
            'images/search/noresults_1800x450.jpg',

            //css
            'styles/origincomponents.css',
        ],
        otk: [
            'otk.css',
            'fonts/origin.woff',
        ],
        xhr: [
            'https://fonts.googleapis.com/css?family=Open+Sans:400,300,600',
            'https://fonts.gstatic.com/s/opensans/v13/cJZKeOuBrn4kERxqtaUH3VtXRa8TVwTICgirnJhmVJw.woff2',
            'https://fonts.gstatic.com/s/opensans/v13/MTP_ySUJH_bn48VBG8sNSugdm0LZdjqr5-oayXSOefg.woff2',
            'https://fonts.gstatic.com/s/opensans/v13/DXI1ORHCpsQm3Vp6mXoaTegdm0LZdjqr5-oayXSOefg.woff2',
            'https://fonts.gstatic.com/s/opensans/v13/u-WUoqrET9fUeobQW7jkRRJtnKITppOI_IvcXXDNrsc.woff2',
            'https://fonts.gstatic.com/s/opensans/v13/xozscpT2726on7jbcb_pAhJtnKITppOI_IvcXXDNrsc.woff2',
            'https://fonts.gstatic.com/s/opensans/v13/RjgO7rYTmqiVp7vzi-Q5URJtnKITppOI_IvcXXDNrsc.woff2'
        ],
        templates: [
            '{basedata}template/error/notfound.tidy.{countrylocale}.directive',
            '{basedata}template/my-home/my-home.{countrylocale}.directive',
            '{basedata}template/game-library/mygames.tidy.{countrylocale}.directive',
            '{basedata}template/store/home.{countrylocale}.tidy.directive',
            '{basedata}template/store/about.{countrylocale}.tidy.directive',
            '{basedata}template/store/search-no-results.{countrylocale}.directive',
/*
            "vat": "{basedata}template/store/vat.{countrylocale}.tidy.directive",
            "download": "{basedata}template/store/download.{countrylocale}.tidy.directive",
            "great-game-guarantee": "{basedata}template/store/great-game-guarantee.{countrylocale}.tidy.directive",
            "great-game-guarantee-terms": "{basedata}template/store/great-game-guarantee-terms.{countrylocale}.tidy.directive",
            "pdp": "{basedata}template/games/{franchise}{game}{type}{edition}/jcr:content/templates/pdp.tidy.{countrylocale}.directive",
*/
            '{basedata}template/globalsitestripes.{countrylocale}.directive',
            '{basedata}template/store/sitestripe.tidy.{countrylocale}.directive',
/*
            "oax-landing-page": "{basedata}template/store/origin-access.tidy.{countrylocale}.directive",
            "oax-trials": "{basedata}template/store/origin-access/trials.tidy.{countrylocale}.directive",
            "oax-faq": "{basedata}template/store/origin-access/faq.tidy.{countrylocale}.directive",
            "oax-terms": "{basedata}template/store/origin-access/terms.tidy.{countrylocale}.directive",
            "oax-vault": "{basedata}template/store/origin-access/vault-games.tidy.{countrylocale}.directive",
            "oax-real-deal": "{basedata}template/store/oax-real-deal.tidy.{countrylocale}.directive",

            "freegames-hub": "{basedata}template/store/free-games.tidy.{countrylocale}.directive",
            "freegames-gametime": "{basedata}template/store/free-games/trials.tidy.{countrylocale}.directive",
            "freegames-onthehouse": "{basedata}template/store/free-games/on-the-house.tidy.{countrylocale}.directive",
            "freegames-demosbetas": "{basedata}template/store/free-games/demos-and-betas.tidy.{countrylocale}.directive",
            "deals-center": "{basedata}template/store/deals.tidy.{countrylocale}.directive",
*/
            '{basedata}template/shell/shell-navigation.tidy.{countrylocale}.directive',
            '{basedata}template/store/footer-web.tidy.{countrylocale}.directive',
            '{basedata}template/store/footer-client.tidy.{countrylocale}.directive',

            //not really template but don't want to set up another section just for this
            '{basedata}defaults/web-defaults/defaults.{countrylocale}.config'
        ]
    };


    function djb2Code(str) {
        var chr,
            hash = 5381,
            len = str.length / 2;

        for (var i = str.length-1; i >= len; i--) {
            chr = str.charCodeAt(i);
            hash = hash + chr;
        }
        return hash;
    }

    function computeHashIndex(urlpath) {
        var hashVal = djb2Code(urlpath);

        hashVal = hashVal % 4;
        return hashVal;
    }


    function getPathPriv() {
        //IE10 doesn't support window.location.origin

        if (typeof window.location.origin === 'undefined') {
            window.location.origin = window.location.protocol + '//' + window.location.host;
        }
        var loc = window.location.origin + window.location.pathname;
        //need to strip out any html refeence in the location
        var dir = loc.substring(0, loc.lastIndexOf('/'));
        return dir;
    }

    function absolutize(endPoint) {
        if (endPoint.indexOf('http') === -1) {
            endPoint = getPathPriv() + '/' + endPoint;
        }
        return endPoint;
    }

    function httpRequest(endPoint, headers) {
        //endPoint = encodeURI(endPoint);
        return new Promise(function(resolve, reject) {
            var req;

            function requestSuccess(data) {
preloadHelper.log(endPoint+': success');
                resolve(data);
            }

            function requestError(status, textStatus, response) {
                var msg = 'httpRequest ERROR: ' + endPoint + ' ' + status + ' (' + textStatus + ')',
                    error = new Error(msg),
                    errResponse = {};

                preloadHelper.log(endPoint+status);

                error.status = status;
                error.endPoint = endPoint;

                if (response && response.length > 0) {
                    try {
                        errResponse = JSON.parse(response);
                    } catch (error) {
                        console.error('ERROR:dataManager requestError could not parse response JSON', response, ' for:', endPoint);
                        errResponse = {};
                        // its not json
                    }
                }
                error.response = errResponse;
                reject(error);
            }

            if (endPoint === '') {
                requestError(-1, 'empty endPoint', '');
            } else {
                //now take the path portion of the endpoint and hash it
                if ((endPoint.indexOf('api{num}') > 0) || (endPoint.indexOf('data{num}') > 0)) {
                    var origincom = endPoint.indexOf('origin.com/'),
                        portion2beHashed;

                    portion2beHashed = endPoint.substr(origincom+11);
                    var hashIndex = computeHashIndex(portion2beHashed) + 1;

                    endPoint = endPoint.replace('{num}', hashIndex);
                }

                req = new XMLHttpRequest();
                req.open('GET', endPoint, true /*async*/ );
                if (headers) {
                    for (var i = 0, j = headers.length; i < j; i++) {
                        req.setRequestHeader(headers[i].label, headers[i].val);
                    }
                }
                req.timeout = HTTP_REQUEST_TIMEOUT;

                req.onload = function() {
                    if (req.status === 200) {
                        requestSuccess(req.response);
                    } else {
                        requestError(req.status, req.statusText, req.response);
                    }
                };

                // Handle network errors
                req.onerror = function() {
                    preloadHelper.log(endPoint+': network error');
                    requestError('-1', 'XHR_Network_Error', '');
                };

                req.ontimeout = function() {
                    preloadHelper.log(endPoint+': timeout');
                    requestError('-2', 'XHR_Timed_Out', '');
                };

                req.send();
            }
        });
    }


    function preloadCriticalComplete(success) {
        console.log('preloadCriticalComplete:', success);
        preloadHelper.log('preloadCriticalComplete:'+success);

        //report across webchannel that we're done
        preloadHelper.preloadComplete(success);
    }

    function handlePreloadCriticalSuccess() {
        preloadCriticalComplete(true);
    }

    function handlePreloadCriticalFailure(response) {
        //this will return the first error it encountered, if there is more than one it will only show the first
        console.error('[PRELOAD:preloadCritical FAILED', response.data + '(' + response.status + ')');
        preloadHelper.log('[PRELOAD:preloadCritical FAILED'+response.data + '(' + response.status + ')');
        preloadCriticalComplete(false);
    }

    function handleExceptions(error) {
        console.error(error.stack);
    }

    function preloadForOffline(urlParams) {
        var i,
            storefrontlocale,
            envcmsstage,
            queryparams,
            oigcontext,
            replaceStr,
            endPoint,
            params = [],
            criticalRequestPromises = [],
            templateheaders = [{
                'label': 'accept',
                'val': 'test/html; application/json; x-cache/force-write'
            }];


        //need to construct the storefront/locale/ portion of the path so game-library will get loaded with the correct values
        storefrontlocale = urlParams.country + '/' + urlParams.locale + '/';

        if (urlParams.env) {
            params.push('env='+urlParams.env);
        }

        if (urlParams.cmsstage) {
            params.push('cmsstage='+urlParams.cmsstage);
        }

        if (params.length > 0) {
            envcmsstage = '?' + params.join('&');
        }


        //if env and or cmsstage exists, then oigcontext needs to be &oigcontext
        //same thing with any other queryparams
        if (envcmsstage) {
            oigcontext = '&oigcontext=true';
        } else {
            oigcontext = '?oigcontext=true';
            envcmsstage = '';
        }


        if (urlParams.otherparams) {
            params.push(urlParams.otherparams);
        }
        if (params.length > 0) {
            queryparams = '?' + params.join('&');
        } else {
            queryparams = '';
        }

        //if env is empty, it's production, but for configs, we need to specifically have 'production'
        if (urlParams.env) {
            replaceStr = urlParams.env;
        } else {
            replaceStr = 'production';
        }

        for (i = 0; i < mustLoad.app.length; i++) {
            endPoint = absolutize(mustLoad.app[i]);
            //do all the substitions
            endPoint = endPoint.replace('{storefrontlocale}', storefrontlocale);
            endPoint = endPoint.replace('{env}', replaceStr);
            endPoint = endPoint.replace('{envcmsstage}', envcmsstage);
            endPoint = endPoint.replace('{oigcontext}', oigcontext);
            endPoint = endPoint.replace('{queryparams}', queryparams);
            criticalRequestPromises.push(httpRequest(endPoint, templateheaders));
        }

        //now go thru and load the component related stuff
        for (i = 0; i < mustLoad.component.length; i++) {
            endPoint = absolutize('bower_components/origin-components/dist/' + mustLoad.component[i]);
            criticalRequestPromises.push(httpRequest(endPoint));
        }

        //now get otk
        for (i = 0; i < mustLoad.otk.length; i++) {
            endPoint = absolutize('bower_components/otk/dist/' + mustLoad.otk[i]);
            criticalRequestPromises.push(httpRequest(endPoint));
        }

        //xhr requests to be cached
        for (i = 0; i < mustLoad.xhr.length; i++) {
            endPoint = mustLoad.xhr[i];
            criticalRequestPromises.push(httpRequest(endPoint));
        }

        //templates to be cached, they need to be separated out because they need force-write headers added as well as specifying xml
        replaceStr = '';
        if (urlParams.env && urlParams.env.toLowerCase() !== 'production') {
            replaceStr = urlParams.env + '.';
        }
        basedata = basedata.replace('{envdot}', replaceStr);

        replaceStr = '';
        if (urlParams.cmsstage) {
            replaceStr = urlParams.cmsstage + '/';
        }
        basedata = basedata.replace('{cmsstage}', replaceStr);
        countrylocale = urlParams.locale + '.' + urlParams.country;

        for (i = 0; i < mustLoad.templates.length; i++) {
            endPoint = mustLoad.templates[i];
            endPoint = endPoint.replace('{basedata}', basedata);
            endPoint = endPoint.replace('{countrylocale}', countrylocale);
            criticalRequestPromises.push(httpRequest(endPoint, templateheaders));
        }

        Promise.all(criticalRequestPromises)
            .then(handlePreloadCriticalSuccess, handlePreloadCriticalFailure)
            .catch(handleExceptions);
    }

    function onWebChannelReady() {
        preloadHelper.urlParams()
            .then(preloadForOffline);
    }

    function handleWebChannelError() {
        console.error('[webchannel] error');
        preloadHelper.log('[webchannel] error');
    }

    function init() {
        originWrapper.wrapWithWebChannel()
            .then(onWebChannelReady)
            .catch(handleWebChannelError);
    }

    init();

}(window.originWrapper));
