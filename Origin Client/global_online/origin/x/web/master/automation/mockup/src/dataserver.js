var express = require('express');
var bodyParser = require("body-parser");
var fs = require('fs');

// enable CORS
var allowCrossDomain = function(req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE,OPTIONS');
    res.header('Access-Control-Allow-Headers', req.headers['access-control-request-headers']);

    // intercept OPTIONS method
    if ('OPTIONS' == req.method) {
        res.sendStatus(200);
    } else {
        next();
    }
};

var app = express();
app.use(allowCrossDomain);
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies

// Store posted request body.
// All get call should check this queue map first. Return first found entry,
// otherwise return file base data.
var respQueueMap = {};

module.exports = {

    run : function(proxy_port, xmpp_server) {

        function knownExt(path) {
            var recs = ['.json', '.xml', '.htm', '.html', '.js', '.css', '.txt', '.log', '.dat', '.png', '.jpg', '.jpeg', '.gif', '.ocd', '.ico'];
            for (i in recs) {
                if (path.slice(-(recs[i].length)) === recs[i]) {
                    return true;
                }
            }
            return false;
        }

        function getContentType(filePath) {
            if (filePath.slice(-('.xml'.length)) === '.xml') {
                return 'text/xml;charset=UTF-8';
            } else if (filePath.slice(-('.json'.length)) === '.json') {
                return 'application/json';
            } else if (filePath.slice(-('.jpg'.length)) === '.jpg') {
                return 'image/jpeg';
            } else if (filePath.slice(-('.png'.length)) === '.png') {
                return 'image/jpeg';
            } else if (filePath.slice(-('.jpeg'.length)) === '.jpeg') {
                return 'image/jpeg';
            } else if (filePath.slice(-('.gif'.length)) === '.gif') {
                return 'image/jpeg';
            } else if (filePath.slice(-('.htm'.length)) === '.htm') {
                return 'text/html';
            } else if (filePath.slice(-('.html'.length)) === '.html') {
                return 'text/html';
            } else if (filePath.slice(-('.ocd'.length)) === '.ocd') {
                return 'application/json';
            } else if (filePath.slice(-('.ico'.length)) === '.ico') {
                return 'image/x-icon';
            }

            return 'text/plain';
        }

        function applyRequestUrl(configurloverride, protocol, url) {
            var str = configurloverride.toString();

            var protocolRegExp = new RegExp('{protocol}', 'g');
            str = str.replace(protocolRegExp, protocol);

            var tokenlist = url.split(':');

            var hostRegExp = new RegExp('{host}', 'g');
            str = str.replace(hostRegExp, tokenlist[0]);

            if (tokenlist.length > 1) {
                var portRegExp = new RegExp('{port}', 'g');
                str = str.replace(portRegExp, tokenlist[1]);
            }

            return new Buffer(str);
        }

        function checkQueue(map, key) {
            var queue = map[key];
            if (queue) {
                var entry = queue.shift();
                if (queue.length === 0) {
                    delete map[key];
                }
                return entry;
            }

            return undefined;
        }

        function getFilePath(request) {
            // try to get mockup data from file
            var filePath = request.params[0];

            // in case the filePath contains invalid char for file system
            filePath = filePath.replace(':', "%3A");

            // check if asking for file with specific extension
            if (!knownExt(filePath)) {
                filePath = filePath + '.json';
            }

            return filePath;
        }

        /**
         * Mix the data together
         * @return {void}
         */
        function mix(oldData, newData) {
            for (var key in newData) {
                if (!newData.hasOwnProperty(key)) {
                    continue;
                }
                oldData[key] = newData[key];
            }
        }

        function loadData(request, response) {

            var fullURL = request.protocol + '://' + request.get('host') + request.originalUrl;

            // check if queue has set a response for this request
            var presetResponse = checkQueue(respQueueMap, fullURL);
            if (presetResponse) {
                console.log('  shifting ' + fullURL);

                var respStatus = presetResponse.statusCode ? presetResponse.statusCode : 200;

                var respData;
                var filePath = getFilePath(request);
                if (presetResponse.data) {
                    respData = new Buffer(presetResponse.data);
                } else {
                    // try to get mockup data from file
                    var file = require("fs");
                    console.log('  reading ' + filePath);
                    respData = file.readFileSync(filePath);
                    if (!respData) {
                        respData = new Buffer('');
                    }
                }

                var respHeaders = {
                    'Content-Length' : respData.length,
                    'Access-Control-Allow-Origin' : request.headers['origin'] ? request.headers['origin'] : '*',
                    'Access-Control-Allow-Credentials' : true,
                    'Content-Type' : getContentType(filePath)
                };

                var presetHeaders = presetResponse.headers;
                if (presetHeaders) {
                    mix(respHeaders, presetHeaders);
                }

                //console.log('##################################################################################');
                //console.log(respHeaders);
                //console.log(respData);
                //console.log('&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&');
                response.writeHead(respStatus, respHeaders);
                response.write(respData);
                response.end();

                console.log(respQueueMap);
            } else {
                // try to get mockup data from file
                var filePath = getFilePath(request);
                console.log('  reading ' + filePath);

                var file = require("fs");
                file.readFile(filePath, function(error, respData) {

                    if (respData === undefined) {
                        // cannot find mockup data and cannot find pre-set response, either.
                        console.log('  cannot find ' + filePath);
                        var respHeaders = {
                            'Content-Length' : 0,
                            'Access-Control-Allow-Origin' : '*'
                        };

                        response.writeHead(404, respHeaders);
                        response.end();
                    } else {
                        // To allow this mockup server running from anywhere, it needs to return a dynamically
                        // generated configurloverride.json with user accessible URLs
                        if (filePath.slice(-('/jssdkconfig.json'.length)) === '/jssdkconfig.json') {
                            respData = applyRequestUrl(respData, request.protocol, request.get('host'));
                        }

                        var respHeaders = {
                            'Content-Length' : respData.length,
                            'Access-Control-Allow-Origin' : request.headers['origin'] ? request.headers['origin'] : '*',
                            'Access-Control-Allow-Credentials' : true,
                            'Content-Type' : getContentType(filePath)
                        };

                        //console.log('-#################################################################################');
                        //console.log(respHeaders);
                        //console.log(respData);
                        //console.log('-&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&');
                        response.writeHead(200, respHeaders);
                        response.write(respData);
                        response.end();
                        //console.log(response.req.headers);
                    }
                });
            }
        }

        app.post('/*', function(request, response) {
            var fullURL = request.protocol + '://' + request.get('host') + request.originalUrl;
            console.log('receiving POST ' + fullURL);
            loadData(request, response);
        });

        app.get('/*', function(request, response) {
            var fullURL = request.protocol + '://' + request.get('host') + request.originalUrl;
            console.log('receiving  GET ' + fullURL);
            loadData(request, response);
        });

        app.listen(proxy_port, function() {
            console.log('Local data server running at port: ' + proxy_port);
        });

    },

    inject : function(url, statusCode, headers, data) {
        var resp = {};

        if (url !== undefined) {
            try {
                var entry = {};

                if (statusCode) {
                    entry.statusCode = statusCode;
                } else {
                    entry.statusCode = 200;
                }

                if (headers) {
                    entry.headers = headers;
                }

                if (data) {
                    entry.data = data;
                }

                if (!respQueueMap[url]) {
                    respQueueMap[url] = [];
                }

                respQueueMap[url].push(entry);
                console.log('  added response for "' + url + '"', entry);

                resp.respData = new Buffer(data);
                resp.statusCode = 200;
            } catch(err) {
                resp.respData = err.message + ', please check POST request body. Expect headers parameter as JSON format';
                resp.statusCode = 400;
            }
        } else {
            resp.respData = 'missing url parameter';
            resp.statusCode = 400;
        }

        return resp;
    },

    reset : function() {
        console.log('  reset all responses');
        respQueueMap = {};
    }

};
