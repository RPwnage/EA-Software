var server = require("./server");
var router = require("./router");
var requestHandlers = require("./requestHandlers");

var handle = {}
handle["getFile"] = requestHandlers.getFile;
handle["getFileJson"] = requestHandlers.getFileJson;
handle["/metrics"] = requestHandlers.metrics;

server.start(router.route, handle);
