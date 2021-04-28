var path = require("path");
var fs = require("fs");
var style = require("./style");

var mimeTypes = {
      "html": "text/html",
      "xml": "text/xml",
      "json": "application/json"};

var healthCheckLastModifiedTime = new Date();

var htmlStyle = style.style();
var htmlLegend = style.legend();
var htmlConfig = style.config();
var voiceServerVersion = style.version();
var healthCheckInterval = style.healthCheckInterval();

function fileNotFound(response) {
  response.writeHead(404, {"Content-Type": "text/plain"});
  response.write("404 Not Found\n");
  response.end();
}

function localToUtcHtml( localDateTime ) {
  var daysOfWeek = [ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" ]; 
  var monthsOfYear = [ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" ];

  var dayOfWeek = daysOfWeek[localDateTime.getUTCDay()];
  var monthOfYear = monthsOfYear[localDateTime.getUTCMonth()];
  var date = localDateTime.getUTCDate();
  var hours = localDateTime.getUTCHours();
  if( String(hours).length < 2) hours = "0" + hours;
  var minutes = localDateTime.getUTCMinutes();
  if( String(minutes).length < 2) minutes = "0" + minutes;
  var seconds = localDateTime.getUTCSeconds();
  if( String(seconds).length < 2) seconds = "0" + seconds;
  var year = localDateTime.getUTCFullYear();

  var formatted = dayOfWeek + " " + monthOfYear + " " + date + " " + hours + ":" + minutes + ":" + seconds + " UTC " + year;

  return formatted;
}

function localToUtcXml( localDateTime ) {
  var month = localDateTime.getUTCMonth() + 1;
  if( String(month).length < 2) month = "0" + month;
  var date = localDateTime.getUTCDate();
  if( String(date).length < 2) date = "0" + date;
  var hours = localDateTime.getUTCHours();
  if( String(hours).length < 2) hours = "0" + hours;
  var minutes = localDateTime.getUTCMinutes();
  if( String(minutes).length < 2) minutes = "0" + minutes;
  var seconds = localDateTime.getUTCSeconds();
  if( String(seconds).length < 2) seconds = "0" + seconds;
  var year = localDateTime.getUTCFullYear();

  var formatted = year + "-" + month + "-" + date + "T" + hours + ":" + minutes + ":" + seconds + "Z";

  return formatted;
}

function colorText( text, color ) {
  return "<font color=\"" + color + "\">" + text + "</font>";
}

function writeXmlResponse(json, response) { 
  var status = json.health.overallServicesStatus;

  var now = new Date();
  var nowFormattedUTC = localToUtcXml(now);
  var lastCheckDate = new Date(json.health.services[0].service.statusDetail.node.lastCheck);

  var lastFormattedUTC = "";
  if( status !== "PENDING" ) lastFormattedUTC = localToUtcXml(lastCheckDate);

  var age = "";
  if( status !== "PENDING" ) age = now.getTime() - lastCheckDate.getTime();

  var serviceStatus = json.health.services[0].service.statusDetail.node.status;
  if( status === "TIMEOUT" ) serviceStatus = "TIMEOUT";

  response.writeHead(200, {"Content-Type": "text/xml", "Cache-Control": "no-cache, no-store"});
  response.write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
  response.write("<health>\n");
  response.write("  <name>" + json.health.name + "</name>\n");
  response.write("  <version>" + voiceServerVersion + "</version>\n");
  response.write("  <overall-services-status>" + json.health.overallServicesStatus + "</overall-services-status>\n");
  response.write("  <last-generated>" + nowFormattedUTC + "</last-generated>\n");
  response.write("  <services>\n");
  response.write("    <service>\n");
  response.write("      <name>" + json.health.services[0].service.name + "</name>\n");
  response.write("      <status>" + json.health.services[0].service.status + "</status>\n");
  response.write("      <soft>" + "false" + "</soft>\n");
  response.write("      <status-detail>\n");
  response.write("        <node>\n");
  response.write("          <url>\n");
  response.write("            <![CDATA[ " + json.health.services[0].service.statusDetail.node.url + " ]]>\n");
  response.write("          </url>\n");
  response.write("          <status>" + serviceStatus + "</status>\n");
  response.write("          <last-check>" + lastFormattedUTC + "</last-check>\n");
  response.write("          <time-taken>" + json.health.services[0].service.statusDetail.node.timeTaken + "</time-taken>\n");
  response.write("          <age>" + age + "</age>\n");
  response.write("          <info>\n");
  response.write("            <![CDATA[ " + json.health.services[0].service.statusDetail.node.info + " ]]>\n");
  response.write("          </info>\n");
  response.write("        </node>\n");
  response.write("      </status-detail>\n");
  response.write("    </service>\n");
  response.write("  </services>\n");
  response.write("</health>");
  response.end();
}

function writeHtmlHead(response) {
  response.write("  <head>\n");
  response.write("    <title>Sonar Voice Server Status</title>\n");
  response.write(htmlStyle);
  response.write("  </head>\n");
}

function writeHtmlStatus(json, response) {
  response.write("    <table>\n");
  response.write("      <thead>\n");
  response.write("        <caption>" + json.health.name + " (" + voiceServerVersion + ") Overall Status: <b> <strong>");

  if( json.health.overallServicesStatus === 'OK' )
    response.write( colorText("OK", "green") );
  else if( json.health.overallServicesStatus === 'FAIL' )
    response.write( colorText("FAIL", "red") );
  else if( json.health.overallServicesStatus === 'TIMEOUT' )
    response.write( colorText("TIMEOUT", "yellow") );
  else
    response.write( colorText("PENDING", "gray") );

  var now = new Date();
  var nowFormatted = localToUtcHtml(now);
  response.write("</strong> </b> (" + nowFormatted + ")</caption\n");
  response.write("        <tr>\n");
  response.write("          <th scope=\"col\">Dependency</th>\n" +
                 "          <th scope=\"col\">URL</th>\n" +
                 "          <th scope=\"col\">Status</th>\n" +
                 "          <th scope=\"col\">At</th>\n" +
                 "          <th scope=\"col\">Took</th>\n" +
                 "          <th scope=\"col\">Age</th>\n" +
                 "          <th scope=\"col\">Info</th>\n" +
                 "        </tr>\n" +
                 "      </thead>\n");

  response.write("      <tbody>\n");

  // figure out color
  var serviceId = 0;
  var service = json.health.services[serviceId].service;
  var color = "green";
  var status = service.status;
  if( json.health.overallServicesStatus === "TIMEOUT" ) status = "TIMEOUT";

  if( status === 'FAIL' )
    color = "red";
  else if( status === 'TIMEOUT' )
    color = "yellow";
  else if( status === 'PENDING' )
    color = "gray";

  var serviceName = service.name;
  var serviceUrl = service.statusDetail.node.url;

  var serviceLastCheck = new Date(service.statusDetail.node.lastCheck);
  var serviceLastCheckFormatted = "-";
  if( status !== "PENDING" ) serviceLastCheckFormatted = localToUtcHtml(serviceLastCheck);

  var serviceTimeTaken = "-";
  if( status !== "PENDING" ) serviceTimeTaken = service.statusDetail.node.timeTaken + " ms";

  var serviceAge = "-";
  if( status !== "PENDING" ) serviceAge = (now.getTime() - serviceLastCheck.getTime()) + " ms";

  var serviceInfo = service.statusDetail.node.info;

  response.write("      <tr>\n");
  response.write("        <td>" + colorText( serviceName, color ) + "</td>\n");
  response.write("        <td>" + serviceUrl + "</td>\n");
  response.write("        <td><strong>" + colorText( status, color ) + "</strong></td>\n");
  response.write("        <td>" + serviceLastCheckFormatted + "</td>\n");
  response.write("        <td class=\"num\">" + serviceTimeTaken + "</td>\n");
  response.write("        <td>" + serviceAge + "</td>\n");
  response.write("        <td><pre></pre></td>\n");
  response.write("      </tr>\n");
  response.write(htmlLegend);
  response.write("      </tbody>\n");
  response.write("    </table>\n");
}

function writeHtmlConfiguration(json, response) {
  response.write("    <table>\n");
  response.write("      <caption>" + json.health.name + " (" + voiceServerVersion + ") Configuration Properties</caption>\n");
  response.write("      <thead>\n" +
                 "        <tr>\n" +
                 "          <th scope=\"col\">Property</th>\n" +
                 "          <th scope=\"col\">Value</th>\n" +
                 "        </tr>\n" +
                 "      </thead>\n");
  response.write(htmlConfig);
  response.write("    </table>\n");
}

function writeHtmlBody(json, response) {
  response.write("  <body>\n");
  writeHtmlStatus(json, response);
  response.write("<br/><br/>\n");
  writeHtmlConfiguration(json, response);
  response.write("  </body>\n");
}

function writeHtmlResponse(json, response) {
  response.writeHead(200, {"Content-Type": "text/html", "Cache-Control": "no-cache, no-store"});
  response.write("<!DOCTYPE html PUBLIC\"-// W3C//DTD HTML 4.01//EN\">\n");
  response.write("<html>\n");
  writeHtmlHead(response);
  writeHtmlBody(json, response);
  response.write("</html>");
  response.end();
}

function writeResponse(filetype, json, response) {
  if( filetype === 'xml' ) {
    writeXmlResponse(json, response);
  }
  else if( filetype === 'html' ) {
    writeHtmlResponse(json, response);
  }
  else {
    fileNotFound(response);
  }
}

function respond(filepath, response) {
  // file type to generate
  var filetype = path.extname(filepath).split(".").reverse()[0];
  var filename = path.basename(filepath, path.extname(filepath));
  var dirname = path.dirname(filepath);

  // get the json file
  var jsonFilename = path.join(dirname, filename + ".json");

  var stats;
  try {
    stats = fs.lstatSync(jsonFilename); // throws if path doesn't exist
  } catch(e) {
    fileNotFound(response);
    return;
  }

  if (stats.isFile()) { // path exists, is a file
    // get last modified time
    var now = new Date();
    var updateExpired = (now.getTime() - healthCheckLastModifiedTime.getTime()) > (healthCheckInterval + 2000); // interval + margin
    healthCheckLastModifiedTime = stats.mtime;

    fs.readFile(jsonFilename, function (err, data) {
      if (err) {
        return fileNotFound(response);
      }

      var parsedJson = JSON.parse(data);

      if( updateExpired && parsedJson.health.overallServicesStatus !== "PENDING" ) {
        parsedJson.health.overallServicesStatus = "TIMEOUT";
      }

      writeResponse(filetype, parsedJson, response);
    });
  }
}

function getFile(filename, response) {
  var filepath = path.join(process.cwd(), filename);

  respond(filepath, response);
}

function getFileJson(filename, response) {
  var filepath = path.join(process.cwd(), filename);
  fs.readFile(filepath, function (err, data) {
    if (err) {
      return fileNotFound(response);
    }

    response.writeHead(200, {"Content-Type": "application/json", "Cache-Control": "no-cache, no-store"});
    response.write(data);
    response.end();
  });
}

function metrics(method, response) {
  if( method === "GET" ) {
    var filepath = path.join(process.cwd(), "/sonar-voiceserver/metrics.json");
    fs.readFile(filepath, function (err, data) {
      if (err) {
        return fileNotFound(response);
      }

      response.writeHead(200, {"Content-Type": "application/json", "Cache-Control": "no-cache, no-store"});
      response.write(data);
      response.end();
    });
  }
  else {
    fileNotFound(response);
  }
}

exports.getFile = getFile;
exports.getFileJson = getFileJson;
exports.metrics = metrics;
