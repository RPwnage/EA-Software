package com.ea.tool.origin.gridnodeagent;

import java.io.File;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.net.URISyntaxException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.json.JSONObject;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Assert;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

public class GridNodeAgentTest {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    private static final String LOCAL_GRID_NODE_AGENT_SCRIPT = "agent_main_local.py";
    private static final Path LOCAL_GRID_NODE_AGENT_DIR = Paths.get("..", "..", "..", "selenium-grid-node");
    private static final String LOCAL_GRID_NODE_AGENT_HOST_BASE = "http://localhost:8080";

    private static Process localNodeAgent = null;

    @BeforeClass
    public static void setUpClass() throws IOException {
        final ArrayList<String> command = new ArrayList<>();
        command.add(System.getProperties().getProperty("python"));
        command.add(LOCAL_GRID_NODE_AGENT_SCRIPT);
        final ProcessBuilder procBuilder = new ProcessBuilder(command).directory(LOCAL_GRID_NODE_AGENT_DIR.toFile()).inheritIO();
        _log.debug("starting local grid node agent ...");
        _log.debug("command: " + procBuilder.command());
        _log.debug("directory: " + procBuilder.directory().getAbsolutePath());
        try {
            // need to set redirect to solve powershell execution hangs problem
            final File tempRedirect = File.createTempFile("redirect", ".log");
            tempRedirect.deleteOnExit();
            procBuilder.redirectInput(tempRedirect);
            localNodeAgent = procBuilder.start();
        } catch (IOException e) {
            _log.error(e);
            throw e;
        }
    }

    @AfterClass
    public static void tearDownClass() {
        try {
            OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/stop");
        } catch (IOException | URISyntaxException e) {
            _log.error(e);
            if (null != localNodeAgent) {
                localNodeAgent.destroy();
            }
        }

        if (null != localNodeAgent) {
            try {
                localNodeAgent.waitFor(10, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                _log.error(e);
            }
            _log.debug("is local grid node agent running? " + localNodeAgent.isAlive());
            localNodeAgent = null;
        }
    }

    @Before
    public void setUp() {
    }

    @After
    public void tearDown() {
    }

    @Test
    public void fileDirectoryScreenCaptureTest() throws Exception {
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/directory/create?path=tobedeleted"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/create?path=tobedeleted/temp.txt&size=1000"));
        Assert.assertEquals("1000", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/size?path=tobedeleted/temp.txt"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/copy?source=tobedeleted/temp.txt&dest=tobedeleted/tobedeleted.txt"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.post(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/write?path=tobedeleted/temp.txt", "a"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/move?source=tobedeleted/temp.txt&dest=tobedeleted/tobedeleted.txt"));
        Assert.assertEquals("a", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/read?path=tobedeleted/tobedeleted.txt"));
        final File alsotobedeleted = LOCAL_GRID_NODE_AGENT_DIR.resolve("tobedeleted").resolve("alsotobedeleted.png").toFile();
        OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/capture/fullscreen", alsotobedeleted);
        Assert.assertEquals("[\"alsotobedeleted.png\", \"tobedeleted.txt\"]", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/directory/list?path=tobedeleted"));
        Assert.assertEquals("[\"alsotobedeleted.png\"]", OAAgentRestfulHelper.post(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/directory/search?path=tobedeleted", "[\"delete\", \"png\"]"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/delete?path=tobedeleted/alsotobedeleted.png"));
        Assert.assertEquals("False", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/file/exist?path=tobedeleted/alsotobedeleted.png"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/directory/delete?path=tobedeleted"));
        Assert.assertEquals("False", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/directory/exist?path=tobedeleted"));
    }

    @Test
    public void systemInformationTest() throws Exception {
        JSONObject environ = new JSONObject(OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/sysinfo/environ")).getJSONObject("data");
        _log.debug(environ.toString());
        for (String key : System.getenv().keySet()) {
            _log.debug(key + "=" + System.getenv().get(key));
            if (!key.startsWith("=")) {
                Assert.assertEquals(System.getenv().get(key), environ.getString(key));
            }
        }
    }

    @Test
    public void shellKeyEventTest() throws Exception {
        Assert.assertEquals("123\r\n", OAAgentRestfulHelper.post(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/shell/sync?shell=True", "echo 123"));
        final String pid = OAAgentRestfulHelper.post(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/shell/async", "notepad");
        Thread.sleep(2000L);
        Assert.assertEquals("ok", OAAgentRestfulHelper.post(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/keyevent/type?delay=10", "typing ..."));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/keyevent/multikey?combo=17+90&delay=1000"));
        Thread.sleep(1000L);
        Assert.assertEquals("SUCCESS: Sent termination signal to the process with PID " + pid + ".\r\n", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/shell/killproc?pid=" + pid));
    }

    @Test
    public void utilTest() throws Exception {
        Assert.assertEquals("True", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/util/clientstatus"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/util/fullscreen?id=0_node_agent_unit_test&name=pass.png"));
        Assert.assertEquals("ok", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/util/upload?id=0_node_agent_unit_test"));
        Assert.assertEquals("True", OAAgentRestfulHelper.get(LOCAL_GRID_NODE_AGENT_HOST_BASE + "/util/enablesilentuninstall?windowsProductCode=1A590C87-0B62-4CF7-9B3C-85789E577C03"));
    }

}
