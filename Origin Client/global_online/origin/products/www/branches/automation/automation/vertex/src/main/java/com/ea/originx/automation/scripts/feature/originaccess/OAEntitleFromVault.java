package com.ea.originx.automation.scripts.feature.originaccess;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Purchase an Origin Access subscription and Entitle something from the Vault
 *
 * @author glivingstone
 * @author rocky
 * @author mdobre
 */
public class OAEntitleFromVault extends EAXVxTestTemplate {

    public enum TEST_ACTION {
        ADD,
        REMOVE
    }

    public enum TEST_PAGE {

        VAULT_PAGE,
        GDP_PAGE
    }

    public void testEntitleFromVault(ITestContext context, TEST_ACTION action, TEST_PAGE page) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        final String entOfferID = entitlement.getOfferId();

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        String pageName = "";

        switch (page) {
            case VAULT_PAGE:
                pageName = "vault page"; //3
                break;
            case GDP_PAGE:
                pageName = " it's GDP page"; //3
                break;
            default:
                throw new RuntimeException("Unexpected test page " + page);
        }

        logFlowPoint("Login as newly registered account"); //1
        logFlowPoint("Purchase Origin Access"); //2
        logFlowPoint("Add a vault entitlement to the game library through " + pageName); //3
        logFlowPoint("Navigate to game library page and verify the Entitlement Added Earlier is in the Game Library"); //4
        if (action == TEST_ACTION.REMOVE) {
            logFlowPoint("Remove the Entitlement from Game Library and verify the entitlement does not exist anymore"); //5
        }

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            logPassFail(MacroRedeem.redeemOABasicSubscription(driver, userAccount.getEmail()), true );
        } else {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }

        // 3
        VaultPage vaultPage = new VaultPage(driver);
        vaultPage.verifyPageReached();
        vaultPage.clickEntitlementByOfferID(entitlement.getOfferId());
        GDPHeader gdpHeader = new GDPHeader(driver);
        gdpHeader.verifyGDPHeaderReached();
        logPassFail(MacroGDP.addVaultGameAndCheckViewInLibraryCTAPresent(driver), true);

        // 4
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile vaultGame = new GameTile(driver, entOfferID);
        logPassFail(vaultGame.verifyOfferID(), true);

        // 5
        if (action == TEST_ACTION.REMOVE) {
            logPassFail(MacroGameLibrary.removeFromLibrary(driver, entOfferID), true);
        }
        
        softAssertAll();
    }
}