var http = require("http");
var url = require("url");

function start(route, handle) {
  function onRequest(request, response) {
    var pathname = url.parse(request.url).pathname;
    route(handle, request.method, pathname, response);
  }

  http.createServer(onRequest).listen(8081);
}

exports.start = start;

