package com.ea.originx.automation.lib.helpers;

import java.lang.invoke.MethodHandles;
import java.text.DecimalFormat;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * A collection of helpers that help you easily format Strings, parse Strings, and more.
 */
public class StringHelper {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());
    private static final List<Integer> RANDOM_CHARACTER_BLACKLIST = Arrays.asList(127, 129, 141, 143, 144, 157, 160);

    /**
     * Determines if a given String is null or empty.
     *
     * @param stringToCheck The String to check
     * @return true if the given String is null or empty, false otherwise
     */
    public static boolean nullOrEmpty(String stringToCheck) {
        return stringToCheck == null || stringToCheck.isEmpty();
    }

    /**
     * Parses a number from the provided string.
     *
     * @param num A String in which to parse the number from
     * @return The number from the provided String as a String
     */
    public static String normalizeNumberString(String num) {
        if (num == null || num.isEmpty()) {
            return "0";
        }
        // replace all characters that are not a digit, period, or comma
        String normalizedNum = num.replaceAll("[^\\d\\.,]", "");
        if (normalizedNum.isEmpty()) {
            return "0";
        }
        if (normalizedNum.endsWith(".")) {
            normalizedNum = normalizedNum.substring(0, normalizedNum.length() - 1);
        }
        // remove all dots that have at least 3 digits after them because some locales use dots instead of commas to delineate thousands
        normalizedNum = normalizedNum.replaceAll("\\.(?=\\d{3})", "");
        // if the hundredths is delineated by a comma instead of a period, replace it with a period
        if (normalizedNum.length() > 2 && normalizedNum.substring(normalizedNum.length() - 3).startsWith(",")) {
            normalizedNum = normalizedNum.substring(0, normalizedNum.length() - 3) + "." + normalizedNum.substring(normalizedNum.length() - 2);
        }
        // remove any left over chars that aren't digits or a period
        normalizedNum = normalizedNum.replaceAll("[^\\d\\.]", "");

        // if first character is a period from currency type, remove it (example: Rs. 8,402 -> .8,402 -> 8,402)
        if (normalizedNum.charAt(0) == '.') {
            StringBuilder sb = new StringBuilder(normalizedNum);
            sb.deleteCharAt(0);
            normalizedNum = sb.toString();
        }

        return normalizedNum;
    }

    /**
     * Removes special characters from the provided String. Only keeps
     * letters, numbers, and spaces.
     *
     * @param stringToCheck The String to check
     * @return The parsed String
     */
    public static String convertToAlphaNumeric(String stringToCheck) {
        String c = stringToCheck;
        Pattern pattern = Pattern.compile("[^a-zA-Z0-9 ]");
        Matcher match = pattern.matcher(stringToCheck);
        while (match.find()) {
            String s = match.group();
            c = c.replaceAll("\\"+s, "");
        }
        return c;
    }

    /**
     * Checks if a given String contains any of the Strings in the given containeeList (case ignored).
     *
     * @param container The String to check against
     * @param containeeList The list of Strings to be checked against the container
     * @return true if any element in the containeeList is contained in the container (case ignored)
     */
    public static boolean containsAnyIgnoreCase(String container, String... containeeList) {
        _log.debug("container: " + container + ", containeeList: " + Arrays.toString(containeeList));
        for (String containee : containeeList) {
            if (containsIgnoreCase(container, containee)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Checks if a given String matches a regular expression (case ignored).
     *
     * @param string String to match the RegEx to
     * @param regex RegEx to match (will first be converted to a case insensitive RegEx pattern)
     * @return true if the String matches RegEx (case ignored), false otherwise
     */
    public static boolean matchesIgnoreCase(String string, String regex) {
        _log.debug("String: " + string + ", regular expression: " + regex);
        Pattern caseInsensitiveRegex = Pattern.compile(regex, Pattern.CASE_INSENSITIVE);
        Matcher matcher = caseInsensitiveRegex.matcher(string);
        return matcher.find();
    }

    /**
     * Checks if a set of Strings are contained within another String, ignoring
     * the case of the Strings. The set can be only one item. A String is
     * considered 'contained' in the container if it is a substring of the container.
     *
     * @param container The String to check inside
     * @param containees List of Strings to check if they are substrings of the container
     * @return true if the String has all containees as a substring (case ignored), false otherwise
     */
    public static boolean containsIgnoreCase(String container, String... containees) {
        boolean noneMissing = true;
        if (container == null || container.isEmpty()) {
            return false;
        }
        for (String containee : containees) {
            noneMissing &= StringUtils.containsIgnoreCase(container, containee);
        }
        return noneMissing;
    }

    /**
     * Checks if a String is 'contained' in a list of Strings (case ignored).
     * A String is considered 'contained' in a list if the String is equal
     * to (case ignored) one of the Strings in the list.
     *
     * @param containerList The list to check against
     * @param containee The String to be checked against the list for containment
     * @return true if the containee is equal to (case ignored) an element in
     * the containerList, false otherwise
     */
    public static boolean containsIgnoreCase(List<String> containerList, String containee) {
        for (String container : containerList) {
            if (StringUtils.equalsIgnoreCase(container, containee)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Checks if Strings in a list are 'contained' in another list of Strings
     * (case ignored). A String is considered 'contained' in a list if the
     * String is equal to (case ignored) one of the Strings in the list.
     *
     * @param containerList The list of Strings to check against
     * @param containeeList The list of String to be checked against the containerList for containment
     * @return true if every element in the containeeList is equal to (case ignored) an element in the
     * containerList, false otherwise
     */
    public static boolean containsIgnoreCase(List<String> containerList, List<String> containeeList) {
        _log.debug("containerList: " + containerList.toString() + ", containeeList: " + containeeList.toString());
        for (String containee : containeeList) {
            if (!containsIgnoreCase(containerList, containee)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Replaces a character in given text with another character at a specified index.
     *
     * @param text The text to be replaced with the new character
     * @param position Position/index of the character
     * @param c The character to replace the character at the specified position in text
     * @return String with the replaced char
     */
    public static String replaceCharAt(String text, int position, char c) {
        return text.substring(0, position) + c + text.substring(position + 1);
    }

    /**
     * Gets the first matching RegEx of the given pattern.
     *
     * @param text The text to search in
     * @param pattern The RegEx pattern to use for the search
     * @return The first instance of a String that matches the given pattern
     */
    public static String getFirstRegexMatch(String text, String pattern) {
        String firstMatch = null;
        Pattern thePattern = Pattern.compile(pattern, Pattern.MULTILINE);
        Matcher m = thePattern.matcher(text);
        if (m.find()) {
            firstMatch = m.group(0);
        }
        return firstMatch;
    }

    /**
     * Extract a double from a String.
     * For example, returns 10.00 from 'Buy $10.00' String
     *
     * @param text The string to extract a double from
     * @return Extracted double from the given text
     */
    public static double extractNumberFromText(String text) {
        return Double.parseDouble(text.replaceAll("[^\\d.]", ""));
    }

    /**
     * Remove non-digits from a given String.
     * For example, returns "12345" from "Security Code: 12345" String
     *
     * @param text The String to remove non-digits from.
     * @return Integer only from text
     */
    public static String removeNonDigits(String text) {
        return text.replaceAll("[^\\d+]", "");
    }
    
    /**
     * Remove high-ASCII characters from a given String.
     * For example, returns "Titanfall 2" from "Titanfall™ 2" String
     *
     * @param text The String to remove ASCII from.
     * @return String only from text
     */
    public static String removeHighASCIICharacters(String text) {
        return text.replaceAll("[^\\x00-\\x7f]", "");
    }

    /**
     * Generates a String of random characters of the given length. Prevents
     * special characters from being entered.
     *
     * @param length Length of the String to generate
     * @return A String of random characters
     */
    public static String generateRandomString(int length) {
        StringBuilder temp = new StringBuilder();
        Random rand = new Random();
        int nextRand;
        for (int i = 0; i < length; i++) {
            nextRand = (rand.nextInt(223) + 32); // Skip first 31 ASCII characters
            if (!RANDOM_CHARACTER_BLACKLIST.contains(nextRand)) {
                temp.append((char) nextRand);
            } else {
                i--; // Try again.
            }
        }
        return temp.toString();
    }

    /**
     * Formats a double to a String to 2 decimal places.
     *
     * @param price Double to convert into a String that is 2 decimal places
     * @return Price as a String that is rounded to 2 decimal places
     */
    public static String formatDoubleToPriceAsString(double price) {
        DecimalFormat df = new DecimalFormat("#.##");
        return df.format(price);
    }

    /**
     * Remove curly brackets and single quotes from a given String.
     *
     * @param data The String to remove curly brackets and single quotes from
     * @return The parsed String
     */
    public static String normalizeSetString(String data) {
        return data.replaceAll("\\{", "").replaceAll("}", "").replaceAll("'", "");
    }
    
}