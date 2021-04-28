package com.ea.originx.automation.lib.helpers;

import java.text.MessageFormat;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.Set;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * Utility class providing methods to access the Locale of the current thread and to get
 * localized strings.
 * <br>
 * Typically used to set a locale for the thread the script is running in the script using the setLocale function.
 * After setting the locale, a resource bundle should be registered using the registerResourceBundle function.
 * <br>
 * This class reads the resource bundles in the resources/i18n folder.
 *
 * @author caleung
 */
public class I18NUtil {

    /**
     * Thread-local containing the general locale for the current thread.
     */
    private static ThreadLocal<Locale> threadLocale = new ThreadLocal<>();

    /**
     * List of registered bundles.
     */
    private static Set<String> resourceBundleBaseNames = new HashSet<>();

    /**
     * Map of loaded bundles by locale.
     */
    private static Map<Locale, Set<String>> loadedResourceBundles = new HashMap<>();

    /**
     * Map of cached messaged by locale.
     */
    private static Map<Locale, Map<String, String>> cachedMessages = new HashMap<>();

    /**
     * Lock objects.
     */
    private static ReadWriteLock lock = new ReentrantReadWriteLock();
    private static Lock readLock = lock.readLock();
    private static Lock writeLock = lock.writeLock();

    /**
     * Set the locale for the current thread.
     *
     * @param locale The locale to set
     */
    public static void setLocale(Locale locale)
    {
        threadLocale.set(locale);
    }

    /**
     * Get the general locale for the current thread and will revert to the default locale if none
     * were specified for this thread.
     *
     * @return The general locale of the current thread
     */
    public static Locale getLocale() {
        Locale locale = threadLocale.get();
        if (locale == null)
        {
            // Get the default locale
            locale = Locale.getDefault();
        }
        return locale;
    }

    /**
     * Register a resource bundle.
     * <p>
     * This should be the bundle base name.
     * <p>
     * Once registered the messages will be available using the getMessage method.
     *
     * @param bundleBaseName The bundle base name (usually 'i18n.MessagesBundle')
     */
    public static void registerResourceBundle(String bundleBaseName) {
        try {
            writeLock.lock();
            resourceBundleBaseNames.add(bundleBaseName);
        }
        finally {
            writeLock.unlock();
        }
    }

    /**
     * Get message from registered resource bundle.
     *
     * @param messageKey The message key
     * @return The localized message String, null if none found
     */
    public static String getMessage(String messageKey) {
        return getMessage(messageKey, getLocale());
    }

    /**
     * Get a localized message String.
     *
     * @param messageKey The message key to look for
     * @param locale The locale to override to
     * @return The localized message String, null if none found
     */
    public static String getMessage(String messageKey, Locale locale) {
        String message = null;
        Map<String, String> props = getLocaleProperties(locale);
        if (props != null) {
            message = props.get(messageKey);
        } else {
            setLocale(Locale.getDefault());
            registerResourceBundle("i18n.MessagesBundle");
            props = getLocaleProperties(locale);
            message = props.get(messageKey);
        }
        return message==null? messageKey:message;
    }

    /**
     * Get a localized message String, parameterized using standard MessageFormatter.
     *
     * @param messageKey The message key
     * @param params Format parameters
     * @return The localized string, null if none found
     */
    public static String getMessage(String messageKey, Object ... params) {
        return getMessage(messageKey, getLocale(), params);
    }

    /**
     * Get a localized message String, parameterized using standard MessageFormatter.
     *
     * @param messageKey The message key
     * @param locale The locale to override current locale
     * @param params The localized message String
     * @return The localized String, null if none found
     */
    public static String getMessage(String messageKey, Locale locale, Object ... params) {
        String message = getMessage(messageKey, locale);
        if (message != null && params != null) {
            message = MessageFormat.format(message, params);
        }
        return message;
    }

    /**
     * Get the messages for a locale.
     * <p>
     * Will use cache where available otherwise will load into cache from bundles.
     *
     * @param locale The locale
     * @return The message map
     */
    private static Map<String, String> getLocaleProperties(Locale locale) {
        Set<String> loadedBundles = null;
        Map<String, String> props = null;
        int loadedBundleCount = 0;
        try {
            readLock.lock();
            loadedBundles = loadedResourceBundles.get(locale);
            props = cachedMessages.get(locale);
            loadedBundleCount = resourceBundleBaseNames.size();
        }
        finally {
            readLock.unlock();
        }

        if (loadedBundles == null) {
            try {
                writeLock.lock();
                loadedBundles = new HashSet<>();
                loadedResourceBundles.put(locale, loadedBundles);
            }
            finally {
                writeLock.unlock();
            }
        }

        if (props == null) {
            try {
                writeLock.lock();
                props = new HashMap<>();
                cachedMessages.put(locale, props);
            }
            finally {
                writeLock.unlock();
            }
        }

        if (loadedBundles.size() != loadedBundleCount) {
            try {
                writeLock.lock();
                for (String resourceBundleBaseName : resourceBundleBaseNames) {
                    if (loadedBundles.contains(resourceBundleBaseName) == false) {
                        ResourceBundle resourcebundle = ResourceBundle.getBundle(resourceBundleBaseName, locale);
                        Enumeration<String> enumKeys = resourcebundle.getKeys();
                        while (enumKeys.hasMoreElements() == true) {
                            String key = enumKeys.nextElement();
                            props.put(key, resourcebundle.getString(key));
                        }
                        loadedBundles.add(resourceBundleBaseName);
                    }
                }
            }
            finally {
                writeLock.unlock();
            }
        }

        if (props.size() == 0) { // if no props found, return null
            return null;
        }

        return props;
    }
}