package com.ea.originx.automation.scripts.feature.redeem;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroRedeem;
import com.ea.originx.automation.lib.macroaction.MacroSettings;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.LanguageInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import java.util.Locale;

/**
 * Parametrized test to check that redeeming an invalid code does not work
 *
 * @author jdickens
 */
public class OARedeemInvalidCode extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        ALREADY_USED,
        INVALID
    }

    public void testRedeemInvalidCode(ITestContext context, TEST_TYPE type) throws Exception {
        OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String userName = userAccount.getUsername();
        String notRedeemableCodeDescription;
        switch (type) {
            case ALREADY_USED:
                notRedeemableCodeDescription = "already used";
                break;
            case INVALID:
                notRedeemableCodeDescription = "invalid";
                break;
            default:
                throw new RuntimeException("Unexpected test type " + type);
        }

        logFlowPoint("Launch Origin and login with a newly created user"); // 1
        logFlowPoint("Click the 'Main Menu' and select 'Redeem Product Code'"); // 2
        logFlowPoint("Enter a code that has already been redeemed, click 'Next' "
                + "and verify that an error message is displayed which states that the inputted code is " + notRedeemableCodeDescription); // 3
        logFlowPoint("Go to the 'Game Library' and verify that no games have been redeemed"); // 4
        logFlowPoint("Restart the client in German and log back in"); // 5
        logFlowPoint("Verify that the client's language has been changed to German"); // 6
        logFlowPoint("Click the 'Main Menu' and select 'Redeem Product Code'"); // 7
        logFlowPoint("Enter a code that has already been redeemed, click 'Next' "
                + "and verify that an error message is displayed in German which states that the inputted code is " + notRedeemableCodeDescription); // 8
        logFlowPoint("Go to the 'Game Library' and verify that no games have been redeemed"); // 9

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new MainMenu(driver).selectRedeemProductCode();
        RedeemDialog redeemDialog = new RedeemDialog(driver);
        redeemDialog.waitAndSwitchToRedeemProductDialog();
        logPassFail(redeemDialog.waitIsVisible(), true);

        // 3
        boolean isCodeNotRedeemable = type == TEST_TYPE.ALREADY_USED ? MacroRedeem.verifyRedeemCodeAlreadyUsed(driver) : MacroRedeem.verifyRedeemCodeInvalid(driver);
        logPassFail(isCodeNotRedeemable, false);

        // 4
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageEmpty(), false);

        // 5
        MacroSettings.setLanguage(driver, client, LanguageInfo.LanguageEnum.GERMAN);
        client.stop();
        ProcessUtil.killOriginProcess(client);
        driver = startClientObject(context, client);
        I18NUtil.setLocale(new Locale(LanguageInfo.LanguageEnum.GERMAN.getLanguageCode(), CountryInfo.CountryEnum.GERMANY.getCountryCode()));
        I18NUtil.registerResourceBundle("i18n.MessagesBundle");
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 6
        boolean isLanguageCorrect = new NavigationSidebar(driver).verifyGameLibraryString();
        logPassFail(isLanguageCorrect, false);

        // 7
        new MainMenu(driver).selectRedeemProductCode();
        RedeemDialog germanRedeemDialog = new RedeemDialog(driver);
        germanRedeemDialog.waitAndSwitchToRedeemProductDialog();
        logPassFail(germanRedeemDialog.waitIsVisible(), true);

        // 8
        isCodeNotRedeemable = type == TEST_TYPE.ALREADY_USED ? MacroRedeem.verifyRedeemCodeAlreadyUsed(driver) : MacroRedeem.verifyRedeemCodeInvalid(driver);
        logPassFail(isCodeNotRedeemable, false);

        // 9
        new NavigationSidebar(driver).gotoGameLibrary();
        gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageEmpty(), false);

        softAssertAll();
    }
}
