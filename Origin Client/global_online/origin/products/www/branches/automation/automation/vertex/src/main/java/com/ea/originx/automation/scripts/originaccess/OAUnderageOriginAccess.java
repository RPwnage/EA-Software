package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.CountryInfo;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test underage users from all regions cannot see the store and the EADP
 * account pages
 *
 * @author mdobre
 */
public class OAUnderageOriginAccess extends EAXVxTestTemplate {

    @TestRail(caseId = 11062)
    @Test(groups = {"client_only","originaccess", "full_regression"})
    public void testUnderageOriginAccess(ITestContext context) throws Exception {

        List<UserAccount> underAgeUserAccounts = new ArrayList<UserAccount>();

        for (CountryInfo.CountryEnum country : CountryInfo.CountryEnum.values()) {
            logFlowPoint("Log into Origin with account from " + country.getCountry() +" and verify the user is not able to see the store and the EADP account pages in the Origin menu."); 
        }
        final OriginClient client = OriginClientFactory.create(context);
        String parentEmail = AccountManager.getRandomAccount().getEmail();

        //create underage accounts for every country
        for (CountryInfo.CountryEnum country : CountryInfo.CountryEnum.values()) {
            underAgeUserAccounts.add(AccountManagerHelper.registerNewUnderAgeWithCountryThrowAwayAccountThroughREST(10, country.getCountry(), parentEmail));
        }

        final WebDriver driver = startClientObject(context, client);

        for (int i = 0; i < underAgeUserAccounts.size(); i++) {
            MacroLogin.startLogin(driver, underAgeUserAccounts.get(i));
            MainMenu mainMenu = new MainMenu(driver);
            boolean isStoreNotVisible = !mainMenu.verifyItemExistsInView("Store");
            boolean isAccountBillingNotInOriginMenu = !mainMenu.verifyItemExistsInOrigin("EA Account and Billing");
            boolean isOrderHistoryNotInMenu = !mainMenu.verifyItemExistsInOrigin("Order History");
            logPassFail(isStoreNotVisible && isAccountBillingNotInOriginMenu && isOrderHistoryNotInMenu, false);
            mainMenu.selectLogOut(true);
        }
        softAssertAll();
    }
}
