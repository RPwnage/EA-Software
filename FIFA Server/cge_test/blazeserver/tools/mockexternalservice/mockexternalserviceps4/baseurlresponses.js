//Author: Sze Ben
//Date: 2016

var util = require("../common/responseutil");
var log = require("../common/logutil");

function get_baseurl_sessioninvitation(requestHeaders, response)
{
    if (util.isUnspecifiedNested(requestHeaders, 'host')) {
        log.ERR("[BASEURL]get_baseurl_sessioninvitation: unexpected internal state: request missing header host.");
        response.statusCode = 400;
        return "";
    }
    response.statusCode = 200;
    return { "url" : requestHeaders.host };
}

module.exports.get_baseurl_sessioninvitation = get_baseurl_sessioninvitation;
