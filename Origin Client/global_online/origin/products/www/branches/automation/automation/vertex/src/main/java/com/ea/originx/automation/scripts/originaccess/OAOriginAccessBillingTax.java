package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.macroaction.MacroSettings;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.LanguageInfo;
import com.ea.vx.originclient.utils.AccountService;
import com.ea.vx.originclient.utils.PaymentService;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import java.util.ArrayList;
import java.util.Locale;

/**
 * Testing if tax is calculated when purchasing Origin Access
 *
 * @author ivlim
 */
public class OAOriginAccessBillingTax extends EAXVxTestTemplate {

    public void testOriginAccessBillingTax(ITestContext context, CountryInfo.CountryEnum countryEnum, LanguageInfo.LanguageEnum languageEnum) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        WebDriver driver;

        // Set country and locale
        String countryName = countryEnum.getCountry();
        String countryCode = countryEnum.getCountryCode();
        String countryCodeX = countryEnum.getCountryCodeX();
        String languageCode = languageEnum.getLanguageCode();
        String languageCodeX = languageEnum.getCode();

        // Create user
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(countryName);
        final String username = userAccount.getUsername();

        // Change country and language
        if (isClient) {
            EACoreHelper.overrideCountryTo(countryEnum, client.getEACore());
            driver = startClientObject(context, client);
            MacroLogin.startLogin(driver, userAccount);
            MacroSettings.setLanguage(driver, client, languageEnum);
            Waits.sleep(10000);
            client.stop();
            ProcessUtil.killOriginProcess(client);
        }

        I18NUtil.setLocale(new Locale(languageCode, countryCode));
        I18NUtil.registerResourceBundle("i18n.MessagesBundle");

        logFlowPoint("Login as: " + username); //1
        logFlowPoint("Navigate to the 'Origin Access' page"); //2
        logFlowPoint("Click 'Join' on the Origin Access Page"); //3
        logFlowPoint("Complete the subscription workflow in the countries"); //4
        logFlowPoint("Pull order information using GET invoice information call"); //5
        logFlowPoint("Verify tax rate and calculation is valid based on country"); //6

        //1
        driver = startClientObject(context, client, "", String.format("/%s/%s", countryCodeX, languageCodeX));
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        OriginAccessPage accessPage = new NavigationSidebar(driver).gotoOriginAccessPage();
        logPassFail(accessPage.verifyPageReached(), true);

        //3
        accessPage.clickJoinPremierButton();
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        selectOriginAccessPlanDialog.waitForYearlyPlanAndStartTrialButtonToLoad();
        logPassFail(selectOriginAccessPlanDialog.isDialogVisible(), true);

        //4
        selectOriginAccessPlanDialog.clickNext();
        MacroPurchase.completePurchase(driver);
        OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
        logPassFail(originAccessNUXFullPage.verifyPageReached(), true);

        //5
        final WebDriver browserDriver;
        if (isClient) {
            browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            MacroAccount.loginToAccountPage(browserDriver, userAccount);
        } else {
            originAccessNUXFullPage.clickSkipNuxGoToVaultLink();
            browserDriver = driver;
            new MiniProfile(driver).selectProfileDropdownMenuItemByHref("eaAccounts");
            driver.switchTo().window(new ArrayList<>(driver.getWindowHandles()).get(1)); // switching to newly opened tab
        }
        String orderNumber = MacroAccount.getInvoiceNumber(browserDriver, 1);
        String invoice = PaymentService.getInvoiceInformation(orderNumber);
        logPassFail(invoice.contains("invoiceUuid"), true);

        //6
        logPassFail(MacroPurchase.verifyInvoiceTaxOriginAccess(invoice, countryEnum), true);

        softAssertAll();
    }

}
