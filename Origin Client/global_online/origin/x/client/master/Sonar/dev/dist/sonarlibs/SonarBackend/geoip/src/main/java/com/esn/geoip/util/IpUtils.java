package com.esn.geoip.util;

/**
 * User: ronnie
 * Date: 2011-06-24
 * Time: 09:39
 */
public class IpUtils
{
    /**
     * Converts a textual IP address to a long value.
     *
     * @param ip the IP string
     * @return its long equivalent
     * @see #longToIPV4(long)
     */
    public static long ipV4ToLong(String ip)
    {
        String[] split = ip.split("\\.");
        long addr = 0;

        addr |= Integer.parseInt(split[0]);
        addr <<= 8;
        addr |= Integer.parseInt(split[1]);
        addr <<= 8;
        addr |= Integer.parseInt(split[2]);
        addr <<= 8;
        addr |= Integer.parseInt(split[3]);

        return addr;
    }

    /**
     * Converts a long IP address to a textual value.
     *
     * @param addr the IP address as long
     * @return its string representation
     * @see #ipV4ToLong(String)
     */
    public static String longToIPV4(long addr)
    {
        StringBuilder sb = new StringBuilder();
        sb.append((int) (addr >>> 24) & 0xff);
        sb.append(".");
        sb.append((int) (addr >>> 16) & 0xff);
        sb.append(".");
        sb.append((int) (addr >>> 8) & 0xff);
        sb.append(".");
        sb.append((int) (addr >>> 0) & 0xff);

        return sb.toString();
    }
}
