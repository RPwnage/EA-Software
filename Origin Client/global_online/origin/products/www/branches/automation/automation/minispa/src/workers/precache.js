/**
 * @file precache.js
 *
 * this file is used in a worker. it can be used to go and fetch a url asynchronously
 * we currently use it for supercat but can be expanded to handle other network requests and
 * the subsequent parsing
 */

/* globals self */

(function() {
    'use strict';


    var HTTP_REQUEST_TIMEOUT = 30000; //milliseconds

    function djb2Code(str) {
        var chr,
            hash = 5381,
            len = str.length / 2;

        for (var i = str.length - 1; i >= len; i--) {
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

    function httpRequest(endPoint, headers) {

        //now take the path portion of the endpoint and hash it
        if ((endPoint.indexOf('api{num}') > 0) || (endPoint.indexOf('data{num}') > 0)) {
            var origincom = endPoint.indexOf('origin.com/'),
                portion2beHashed;

            portion2beHashed = endPoint.substr(origincom + 11);
            var hashIndex = computeHashIndex(portion2beHashed) + 1;

            endPoint = endPoint.replace('{num}', hashIndex);
        }

        var req;

        function requestError(status, textStatus, response) {
            var msg = 'httpRequest ERROR: ' + endPoint + ' ' + status + ' (' + textStatus + ')',
                error = new Error(msg),
                errResponse = {};

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
            handlePreCacheFailure(error);

        }


        if (endPoint === '') {
            requestError(-1, 'empty endPoint', '');
        } else {
            req = new XMLHttpRequest();
            req.open('GET', endPoint, true /*async*/ );
            req.responseType = 'arraybuffer';

            if (headers) {
                for (var i = 0, j = headers.length; i < j; i++) {
                    req.setRequestHeader(headers[i].label, headers[i].val);
                }
            }
            req.timeout = HTTP_REQUEST_TIMEOUT;


            req.onload = function() {
                if (req.status === 200) {
                    //we use a binary object here because they are transferrable and go across
                    //to the main thread quicker than if we sent a JSON object across
                    handlePreCacheSuccess(new Uint8Array(req.response));
                } else {
                    requestError(req.status, req.statusText, req.response);
                }
            };

            // Handle network errors
            req.onerror = function() {
                requestError('-1', 'XHR_Network_Error', '');
            };

            req.ontimeout = function() {
                requestError('-2', 'XHR_Timed_Out', '');
            };

            req.send();
        }
    }

    function handlePreCacheSuccess(result) {
        self.postMessage(result.buffer, [result.buffer]);
        self.close();
    }

    function handlePreCacheFailure(error) {
        self.postMessage(JSON.stringify(error.message));
        self.close();
    }

    /**
     * the "public" facing method of this worker file. can be used with any url
     * @param  {string} url the url you want to fetch
     */
    function preCache(url) {
        var headers = [{
            'label': 'Accept',
            'val': 'application/json'
        }];

        httpRequest(url, headers);
    }

    self.addEventListener('message', function(e) {
        preCache(e.data);
    });
}());