package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAMoveGameAvailable;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Move Game' is available in OGD game setting menu if the game is installed
 *
 * @author sunlee
 */
public class OAMoveGameAvailableInOGDGameSettingMenu extends OAMoveGameAvailable {

    @TestRail(caseId = 4383087)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testMoveGameAvailableInOGDGameSettingMenu(ITestContext context) throws Exception {
        testMoveGameAvailable(context, METHOD_TO_MOVE_GAME.USE_OGD_GAME_SETTINGS, this.getClass().getName());
    }
}
