package com.ea.originx.automation.lib.pageobjects.account;

import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;

/**
 * Represent the 'Order History' page ('Account Settings' page with the 'Order
 * History' section open)
 *
 * @author palui
 */
public class OrderHistoryPage extends AccountSettingsPage {

    protected final String ORDER_LINES_CSS = ".orderlist .orderdetail";
    protected final String ORDER_LINE_CSS_TMPL = ORDER_LINES_CSS + ":nth-of-type(%s)";
    protected final By ORDER_LINES_LOCATOR = By.cssSelector(ORDER_LINES_CSS);

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public OrderHistoryPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Order History' section of the 'Account Settings' page is open
     *
     * @return true if open, false otherwise
     */
    public boolean verifyOrderHistorySectionReached() {
        return verifySectionReached(NavLinkType.ORDER_HISTORY);
    }

    /**
     * Get order lines as WebElement's
     *
     * @return List of order lines as WebElement's, or null if not found
     */
    private List<WebElement> getOrderLineElements() {
        try {
            return waitForAllElementsVisible(ORDER_LINES_LOCATOR);
        } catch (TimeoutException e) {
            _log.warn("No order history entries found");
            return new ArrayList<WebElement>();
        }
    }

    /**
     * Count number of order lines in the 'Order History' page
     *
     * @return number of order lines
     */
    public int countOrders() {
        return getOrderLineElements().size();
    }

    /**
     * Get the i-th order line (starting from 1) on the 'Order History' page
     *
     * @param index index of the order line
     * @return {@link OrderHistoryLine} the order line
     */
    public OrderHistoryLine getOrderHistoryLine(int index) {
        int nthOfType = 2 * index - 1; // 2n+1 rows with order lines in odd rows
        return new OrderHistoryLine(driver, String.format(ORDER_LINE_CSS_TMPL, nthOfType));
    }

    /**
     * Get order lines
     *
     * @return List of order lines, or null if not found
     */
    public List<OrderHistoryLine> getOrderHistoryLines() {
        List<OrderHistoryLine> orderHistoryLines = new ArrayList<>();
        for (int i = 1; i <= countOrders(); i++) {
            orderHistoryLines.add(getOrderHistoryLine(i));
        }
        return orderHistoryLines;
    }
}
