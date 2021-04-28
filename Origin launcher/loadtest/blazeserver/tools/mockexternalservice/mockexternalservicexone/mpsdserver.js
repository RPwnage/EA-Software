//Author: Chaitanya Tumu, Sze Ben
//Date: 2016

var http = require("http");
var url = require("url");
var serviceconfigsResponses = require("./serviceconfigsresponses");
var usersResponses = require("./usersresponses");
var batchResponses = require("./batchresponses");
var util = require("../common/responseutil");
var log = require("../common/logutil");
var simOutage = require("../common/outageutil");

var mPort = 8080;//default
var mConfigReader = null;

// process cmd line at startup.
process.argv.forEach(function (val, index, array) {
    
    var argvPort = '-port=';
    var argvConfig = '-config=';
    var argvLogDir = '-logdir=';

    // parse args
    if (index !== 0 && index !== 1) {
        var arg = val.toLowerCase();
        if (arg.substr(0, argvPort.length) === argvPort) {
            if (!Number.isNaN(Number(arg.substr(argvPort.length)))) {
                mPort = Number(arg.substr(argvPort.length));
            }
            else {
                console.error("Invalid port specified '" + val + "', using default port " + mPort);
            }
        }
        else if (arg.substr(0, argvLogDir.length) === argvLogDir) {
            log.setLogFileDir(arg.substr(argvLogDir.length));
        }
        else if (arg.substr(0, argvConfig.length) === argvConfig) {
            var configFile = arg.substr(argvConfig.length);
            mConfigReader = require('readline').createInterface({
                input: require('fs').createReadStream(configFile)
            });
        }
        else {
            console.error("Invalid/unhandled cmd line param: " + val);
        }
    }
});

// process config at startup. Note for performance, console/httpdetail/mpsd logging by default are disabled
mConfigReader.on('line', function (line) {
    
    var argComment = '//';
    var argRepI = 'reportinterval=';
    var argFile = 'logfile=';
    var argCons = 'logconsole';
    var argHttpI= 'loghttp=info';
    var argHttpT= 'loghttp=trace';
    var argMpsd = 'logmpsd';
    var argOutage = 'simoutage=';

    var arg = line.trim().toLowerCase();
    // skip comments
    if (arg.substr(0, argComment.length) === argComment) {
        return;
    }

    // parse args
    if (arg.substr(0, argRepI.length) === argRepI) {
        if (!Number.isNaN(Number(arg.substr(argRepI.length)))) {
            serviceconfigsResponses.setMaxOpCountBeforeReport(Number(arg.substr(argRepI.length)));
        }
        else {
            console.error("Invalid reportinterval '" + val + "', using default interval");
        }
    }
    else if (arg.substr(0, argFile.length) === argFile) {
        log.setLogFileNamePrefix(arg.substr(argFile.length));
    }
    else if (arg.substr(0, argHttpI.length) === argHttpI) {
        log.setDoLogHttpInfo(true);
    }
    else if (arg.substr(0, argHttpT.length) === argHttpT) {
        log.setDoLogHttpTrace(true);
    }
    else if (arg.substr(0, argCons.length) === argCons) {
        log.setDoLogConsole(true);
    }
    else if (arg.substr(0, argMpsd.length) === argMpsd) {
        serviceconfigsResponses.setDoLogMpsd(true);
    }
    else if (arg.substr(0, argOutage.length) === argOutage) {

        var outageRate = parseInt(arg.substr(argOutage.length), 10);
        if (isNaN(outageRate)) {
            console.error("Invalid/unhandled config param/line: " + line);
        }
        else {
            simOutage.setSimOutageRate(outageRate);
        }
    }
    else {
        console.error("Invalid/unhandled config param/line: " + line);
    }
});


//parse JSON body, before handling request
function onRequest(request, response) {
    var requestBodyJSON = "";
    var requestBody = "";
    request.on('data', function (data) {
        try {
            requestBody += data.toString();
        } catch (err) {
            log.ERR("[MPSD]onRequest: exception caught: " + err.message + ", stack: " + err.stack);
            return;
        }
    });
    request.on('end', function (data) {
        if (requestBody !== "") {
            try {
                requestBodyJSON = JSON.parse(requestBody);
            } catch (err) {
                log.ERR("[MPSD]onRequest: exception caught: " + err.message + ", stack: " + err.stack);
                return;
            }
        }
        handleOnRequest(request, response, requestBodyJSON);
    });
}

//Handle the requests
function handleOnRequest(request, response, requestBodyJSON) {
    try {
        log.logReq(request, requestBodyJSON);
    
        var parsedUrl = url.parse(request.url, true);
        var pathname = parsedUrl.pathname;
        var method = request.method;
        var responseBody = "";
        var responseCode = 200;
    
        var ma = pathname.split('/');
        if (ma !== null) {
            module = ma[1];
            branch = (ma.length >=4 ? ma[3] : "");
            switch (method) {
                case "GET":
                    if (module === "serviceconfigs") {
                        if (branch === "sessiontemplates") {
                            if (ma.length === 5) {
                                //GET /serviceconfigs/{scid}/sessiontemplates/{sessionTemplateName}

                                // No outage simulation for retrieving the session templates. At boot, we retrieve these to ensure that
                                // configuration is correct. The instance will fail to boot if it runs into a failure here (and that is the desired behavior.)
                                responseBody = JSON.stringify(serviceconfigsResponses.sessions_get_sessiontemplate(ma[4], response));
                                responseCode = response.statusCode;
                            }
                            else if (ma.length === 7 && ma[5] === "sessions") {
                                //GET /serviceconfigs/{scid}/sessiontemplates/{sessionTemplateName}/sessions/{sessionName}
                                if (simOutage.shouldSimOutage(request.headers)) {
                                    responseBody = "";
                                    responseCode = simOutage.outageErrCode();
                                }
                                else {
                                    responseBody = JSON.stringify(serviceconfigsResponses.sessions_get_response(ma[6], response));
                                    responseCode = response.statusCode;
                                }
                            }
                        }
                        else if (branch === "sessions") {
                            //GET /serviceconfigs/{scid}/sessions?xuid=
                            if (util.isUnspecified(parsedUrl.query.xuid)) {
                                log.ERR("[MPSD]handleOnRequest: incorrect format, expected /sessions?xuid=");
                                responseCode = 400;
                            }
                            responseBody = JSON.stringify(serviceconfigsResponses.sessions_getsessionsxuid_response(parsedUrl.query.xuid, response));
                            responseCode = response.statusCode;
                        }
                    }
                    else if (module === "users") {
                        if (branch === "people") {
                            responseBody = JSON.stringify(usersResponses.people_get_response());
                        }
                        else if (branch === "scids") {
                            responseBody = JSON.stringify(usersResponses.scids_get_response());
                        }
                    }
                    else {
                        responseCode = 400;
                    }
                    break;
                case "POST":
                    switch (module) {
                        case "handles":
                            if (ma.length === 2) {
                                //POST /handles
                                responseBody = JSON.stringify(serviceconfigsResponses.handles_post_setactivity(request, requestBodyJSON, response));
                                responseCode = response.statusCode;
                            }
                            else if (ma.length > 2 && ma[2] === "query") {
                                //POST /handles/query?include=relatedInfo
                                responseBody = JSON.stringify(serviceconfigsResponses.handles_post_getactivity(requestBodyJSON, response));
                                responseCode = response.statusCode;
                            }
                            else {
                                responseCode = 400;
                            }
                            break;
                        case "users":
                            if (branch === "profile") {
                                responseBody = JSON.stringify(usersResponses.profile_post_response());
                            }
                            else if (branch === "feedback") {
                                responseBody = JSON.stringify(usersResponses.empty_post_response());
                                responseCode = 204;
                            }
                            else if (branch === "resetreputation") {
                                responseBody = JSON.stringify(usersResponses.empty_post_response());
                            }
                            else {
                                responseCode = 400;
                            }
                            break;
                        case "batch":
                            responseBody = JSON.stringify(batchResponses.batch_post_response());
                            break;
                        case "system":
                            responseBody = "";
                            break;
                        default:
                            responseCode = 400;
                    }
                    break;
                case "PUT":
                    if (module === "serviceconfigs" && branch === "sessiontemplates" && (ma.length === 7 && ma[5] === "sessions")) {
                        //PUT serviceconfigs/{scid}/sessiontemplates/{sessionTemplateName}/sessions/{sessionName}?{nocommit}
                        if (simOutage.shouldSimOutage(request.headers)) {
                            responseBody = "";
                            responseCode = simOutage.outageErrCode();
                        }
                        else {
                            var noCommit = (!util.isUnspecified(parsedUrl.query.nocommit) ? parsedUrl.query.nocommit : 'false');
                            responseBody = JSON.stringify(serviceconfigsResponses.sessions_put_response(ma[2], ma[4], ma[6], request.headers, requestBodyJSON, response, noCommit));
                            responseCode = response.statusCode;
                        }
                    }
                    else {
                        responseCode = 400;
                    }
                    break;
                case "DELETE":
                    if (module === "serviceconfigs" && branch === "sessiontemplates") {
                        //Side: DELETE calls aren't actually implemented by Blaze currently.
                        responseBody = "";
                        responseCode = 204;
                    }
                    else {
                        responseCode = 400;
                    }
                    break;
                default:
                    responseCode = 400;
            }

            log.logRsp(request, responseCode, responseBody);

            if (util.CONST_DEFAULT_COMMAND_DELAY_MS !== 0) {
                setTimeout(function () { util.writeResponse(response, responseCode, responseBody); }, util.CONST_DEFAULT_COMMAND_DELAY_MS);
            } else {
                util.writeResponse(response, responseCode, responseBody);
            }
        }
        else {
            log.logRsp(request, 400, responseBody);
            util.writeResponse(response, 400, responseBody);
        }
    } catch (err) {
        log.ERR("[MPSD]handleOnRequest: exception caught: " + err.message + ", stack: " + err.stack);
    }
}


//create server and listen on the port
log.initLogFileDir();
console.log("Mock service listening on port " + mPort + "..");
server = http.createServer(onRequest).listen(mPort);



