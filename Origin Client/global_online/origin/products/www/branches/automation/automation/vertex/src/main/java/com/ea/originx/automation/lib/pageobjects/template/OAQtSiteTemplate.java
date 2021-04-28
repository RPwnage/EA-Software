package com.ea.originx.automation.lib.pageobjects.template;

import com.ea.vx.originclient.client.OriginClient;
import org.openqa.selenium.WebDriver;

/**
 * The site template for Qt page objects.
 *
 * @author lscholte
 */
public abstract class OAQtSiteTemplate extends EAXVxSiteTemplate {

    /**
     * Constructor
     *
     * @param driver The WebDriver from which the QtWebDriver will be obtained
     */
    protected OAQtSiteTemplate(WebDriver driver) {
        super(OriginClient.getInstance(driver).getQtWebDriver());
    }

    /**
     * Checks to see if window with given URL is present.
     *
     * @param url The URL to look for.
     * @return true if window with given URL is present, false otherwise.
     */
    public static boolean isWindowPresent(WebDriver driver, String url) {
        return OriginClient
                .getInstance(driver)
                .getQtWebDriver()
                .getWindowHandles()
                .stream()
                .anyMatch(h -> h.equals(url));
    }
}