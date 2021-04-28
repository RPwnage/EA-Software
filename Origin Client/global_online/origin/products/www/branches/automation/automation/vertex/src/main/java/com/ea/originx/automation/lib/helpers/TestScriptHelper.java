package com.ea.originx.automation.lib.helpers;

import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.vx.originclient.net.helpers.RestfulHelper;
import com.ea.vx.originclient.templates.OASiteTemplate;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.json.JSONArray;
import org.openqa.selenium.By;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.Keys;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.lang.invoke.MethodHandles;
import java.net.CookieHandler;
import java.net.CookieManager;
import java.net.CookiePolicy;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TimeZone;

/**
 * A collection of helpers that are related to test scripts.
 */
public class TestScriptHelper {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    // List of browsers for automation test purposes
    private static final List<String> BROWSER_PROCESSES = Arrays.asList("iexplore.exe", "chrome.exe", "firefox.exe", "MicrosoftEdge.exe");
    private static final float ACCESS_DISCOUNT_VALUE = .10f;

    /**
     * Checks if Origin's offline cache exists.
     *
     * @param client {@link OriginClient} The Origin Client instance
     * @return true if the cache exists, false otherwise
     */
    public static boolean verifyOfflineCacheExists(OriginClient client) {
        final String nodeFullURL = client.getNodeURL();
        if (null != nodeFullURL) {
            try {
                final String url = nodeFullURL + "/directory/list?path=" + URLEncoder.encode(getOfflineCachePath(client), "UTF-8");
                final String res = RestfulHelper.get(url);
                final JSONArray result = new JSONArray(res);
                return result.length() > 0;
            } catch (IOException | URISyntaxException e) {
                _log.error(e);
            }
        }
        return false;
    }

    /**
     * Gets the path to the offline cache.
     *
     * @param client {@link OriginClient} The Origin Client instance
     * @return The offline cache folder path
     */
    public static String getOfflineCachePath(OriginClient client) {
        return "C:\\Users\\" + client.getAgentOSInfo().getEnvValue("USERNAME") + "\\AppData\\Local\\Origin\\Origin\\QtWebEngine\\Default\\Local Storage";
    }

    /**
     * Gets the default cache path to the Origin entitlement installers.
     *
     * @return The path to the Origin entitlement installers as a String
     */
    public static String getOriginEntitlementInstallerPath() {
        return "C:\\ProgramData\\Origin\\DownloadCache";
    }

    /**
     * Checks if the given URL returns 4xx, 5xx, or DNS not found error.
     *
     * @param addr HTTP address
     * @return true if the link is broken, false otherwise
     * @throws IOException If an I/O exception occurs
     */
    public static boolean verifyBrokenLink(String addr) throws IOException {
        String httpResponse = getHttpResponse(addr);
        // if broken link is found, then return true
        return "".equals(addr) || httpResponse.startsWith("4") || httpResponse.startsWith("5") || httpResponse.equals("Server DNS not found");
    }

    /**
     * Detecting soft 404 on origin.com with '?_escaped_fragment_' only works on QA env.
     * Gets the HTTP response code (404, 200..) or 'Server DNS not found' from Origin and
     * EA URL. This can handle Origin URL redirection + Origin soft 404 page.
     *
     * @param address HTTP address
     * @return String of the HTTP response code
     * @throws IOException If an I/O exception occurs.
     */
    public static String getHttpResponse(String address) throws IOException {
        URL url;
        URL base;
        URL next;
        HttpURLConnection conn;
        String location;
        String responseCode;
        if (address != null && StringHelper.containsAnyIgnoreCase(address, "ea.com")) {
            CookieHandler.setDefault(new CookieManager(null, CookiePolicy.ACCEPT_ALL));
            url = new URL(address + "?_escaped_fragment_");
            conn = (HttpURLConnection) url.openConnection();
            conn.connect();
            responseCode = String.valueOf(conn.getResponseCode());
        } else {
            while (true) {
                url = new URL(address);
                conn = (HttpURLConnection) url.openConnection();
                conn.setConnectTimeout(15000);
                conn.setReadTimeout(15000);
                conn.setInstanceFollowRedirects(false);
                conn.setRequestMethod("GET");
                try {
                    conn.connect();
                } catch (IOException e) {
                    // when entered wrong domain name
                    return "Server DNS not found";
                }
                // repeat until it gets final redirected URL
                switch (conn.getResponseCode()) {
                    case HttpURLConnection.HTTP_MOVED_PERM:
                        // for 301
                    case HttpURLConnection.HTTP_MOVED_TEMP:
                        // for 302
                    case HttpURLConnection.HTTP_FORBIDDEN:
                        location = conn.getHeaderField("Location");
                        base = new URL(address);
                        next = new URL(base, location);
                        address = next.toExternalForm();
                        continue;
                }
                break;
            }
            responseCode = String.valueOf(conn.getResponseCode());
            // check response code from final EA or Origin's redirected URL
            if (address != null && StringHelper.containsAnyIgnoreCase(address, "www.origin.com")) {
                url = new URL(address + "?_escaped_fragment_");
                conn = (HttpURLConnection) url.openConnection();
                conn.connect();
                responseCode = String.valueOf(conn.getResponseCode());
            }
        }
        _log.debug("Debug: URL " + url + " returning " + responseCode);
        return responseCode;
    }

    /**
     * Gets the HTTP response body from the given connection.
     *
     * @param conn Connection to retrieve the response body
     * @return Response body (JSON string)
     * @throws UnsupportedEncodingException If an encoding exception occurs.
     * @throws IOException If an I/O exception occurs.
     */
    public static String getHttpResponseBody(HttpURLConnection conn) throws UnsupportedEncodingException, IOException {
        StringBuilder sb = new StringBuilder();
        BufferedReader in = null;
        try {
            in = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
            String inputLine;
            while ((inputLine = in.readLine()) != null) {
                sb.append(inputLine);
            }
        } finally {
            if (null != in) {
                in.close();
            }
        }
        return sb.toString();
    }

    /**
     * Gets the site URL from the current URL, which is the address before the
     * first "/" <br>
     * e.g. The siteUrl for
     * https://dev.www.origin.com/can/en-us/store?cmsstage=preview is
     * https://dev.www.origin.com (without any suffix for folder path or
     * parameters)
     *
     * @param driver Selenium WebDriver
     * @return siteURL
     * @throws MalformedURLException If URL is malformed (i.e. missing protocol)
     */
    public static String getCurrentSiteURL(WebDriver driver) throws MalformedURLException {
        URL currentUrl = new URL(driver.getCurrentUrl());
        String siteURL = currentUrl.getProtocol() + "://" + currentUrl.getHost();
        return siteURL;
    }

    /**
     * Kill any and all of the above browsers that are running.
     *
     * @param client {@link OriginClient} The Origin Client instance
     */
    public static void killBrowsers(OriginClient client) {
        final String nodeFullURL = client.getNodeURL();
        if (null != nodeFullURL) {
            ProcessUtil.killProcesses(client, BROWSER_PROCESSES);
        }
    }

    /**
     * Gets the query parameters (after "?") from a query URL
     *
     * @param url Query URL
     * @return The query parameters in a Map
     */
    public static Map<String, List<String>> getQueryParams(String url) {
        try {
            Map<String, List<String>> params = new HashMap();
            String[] urlParts = url.split("\\?");
            if (urlParts.length > 1) {
                String query = urlParts[1];
                for (String param : query.split("&")) {
                    String[] pair = param.split("=");
                    String key = URLDecoder.decode(pair[0], "UTF-8");
                    String value = "";
                    if (pair.length > 1) {
                        value = URLDecoder.decode(pair[1], "UTF-8");
                    }
                    List<String> values = params.get(key);
                    if (values == null) {
                        values = new ArrayList();
                        params.put(key, values);
                    }
                    values.add(value);
                }
            }
            return params;
        } catch (UnsupportedEncodingException ex) {
            throw new AssertionError(ex);
        }
    }

    /**
     * Go to next tab on the browser.
     *
     * @param browserDriver WebDriver for the browser
     */
    public static void gotoNextTab(WebDriver browserDriver) {
        browserDriver.findElement(By.cssSelector("body")).sendKeys(Keys.CONTROL, Keys.TAB);
    }

    /**
     * Gets the query path from a given URL
     *
     * @param url URL to get the query string from
     * @return Query path from a query URL (i.e. path before ? parameters)
     */
    public static String getQueryPath(String url) {
        String[] urlParts = url.split("\\?");
        if (urlParts.length > 0) {
            return urlParts[0];
        }
        return null;
    }

    /**
     * Checks if the date is in valid date format like yyyy-mm-dd.
     *
     * @param date The date to check
     * @return true if the date is in a valid date format, false otherwise
     */
    public static boolean isValidDateFormatWithDash(String date) {
        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");
        dateFormat.setLenient(false);
        try {
            dateFormat.parse(date);
        } catch (ParseException e) {
            return false;
        }
        return true;
    }

    /**
     * A template function to determine if a provided list of values is sorted.
     *
     * @param <T> A type of object that can be compared to others of the same type
     * @param itemsList The list of items to check
     * @param inverseOrder Indicates the list items are in reverse (descending) order
     * @return true if the list is sorted, false otherwise
     */
    public static <T extends Comparable<T>> boolean verifyListSorted(List<T> itemsList, boolean inverseOrder) {
        if (itemsList.isEmpty()) {
            return true;
        }
        List<T> scratchList = new ArrayList<>(itemsList);
        if (inverseOrder) {
            Collections.reverse(scratchList);
        }
        T previous = scratchList.get(0);
        for (T current : scratchList) {
            if (current.compareTo(previous) < 0) {
                _log.info("List not sorted: " + previous + " came before " + current + " in " + scratchList);
                return false;
            }
            previous = current;
        }
        return true;
    }

    /**
     * Checks if two numbers are approximately equal to each other (i.e. their
     * difference if any is within the tolerance).
     *
     * @param num1 First number
     * @param num2 Second number
     * @param tolerance Tolerance to check difference against
     * @return true if abs(num1 - num2) is less than or equal to the tolerance, false otherwise
     */
    public static boolean approxEqual(int num1, int num2, int tolerance) {
        return Math.abs(num1 - num2) <= tolerance;
    }

    /**
     * Checks if two numbers are approximately equal to each other (i.e. their
     * difference if any is within 1).
     *
     * @param num1 First number
     * @param num2 Second number
     * @return true if abs(num1 - num2) is less than or equal to 1, false otherwise
     */
    public static boolean approxEqual(int num1, int num2) {
        final int defaultTolerance = 2;
        return approxEqual(num1, num2, defaultTolerance);
    }

    /**
     * Checks if the date is in valid date format like dd/mm/yy
     *
     * @param date The date to check
     * @return true if the date is in a valid date format, false otherwise
     */
    public static boolean isValidDateFormat(String date) {
        SimpleDateFormat dateFormat = new SimpleDateFormat("dd/MM/yy");
        dateFormat.setLenient(false);
        try {
            dateFormat.parse(date);
        } catch (ParseException e) {
            return false;
        }
        return true;
    }

    /**
     * Returns a date in the past that is the current date but the given amount of years ago.
     *
     * @param yearsOld Number of years in the past
     * @return String in format of date "yyyy/MMMM/dd". For example: "1992/January/05"
     */
    public static String getBirthday(int yearsOld) {
        SimpleDateFormat returnDate = new SimpleDateFormat("yyyy/MMMM/dd");
        Calendar cal = Calendar.getInstance();
        cal.add(Calendar.YEAR, -yearsOld);
        return returnDate.format(cal.getTime());
    }
    
    /**
     * Converts a duration String into seconds. Support 2 formats (white spaces ignored): <br>
     * "HH:mm:ss" - e.g. "0: 2:10" <br>
     * "HH'h'mm'm'ss's'" - e.g. "0h 12m 50s"
     *
     * @param duration Duration as a String in recognized format (white spaces ignored)
     * @return Duration in seconds
     * @throws ParseException If an exception occurs while parsing duration
     */
    public static int getSeconds(String duration) throws ParseException {
        String standardizedDuration = duration.replaceAll("\\s+", "").replace("remaining",""); // remove all white spaces
        if (standardizedDuration.matches("^\\d+h\\d+m\\d+s$")) {
            // Convert from <HH>h<mm>m<ss>s by replacing h/m/s with : and chopping off the trailing :
            standardizedDuration = StringUtils.chop(standardizedDuration.replaceAll("h|m|s", ":"));
        }
        DateFormat dateFormat = new SimpleDateFormat("HH:mm:ss");
        dateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
        try {
            Date date = dateFormat.parse(standardizedDuration);
            long seconds = date.getTime() / 1000L;
            return (int) seconds;
        } catch (ParseException e) {
            _log.error(String.format("Invalid duration: '%s'. Supported formats are <HH>:<mm>:<ss> or HH>h<mm>m<ss>s", duration));
            throw e;
        }
    }

    /**
     * A function to determine if a provided list of values is sorted and ignore the case of the items.
     *
     * @param itemsList The list of items to check
     * @param inverseOrder Indicates the list items are in reverse (descending) order
     * @return true if the list is sorted
     */
    public static boolean verifyListSortedIgnoreCase(List<String> itemsList, boolean inverseOrder) {
        if (itemsList.isEmpty()) {
            return true;
        }
        List<String> scratchList = new ArrayList<>(itemsList);
        if (inverseOrder) {
            Collections.reverse(scratchList);
        }
        String previous = scratchList.get(0);
        for (String current : scratchList) {
            if (current.compareToIgnoreCase(previous) < 0) {
                _log.info("List not sorted: " + previous + " came before " + current + " in " + scratchList);
                return false;
            }
            previous = current;
        }
        return true;
    }

    /**
     * Verify if one of the above browsers are running.
     *
     * @param client {@link OriginClient} The Origin Client instance
     * @return true if at least one of the above browsers are running, false otherwise.
     * @throws IOException If an I/O exception occurs.
     * @throws InterruptedException If a thread is interrupted while waiting/sleeping
     */
    public static boolean verifyBrowserOpen(OriginClient client) throws IOException, InterruptedException {
        final String nodeFullURL = client.getNodeURL();
        if (null != nodeFullURL) {
            return ProcessUtil.isAnyProcessRunning(client, BROWSER_PROCESSES);
        }
        return false;
    }

    /**
     * Verify that the WebElement's in a List with matching text Strings exist.
     *
     * @param elements List of WebElements to search
     * @param textStrings List of text strings to match
     * @param exist An array of expected existence (true for existence, false for non-existence)
     * @return true if the list of Strings all match an element in the list, and all
     * elements match the existence statuses. Return false otherwise.
     */
    public static boolean verifyElementsExistStatuses(List<WebElement> elements, List<String> textStrings, boolean[] exist) {
        if (exist.length <= 0 || textStrings.size() <= 0 || exist.length != textStrings.size()) {
            _log.error("No text strings to match or expected statuses list is of different length");
            return false;
        }
        List<String> elementTextStrings = OASiteTemplate.getTextStrings(elements);
        for (int i = 0; i < textStrings.size(); i++) {
            if (StringHelper.containsIgnoreCase(elementTextStrings, textStrings.get(i)) != exist[i]) {
                _log.debug("Matching " + (exist[i] ? "" : "non-") + "existence of '" + textStrings.get(i) + "' in '" + elementTextStrings + "' yields negative result");
                return false;
            }
        }
        return true;
    }
    
    /**
     * Enter javascript command
     *
     * @param driver  webdriver
     * @param value javascript. If it's 0 the download rate is set "No limit".
     * @return return String if exists
     */
    public static String setMaxDownloadRateBpsOutOfGame(WebDriver driver, String value){
        return ((JavascriptExecutor) driver).executeScript("return Origin.client.settings.writeSetting(\"MaxDownloadRateBpsOutOfGame\", \"" + value + "\");").toString();
    }

    /**
     *  Verifies that the price has the origin access discount applied
     * @param originalPrice the original or non-sub price of the item
     * @param accessPrice the subscriber price of the item
     * @return true if the correct access discount value is applied, false otherwise
     */
    public static boolean verifyOriginAccessDiscountApplied(float originalPrice, float accessPrice){
        float discount = Math.abs(accessPrice/originalPrice - 1);
        DecimalFormat df = new DecimalFormat("#.##");
        return Float.parseFloat(df.format(discount)) == ACCESS_DISCOUNT_VALUE;
    }
    
    /**
     * Set cloudsave setting enabled using javascript command
     *
     * @param driver  webdriver
     * @param value If it's true the cloud save is enabled. False otherwise
     * @return return String if exists
     */
    public static String setCloudSaveEnabled(WebDriver driver, boolean value){
        return ((JavascriptExecutor) driver).executeScript("return Origin.client.settings.writeSetting(\"cloud_saves_enabled\", \"" + value + "\");").toString();
    }

    /**
     * Set IOG/OIG setting enabled using javascript command
     *
     * @param driver  webdriver
     * @param value If it's true IGO/OIG is enabled. False otherwise
     * @return return String if exists
     */
    public static String setIGOEnabled(WebDriver driver, boolean value){
        return ((JavascriptExecutor) driver).executeScript("return Origin.client.settings.writeSetting(\"EnableIgo\", \"" + value + "\");").toString();
    }

    /**
     * Enter path using robot keyboard on 'Open Directory' dialog to change game install location
     * @param client {@link OriginClient} The Origin Client instance
     * @param path new path to change game install location
     * @return true if no error occurs while entering new path
     */
    public static boolean enterPathOnOpenDirectoryDialogUsingRobotKeyboard(OriginClient client, String path) {
        boolean errorDuringRobotKeyBoard = false;
        try {
            RobotKeyboard robotHandler = new RobotKeyboard();
            robotHandler.type(client, path, 200);
            Waits.sleepMinutes(1);
            robotHandler.type(client, "\n\n", 2000);
        } catch (Exception e) {
            errorDuringRobotKeyBoard = true;
        }
        return errorDuringRobotKeyBoard;
    }
}
