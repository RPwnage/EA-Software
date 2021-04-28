package com.esn.sonar.geoip;

import com.esn.geoip.BitTree;
import com.esn.geoip.util.IpUtils;
import org.junit.Test;

import static org.junit.Assert.*;

/**
 * User: ronnie
 * Date: 2011-06-21
 * Time: 15:33
 */
public class BitTreeTest
{
    @Test
    public void testConversion()
    {
        String ip0 = "192.168.0.0";
        String ip1 = "192.168.0.1";
        String ip2 = "192.168.0.2";

        assertEquals(ip0, longToIPV4(ipV4ToLong(ip0)));
        assertEquals(ip1, longToIPV4(ipV4ToLong(ip1)));
        assertEquals(ip2, longToIPV4(ipV4ToLong(ip2)));

        assertEquals(ip1, longToIPV4(ipV4ToLong(ip0) + 1));

        assertEquals(ipV4ToLong(ip1), ipV4ToLong(ip0) + 1);
        assertEquals(ipV4ToLong(ip2), ipV4ToLong(ip0) + 2);
    }

    @Test
    public void testLookup()
    {
        BitTree<Boolean> db = new BitTree<Boolean>();

        // Network 1: 223.252.0.0 - 223.252.127.255
        long range1start = ipV4ToLong("223.252.0.0");
        long range1end = ipV4ToLong("223.252.127.255");
        db.add(range1start, range1end, false);

        // Network 2: 223.252.128.0 - 223.252.255.255
        long range2start = ipV4ToLong("223.252.128.0");
        long range2end = ipV4ToLong("223.252.255.255");
        db.add(range2start, range2end, true);

        // Network 3: 192.168.1.10 (single address)
        long range3 = ipV4ToLong("192.168.1.10");
        db.add(range3, range3, true);

        // Network 1: 223.252.0.0 - 223.252.127.255
        assertFalse(db.find(ipV4ToLong("223.252.0.0")));
        assertFalse(db.find(ipV4ToLong("223.252.0.1")));
        assertFalse(db.find(ipV4ToLong("223.252.8.8")));
        assertFalse(db.find(ipV4ToLong("223.252.127.254")));
        assertFalse(db.find(ipV4ToLong("223.252.127.255")));

        // Network 2: 223.252.128.0 - 223.252.255.255
        assertTrue(db.find(ipV4ToLong("223.252.128.0")));
        assertTrue(db.find(ipV4ToLong("223.252.128.1")));
        assertTrue(db.find(ipV4ToLong("223.252.188.0")));
        assertTrue(db.find(ipV4ToLong("223.252.255.254")));
        assertTrue(db.find(ipV4ToLong("223.252.255.255")));

        // Network 3: 192.168.1.10 (single address)
        assertTrue(db.find(ipV4ToLong("192.168.1.10")));
        assertNull(db.find(ipV4ToLong("192.168.1.9")));
        assertNull(db.find(ipV4ToLong("192.168.1.11")));

        // Non-existing IP:s
        assertNull(db.find(ipV4ToLong("127.0.0.1")));
        assertNull(db.find(ipV4ToLong("192.168.0.1")));
        assertNull(db.find(ipV4ToLong("223.251.255.255")));
        assertNull(db.find(ipV4ToLong("223.253.0.0")));
    }


    private static long ipV4ToLong(String ip)
    {
        return IpUtils.ipV4ToLong(ip);
    }

    private static String longToIPV4(long addr)
    {
        return IpUtils.longToIPV4(addr);
    }
}
