package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OALocateGameAvailable;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Locate Game' is available in OGD game setting menu if the game is downloadable
 *
 * @author sunlee
 */
public class OALocateGameAvailableInOGDGameSettingMenu extends OALocateGameAvailable {

    @TestRail(caseId = 4380606)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testLocateGameAvailableInOGDGameSettingMenu(ITestContext context) throws Exception {
        testLocateGameAvailable(context, METHOD_TO_LOCATE_GAME.USE_OGD_GAME_SETTINGS, this.getClass().getName());
    }
}
