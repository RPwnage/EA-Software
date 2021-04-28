package com.ea.originx.automation.scripts.cloudsave;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSettings;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.NewSaveDataFoundDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.CloudSaveService;
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.utils.TestCleaner;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.utils.FileUtil;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import java.util.ArrayList;
import java.util.List;

/**
 * Tests 'New cloud save data' dialog when user has a local save file
 * and tries to save/delete/cancel the local saves through the dialog
 *
 * @author nvarthakavi
 */
public class OACloudSave extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        SAVE,
        DELETE,
        CANCEL
    }

    private OriginClient client;
    private String cloudSaveEntitlementLocalSavePath;

    public void testCloudSave(ITestContext context, TEST_TYPE type) throws Exception {

        client = OriginClientFactory.create(context);

        final EntitlementInfo cloudSaveEntitlement = EntitlementInfoHelper
                .getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final UserAccount account = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final String accountName = account.getUsername();
        final String cloudSaveEntitlementOfferId = cloudSaveEntitlement.getOfferId();
        cloudSaveEntitlementLocalSavePath = client.getAgentOSInfo()
                .getEnvValue("USERPROFILE") + "\\Documents\\My Games\\DipSmall\\";

        //clear the local app data
        new TestCleaner(client).clearLocalSettings(true);

        WebDriver driver = startClientObject(context, client);

        final String cloudSaveFileNameA = "CloudSaveTestFileA.sav";

        final String extraText;
        switch (type) {
            case SAVE:
                extraText = "save local data button and verify the local data is pushed to cloud";
                break;
            case DELETE:
                extraText = "delete button and verify cloud save file is removed and the entitlement is launched";
                break;
            case CANCEL:
                extraText = "cancel button and verify the dialog is closed and the entitlement does not launch";
                break;
            default:
                throw new IllegalArgumentException("Unexpected test type " + type);
        }

        logFlowPoint("Grant a newly registered account with 'DiP Small' entitlement"); //1
        logFlowPoint("Launch Origin and login with the newly registered account: " + accountName); //2
        logFlowPoint("Download and install the granted entitlement after enabling cloud save setting"); //3
        logFlowPoint("Add random file to local cloud save path " +
                "and verify the 'New save data found' dialog shows up after launching the entitlement"); //4
        logFlowPoint("Verify 'Delete' and 'Save' buttons on the dialog"); //5
        logFlowPoint("Click " + extraText); //6

        //1
        final String entitlementId = EntitlementService.grantEntitlement(account, cloudSaveEntitlementOfferId);
        logPassFail(entitlementId != null && !entitlementId.isEmpty(), true);

        //2
        logPassFail(MacroLogin.startLogin(driver, account), true);

        //3
        MacroSettings.changeCloudSaveSettings(driver, true);
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, cloudSaveEntitlementOfferId), true);

        //4
        new DownloadManager(driver).closeDownloadQueueFlyout();
        SystemUtilities.createFolderandFile(client, cloudSaveEntitlementLocalSavePath, cloudSaveFileNameA);
        GameTile gameTile = new GameTile(driver, cloudSaveEntitlementOfferId);
        gameTile.play();
        NewSaveDataFoundDialog newSaveDataFoundDialog = new NewSaveDataFoundDialog(driver);
        logPassFail(Waits.pollingWait(newSaveDataFoundDialog::waitIsVisible), true);

        //5
        logPassFail(newSaveDataFoundDialog.verifyDeleteAndSaveButtonsExist(), true);

        //6
        // no data on cloud initially
        boolean hasCloudData = CloudSaveService.verifyCloudSaveExists(account, cloudSaveEntitlement);
        if (hasCloudData) {
            log("Cloud data is not supposed to be existing at this point");
        }
        boolean isSaveDeleteCancel = !hasCloudData;
        if (type == TEST_TYPE.SAVE) {
            newSaveDataFoundDialog.clickSaveLocalDataButton();
            boolean isEntitlementLaunched = Waits.pollingWaitEx(() -> cloudSaveEntitlement.isLaunched(client));
            if (!isEntitlementLaunched) {
                log("Entitlement is not launched");
            }
            isSaveDeleteCancel &= isEntitlementLaunched;

            // check data is uploaded to cloud
            List<String> expectedCloudFileNames = new ArrayList<>();
            expectedCloudFileNames.add("100-6d0bb00954ceb7fbee436bb55a8397a9"); // this is hash of original filename
            boolean isDataPushedToCloud = Waits.pollingWait(() ->
                    CloudSaveService.verifyCloudSaveExists(account, cloudSaveEntitlement, expectedCloudFileNames));
            if (!isDataPushedToCloud) {
                log("Data is not uploaded to cloud");
            }
            isSaveDeleteCancel &= isDataPushedToCloud;

        } else if (type == TEST_TYPE.DELETE) {
            newSaveDataFoundDialog.clickDeleteLocalDataButton();
            boolean isLocalFileDeleted = Waits.pollingWait(() -> !FileUtil.isFileExist(client,
                    cloudSaveEntitlementLocalSavePath + cloudSaveFileNameA));
            if (!isLocalFileDeleted) {
                log("Local file is not deleted");
            }
            isSaveDeleteCancel &= isLocalFileDeleted;

            boolean isEntitlementLaunched = Waits.pollingWaitEx(() -> cloudSaveEntitlement.isLaunched(client));
            if (!isEntitlementLaunched) {
                log("Entitlement is not launched");
            }
            isSaveDeleteCancel &= isEntitlementLaunched;

            // check data is not uploaded to cloud
            List<String> expectedCloudFileNames = new ArrayList<>();
            expectedCloudFileNames.add("100-6d0bb00954ceb7fbee436bb55a8397a9");
            boolean isDataPushedToCloud = Waits.pollingWait(() ->
                    !CloudSaveService.verifyCloudSaveExists(account, cloudSaveEntitlement, expectedCloudFileNames));
            if (!isDataPushedToCloud) {
                log("Data is uploaded to cloud");
            }
            isSaveDeleteCancel &= isDataPushedToCloud;
        } else {
            newSaveDataFoundDialog.clickCancelButton();
            isSaveDeleteCancel &= Waits.pollingWait(newSaveDataFoundDialog::waitIsClose)
                    && !cloudSaveEntitlement.isLaunched(client);
        }

        logPassFail(isSaveDeleteCancel, true);

        softAssertAll();
    }

    /**
     * Clean up local cloud folder to avoid conflict with other tests launching DIP Small entitlement
     *
     * @param result The results of the test
     * @param context The context in which the driver will be stored in the context attributes.
     */
    @AfterMethod(alwaysRun = true)
    @Override
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (client != null
                && cloudSaveEntitlementLocalSavePath != null
                && FileUtil.isDirectoryExist(client, cloudSaveEntitlementLocalSavePath)) {
            FileUtil.deleteFolder(client, cloudSaveEntitlementLocalSavePath);
        }
        super.testCleanUp(result, context);
    }
}
