package com.esn.sonar.core.util;

import com.esn.sonar.core.MessageDecoder;
import com.esn.sonar.core.MessageEncoder;
import com.esn.sonar.core.TabProtocolFrameDecoder;
import com.esn.sonar.core.challenge.ChallengeDecoder;
import com.esn.sonar.core.challenge.ChallengeEncoder;
import org.jboss.netty.bootstrap.ConnectionlessBootstrap;
import org.jboss.netty.bootstrap.ServerBootstrap;
import org.jboss.netty.buffer.HeapChannelBufferFactory;
import org.jboss.netty.channel.*;
import org.jboss.netty.channel.socket.ServerSocketChannelFactory;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.Field;
import java.net.BindException;
import java.net.InetSocketAddress;
import java.nio.ByteOrder;
import java.security.*;
import java.security.spec.EncodedKeySpec;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.ArrayList;
import java.util.List;

public class Utils
{
    /*
    About 6 times faster than string.split and about 1.5 faster than StringTokenizer
     */


    private static ThreadLocal<String[]> tempArray = new ThreadLocal<String[]>();

    public static String[] tokenize(String string, char delimiter)
    {
        String[] temp = tempArray.get();
        int tempLength = (string.length() / 2) + 2;

        if (temp == null || temp.length < tempLength)
        {
            temp = new String[tempLength];
            tempArray.set(temp);
        }


        int wordCount = 0;
        int i = 0;
        int j = string.indexOf(delimiter);

        while (j >= 0)
        {
            temp[wordCount++] = string.substring(i, j);
            i = j + 1;
            j = string.indexOf(delimiter, i);
        }


        temp[wordCount++] = string.substring(i);

        String[] result = new String[wordCount];
        System.arraycopy(temp, 0, result, 0, wordCount);
        return result;
    }
    /*

    private static final ThreadLocal<List<String>> localList = new ThreadLocal<List<String>>()
    {
        @Override
        protected List<String> initialValue()
        {
            return new ArrayList<String>(4);
        }
    };

    public static String[] tokenize(String str, char delim)
    {
        int length = str.length();
        int start = 0;

        List<String> list = localList.get();
        list.clear();

        for (int index = 0; index < length; index++)
        {
            if (str.charAt(index) == delim)
            {
                list.add(str.substring(start, index));
                start = index + 1;
            }
        }

        list.add(str.substring(start, length));
        return list.toArray(new String[list.size()]);
    }
    */


    public static boolean validateArgument(String arg, int maxLength)
    {
        int length = arg.length();

        if (length > maxLength)
        {
            return false;
        }

        for (int index = 0; index < length; index++)
        {
            if (arg.charAt(index) == '|')
            {
                return false;
            }
        }

        return true;
    }

    public static Channel createChallengeSocket(InetSocketAddress inetSocketAddress, final SimpleChannelUpstreamHandler upstreamHandler, ChannelFactory factory)
    {
        ConnectionlessBootstrap b = new ConnectionlessBootstrap(factory);

        // Configure the pipeline factory.
        b.setPipelineFactory(new ChannelPipelineFactory()
        {
            public ChannelPipeline getPipeline() throws Exception
            {
                return Channels.pipeline(
                        new ChallengeEncoder(),
                        new ChallengeDecoder(),
                        upstreamHandler);
            }
        });

        // Enable broadcast
        //b.setOption("broadcast", "false");
        b.setOption(
                "receiveBufferSizePredictorFactory",
                new FixedReceiveBufferSizePredictorFactory(64));

        b.setOption("bufferFactory", new HeapChannelBufferFactory(ByteOrder.LITTLE_ENDIAN));


        // Bind to the port and start the service.
        return b.bind(inetSocketAddress);
    }

    static private volatile long currTimestamp = -1;
    private static final Ref<Thread> currentTimeThread = new Ref<Thread>();
    private static volatile boolean isRunning = false;

    public static void create()
    {
        if (isRunning)
        {
            throw new IllegalStateException("TimeThread already running");
        }
        currTimestamp = System.currentTimeMillis();
        isRunning = true;
        currentTimeThread.set(newTimeThread());
        currentTimeThread.get().start();
    }

    static public void destroy() throws InterruptedException
    {
        isRunning = false;
        currentTimeThread.get().join();
    }

    private static Thread newTimeThread()
    {
        return new Thread(new Runnable()
        {
            public void run()
            {
                while (isRunning)
                {
                    currTimestamp = System.currentTimeMillis();
                    try
                    {
                        Thread.sleep(100);
                    } catch (InterruptedException e)
                    {
                    }
                }
            }
        });
    }

    public static long getTimestamp()
    {
        return currTimestamp;
    }

    public static Object clone(Object o)
    {
        Object clone = null;

        try
        {
            clone = o.getClass().newInstance();
        } catch (InstantiationException e)
        {
            e.printStackTrace();
        } catch (IllegalAccessException e)
        {
            e.printStackTrace();
        }

        // Walk up the superclass hierarchy
        for (Class obj = o.getClass();
             !obj.equals(Object.class);
             obj = obj.getSuperclass())
        {
            Field[] fields = obj.getDeclaredFields();
            for (int i = 0; i < fields.length; i++)
            {
                fields[i].setAccessible(true);
                try
                {
                    // for each class/suerclass, copy all fields
                    // from this object to the clone
                    fields[i].set(clone, fields[i].get(o));
                } catch (IllegalArgumentException e)
                {
                } catch (IllegalAccessException e)
                {
                }
            }
        }
        return clone;
    }

    public static byte[] dirtyStringToBytes(String str)
    {
        byte ret[] = new byte[str.length()];

        for (int index = 0; index < ret.length; index++)
        {
            ret[index] = (byte) (str.charAt(index) & 0x00ff);
        }

        return ret;
    }

    public static String dirtyBytesToString(byte[] bytes, int offset, int length)
    {
        char ret[] = new char[length];

        for (int index = 0; index < length; index++)
        {
            ret[index] = (char) bytes[offset + index];
        }

        return new String(ret);
    }

    private static final MessageEncoder messageEncoder = new MessageEncoder();
    private static final MessageDecoder messageDecoder = new MessageDecoder();

    public static MessageEncoder getMessageEncoder()
    {
        return messageEncoder;
    }

    public static MessageDecoder getMessageDecoder()
    {
        return messageDecoder;
    }

    public static ChannelPipeline getTabProtocolPipeline(ChannelHandler handler)
    {
        return new StaticChannelPipeline(new TabProtocolFrameDecoder(65536), messageDecoder, messageEncoder, handler);
    }

    public static Channel bindSocket(ChannelPipelineFactory pipelineFactory, InetSocketAddress address, ServerSocketChannelFactory channelFactory) throws BindException
    {
        try
        {
            ServerBootstrap bs = new ServerBootstrap(channelFactory);
            bs.setPipelineFactory(pipelineFactory);
            bs.setOption("backlog", 1024);

            bs.setOption("child.receiveBufferSizePredictorFactory", new FixedReceiveBufferSizePredictorFactory(4096));
            bs.setOption("child.bufferFactory", new HeapChannelBufferFactory());

            //NOTE: No performance gain from using DirectChannelBufferFactory

            Channel channel = bs.bind(address);
            return channel;

        } catch (ChannelException ce)
        {
            if (ce.getCause() instanceof BindException)
                throw new BindException("Unable to bind to port " + ce.getMessage().split(":")[2] + ". Port already in use?");
        }

        return null;
    }

    static byte[] readBinaryFile(File path)
    {
        byte[] ret = new byte[(int) path.length()];
        try
        {
            FileInputStream in = new FileInputStream(path);
            in.read(ret);
            in.close();
        } catch (IOException e)
        {
            return null;
        }

        return ret;
    }

    static void writeBinaryFile(File path, byte[] data)
    {
        try
        {
            FileOutputStream out = new FileOutputStream(path);
            out.write(data);
            out.close();
        } catch (IOException e)
        {
        }
    }

    public static KeyPair loadOrGenerateKeyPair(String basePath, String publicFilename, String privateFilename) throws NoSuchAlgorithmException, InvalidKeySpecException
    {
        KeyPairGenerator keyGen;
        File pubFile = new File(basePath + "/" + publicFilename);
        File prvFile = new File(basePath + "/" + privateFilename);

        byte[] pubBinKey = readBinaryFile(pubFile);
        byte[] prvBinKey = readBinaryFile(prvFile);

        KeyPair keyPair;

        if (pubBinKey == null || prvBinKey == null)
        {
            keyGen = KeyPairGenerator.getInstance("RSA");
            keyGen.initialize(512, new SecureRandom());
            keyPair = keyGen.generateKeyPair();

            writeBinaryFile(pubFile, keyPair.getPublic().getEncoded());
            writeBinaryFile(prvFile, keyPair.getPrivate().getEncoded());
        } else
        {
            KeyFactory keyFactory = KeyFactory.getInstance("RSA");
            EncodedKeySpec prvKeySpec = new PKCS8EncodedKeySpec(prvBinKey);
            EncodedKeySpec pubKeySpec = new X509EncodedKeySpec(pubBinKey);

            PrivateKey privateKey = keyFactory.generatePrivate(prvKeySpec);
            PublicKey publicKey = keyFactory.generatePublic(pubKeySpec);

            keyPair = new KeyPair(publicKey, privateKey);
        }

        return keyPair;
    }


    public static boolean isUnitTestMode()
    {
        return Boolean.getBoolean("sonar.unittest");
    }

    public static List<Integer> createSequence(int start, int end)
    {
        List<Integer> list = new ArrayList<Integer>(end - start);
        for (int i = start; i < end; i++)
        {
            list.add(i);
        }
        return list;
    }

    /**
     * Extract a range of T's from the given list.
     *
     * @param objects    the list
     * @param numRanges  the total number of ranges expected (used for range size calculation)
     * @param rangeIndex the current range index
     * @param <T>        the collection type
     * @return a (possible smaller) view of the given list
     */
    public static <T> List<T> extractRange(List<T> objects, int numRanges, int rangeIndex)
    {
        int size = objects.size();
        int rangeSize = size / numRanges;
        int fromIndex = rangeIndex * rangeSize;
        int toIndex = rangeIndex + 1 == numRanges ? size : Math.min(fromIndex + rangeSize, size);
        return objects.subList(fromIndex, toIndex);
    }

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
