var path = require("path");
var fs = require("fs");

function style() {
  var s =
    "<style type=\"text/css\">\n" +
    "  body\n" +
    "    {\n" +
    "      background: #FFFFFF url(graphics/bg-body.gif) repeat-x scroll 0 0;\n" +
    "      font-family: \"Trebuchet MS\",Verdana,Arial,Helvetica,sans-serif;\n" +
    "      font-size: 76%;\n" +
    "      font-size-adjust: none;\n" +
    "      font-stretch: normal;\n" +
    "      font-style: normal;\n" +
    "      font-variant: normal;\n" +
    "      font-weight: normal;\n" +
    "      line-height: 160%;\n" +
    "      moz-background-clip: border;\n" +
    "      moz-background-inline-policy: continuous;\n" +
    "      moz-background-origin: padding;\n" +
    "      x-system-font: none;\n" +
    "    }\n" +
    "    p\n" +
    "    {\n" +
    "      text-align: center;\n" +
    "    }\n" +
    "    a:link\n" +
    "    {\n" +
    "      border-bottom: 1px dotted #FFBAC8;\n" +
    "      color: #D42945;\n" +
    "      text-decoration: none;\n" +
    "    }\n" +
    "    a:visited\n" +
    "    {\n" +
    "      border-bottom: medium none;\n" +
    "      color: #D42945;\n" +
    "      text-decoration: none;\n" +
    "    }\n" +
    "    a:hover, a:focus\n" +
    "    {\n" +
    "      border-bottom: 1px solid #F03B58;\n" +
    "      color: #F03B58;\n" +
    "      text-decoration: none;\n" +
    "    }\n" +
    "    table a, table a:link, table a:visited\n" +
    "    {\n" +
    "      border: medium none;\n" +
    "    }\n" +
    "    img\n" +
    "    {\n" +
    "      border: 0 none;\n" +
    "      margin-top: 0.5em;\n" +
    "    }\n" +
    "    table\n" +
    "    {\n" +
    "      border-collapse: collapse;\n" +
    "      border-right: 1px solid #E5EFF8;\n" +
    "      border-top: 1px solid #E5EFF8;\n" +
    "      margin: 1em auto;\n" +
    "      width: 50%;\n" +
    "    }\n" +
    "    caption\n" +
    "    {\n" +
    "      caption-side: top;\n" +
    "      color: #4F6B72;\n" +
    "      font-size: 1.5em;\n" +
    "      letter-spacing: 0.1em;\n" +
    "      margin: 1em 0 0;\n" +
    "      padding: 5px;\n" +
    "      text-align: center;\n" +
    "      white-space:nowrap;\n" +
    "    }\n" +
    "    tr.odd td\n" +
    "    {\n" +
    "      background: #F7FBFF none repeat scroll 0 0;\n" +
    "      moz-background-clip: border;\n" +
    "      moz-background-inline-policy: continuous;\n" +
    "      moz-background-origin: padding;\n" +
    "    }\n" +
    "    tr.odd .column1\n" +
    "    {\n" +
    "      background: #F4F9FE none repeat scroll 0 0;\n" +
    "      moz-background-clip: border;\n" +
    "      moz-background-inline-policy: continuous;\n" +
    "      moz-background-origin: padding;\n" +
    "    }\n" +
    "    .column1\n" +
    "    {\n" +
    "      background: #F9FCFE none repeat scroll 0 0;\n" +
    "      moz-background-clip: border;\n" +
    "      moz-background-inline-policy: continuous;\n" +
    "      moz-background-origin: padding;\n" +
    "    }\n" +
    "    td\n" +
    "    {\n" +
    "      border-bottom: 1px solid #E5EFF8;\n" +
    "      border-left: 1px solid #E5EFF8;\n" +
    "      color: #4F6B72;\n" +
    "      padding: 0.3em 1em;\n" +
    "      text-align: left;\n" +
    "      vertical-align: top;\n" +
    "      white-space: nowrap;\n" +
    "    }\n" +
    "    .num\n" +
    "    {\n" +
    "    text-align: right;\n" +
    "    }\n" +
    "    th\n" +
    "    {\n" +
    "      border-bottom: 1px solid #E5EFF8;\n" +
    "      border-left: 1px solid #E5EFF8;\n" +
    "      color: #678197;\n" +
    "      font-weight: normal;\n" +
    "      padding: 0.3em 1em;\n" +
    "      text-align: left;\n" +
    "    }\n" +
    "    tr th\n" +
    "    {\n" +
    "      background: #F4F9FE none repeat scroll 0 0;\n" +
    "      color: #66A3D3;\n" +
    "      font-family: \"Century Gothic\",\"Trebuchet MS\",Arial,Helvetica,sans-serif;\n" +
    "      font-size: 1.2em;\n" +
    "      font-size-adjust: none;\n" +
    "      font-stretch: normal;\n" +
    "      font-style: normal;\n" +
    "      font-variant: normal;\n" +
    "      font-weight: bold;\n" +
    "      line-height: 2em;\n" +
    "      moz-background-clip: border;\n" +
    "      moz-background-inline-policy: continuous;\n" +
    "      moz-background-origin: padding;\n" +
    "      text-align: left;\n" +
    "      x-system-font: none;\n" +
    "    }\n" +
    "    tfoot th\n" +
    "    {\n" +
    "      background: #F4F9FE none repeat scroll 0 0;\n" +
    "      moz-background-clip: border;\n" +
    "      moz-background-inline-policy: continuous;\n" +
    "      moz-background-origin: padding;\n" +
    "      text-align: center;\n" +
    "    }\n" +
    "    tfoot th strong\n" +
    "    {\n" +
    "      color: #66A3D3;\n" +
    "      font-family: \"Century Gothic\",\"Trebuchet MS\",Arial,Helvetica,sans-serif;\n" +
    "      font-size: 1.2em;\n" +
    "      font-size-adjust: none;\n" +
    "      font-stretch: normal;\n" +
    "      font-style: normal;\n" +
    "      font-variant: normal;\n" +
    "      font-weight: bold;\n" +
    "      line-height: normal;\n" +
    "      margin: 0.5em 0.5em 0.5em 0;\n" +
    "      x-system-font: none;\n" +
    "    }\n" +
    "    tfoot th em\n" +
    "    {\n" +
    "      color: #F03B58;\n" +
    "      font-size: 1.1em;\n" +
    "      font-style: normal;\n" +
    "      font-weight: bold;\n" +
    "    }\n" +
    "    td pre\n" +
    "    {\n" +
    "      font-size: 1em;\n" +
    "      margin: 0;\n" +
    "    }\n" +
    "</style>"

  return s;
}

function legend() {
  var s = 
    "<tr>\n" +
    "  <td colspan=\" 7 \" style=\"padding: 20px;\">\n" +
    "    <strong>Legend</strong><br>\n" +
    "    <font color=\"gray\"><i>Italics</i>: Soft dependency<br>\n" +
    "    <font color=\"gray\">PENDING</font>: Status of the system is not yet known (seen during startup).<br>\n" +
    "    <font color=\"green\">OK</font>: System is available<br>\n" +
    "    <font color=\"red\">FAIL</font>: System is not available<br>\n" +
    "    <font color=\"yellow\">TIMEOUT</font>: Checker did not return updated response<br>\n" +
    "  </td>\n" +
    "</tr>\n";

  return s;
}

function config() {
  var filepath = path.join(process.cwd(), "sonar-voiceserver/config.json");

  var data = fs.readFileSync(filepath);
  var json = JSON.parse(data);

  var s =
      "<tbody>\n" +
      "  <tr>\n" +
      "    <td>public</td><td>" + json.config.public + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>voipaddress</td><td>" + json.config.voipaddress + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>voipport</td><td>" + json.config.voipport + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>backendaddress</td><td>" + json.config.backendaddress + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>maxclients</td><td>" + json.config.maxclients + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>healthcheckinterval</td><td>" + json.config.healthcheckinterval + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>backendeventinterval</td><td>" + json.config.backendeventinterval + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>metricsfileinterval</td><td>" + json.config.metricsfileinterval + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>clientstatsfileinterval</td><td>" + json.config.clientstatsfileinterval + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>badchannelpercent</td><td>" + json.config.badchannelpercent + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>badclientpercent</td><td>" + json.config.badclientpercent + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>jitterbuffersize</td><td>" + json.config.jitterbuffersize + "</td>\n" +
      "  </tr>\n" +
      "  <tr>\n" +
      "    <td>jitterscalefactor</td><td>" + json.config.jitterscalefactor + "</td>\n" +
      "  </tr>\n" +
      "</tbody>\n";

  return s;
}

function version() {
  var filepath = path.join(process.cwd(), "../version.properties");

  var ver = fs.readFileSync(filepath);
  ver = String(ver).trim();

  return ver;
}

function healthCheckInterval() {
  var filepath = path.join(process.cwd(), "sonar-voiceserver/config.json");

  var data = fs.readFileSync(filepath);
  var json = JSON.parse(data);

  var s = json.config.healthcheckinterval;

  return s;
}

exports.style = style;
exports.legend = legend;
exports.config = config;
exports.version = version;
exports.healthCheckInterval = healthCheckInterval;
