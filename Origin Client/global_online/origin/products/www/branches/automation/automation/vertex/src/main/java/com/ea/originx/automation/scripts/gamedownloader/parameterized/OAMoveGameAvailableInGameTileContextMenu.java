package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAMoveGameAvailable;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Move Game' is available in game tile context menu if the game is installed
 *
 * @author sunlee
 */
public class OAMoveGameAvailableInGameTileContextMenu extends OAMoveGameAvailable {

    @TestRail(caseId = 3926537)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testMoveGameAvailableInGameTileContextMenu(ITestContext context) throws Exception {
        testMoveGameAvailable(context, METHOD_TO_MOVE_GAME.USE_GAME_TILE_CONTEXT_MENU, this.getClass().getName());
    }
}
