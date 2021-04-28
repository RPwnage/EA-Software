package com.esn.sonar.core;

import com.twitter.common.stats.Stat;
import com.twitter.common.stats.Stats;
import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.channel.ChannelFutureListener;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.MessageEvent;
import org.jboss.netty.channel.SimpleChannelUpstreamHandler;
import org.jboss.netty.handler.codec.http.DefaultHttpResponse;
import org.jboss.netty.handler.codec.http.HttpHeaders;
import org.jboss.netty.handler.codec.http.HttpResponse;
import org.jboss.netty.util.CharsetUtil;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import static org.jboss.netty.handler.codec.http.HttpResponseStatus.OK;
import static org.jboss.netty.handler.codec.http.HttpVersion.HTTP_1_1;

/**
 * User: ronnie
 * Date: 2011-09-13
 * Time: 09:21
 */
public class JsonStatisticsHandler extends SimpleChannelUpstreamHandler
{
    private static final String JSON_START = "{\n";
    private static final String JSON_END = "\n}\n";
    private static final String EMPTY_STRING = "";
    private static final String QUOT = "\"";
    private static final String INDENT = "  ";
    private static final String LINE_SEP = ",\n";

    private static final ThreadLocal<StringBuilder> tmpBuffer = new ThreadLocal<StringBuilder>()
    {
        @Override
        protected StringBuilder initialValue()
        {
            return new StringBuilder();
        }

        @Override
        public StringBuilder get()
        {
            StringBuilder stringBuilder = super.get();
            stringBuilder.setLength(0);
            return stringBuilder;
        }
    };

    protected String render()
    {
        final List<String> lines = new ArrayList<String>();

        Iterable<? extends Stat> variables = Stats.getVariables();
        for (Stat stat : variables)
        {
            lines.add(lineWithIndent(stat.getName(), stat.read()));
        }

        Collections.sort(lines);

        String prepend = EMPTY_STRING;
        StringBuilder sb = tmpBuffer.get();
        sb.append(JSON_START);
        for (String line : lines)
        {
            sb.append(prepend);
            prepend = LINE_SEP;
            sb.append(line);
        }
        sb.append(JSON_END);
        return sb.toString();
    }

    private String lineWithIndent(String name, Object value)
    {
        StringBuilder sb = tmpBuffer.get();
        sb.append(INDENT);
        sb.append(QUOT);
        sb.append(name);
        sb.append(QUOT);
        sb.append(" : ");
        sb.append(value);
        return sb.toString();
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, final MessageEvent e) throws Exception
    {
        byte[] data = render().getBytes(CharsetUtil.UTF_8);

        ChannelBuffer content = ChannelBuffers.buffer(data.length);
        content.writeBytes(data);

        HttpResponse response = new DefaultHttpResponse(HTTP_1_1, OK);
        response.setHeader(HttpHeaders.Names.CONTENT_TYPE, "text/plain; charset=UTF-8");
        response.setContent(content);
        response.setHeader(HttpHeaders.Names.CONTENT_LENGTH, data.length);


        e.getChannel().write(response).addListener(ChannelFutureListener.CLOSE);
    }

}
