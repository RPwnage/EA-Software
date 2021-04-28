package com.esn.sonar.core;

import com.twitter.common.stats.Stat;
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

import static org.jboss.netty.handler.codec.http.HttpResponseStatus.OK;
import static org.jboss.netty.handler.codec.http.HttpVersion.HTTP_1_0;

public class StatisticsHandler extends SimpleChannelUpstreamHandler
{
    private StatsManager[] statsManagers;
    private static final String NL = "\n";

    public StatisticsHandler(StatsManager[] statsManagers)
    {
        this.statsManagers = statsManagers;
    }

    public StatsManager[] getStatsManagers()
    {
        return statsManagers;
    }

    private void renderMonitor(StringBuilder sb, StatsManager statsManager)
    {
        sb.append("<h2>");
        sb.append(statsManager.getName());
        sb.append("</h2>");

        sb.append("<table>");
        Stat[] percentiles = statsManager.getRequestStatsPercentiles();
        sb.append("<tr>");
        sb.append("<th>Request percentiles</th>");
        for (Stat percentile : percentiles)
        {
            sb.append("<th>");
            sb.append(percentile.getName().substring(statsManager.requestStatsName().length() + 1));
            sb.append("</th>");
        }
        sb.append("</tr>");
        sb.append("<tr>");
        sb.append("<td></td>");
        for (Stat percentile : percentiles)
        {
            sb.append("<td>");
            sb.append(percentile.read());
            sb.append(" us</td>");
        }
        sb.append("</tr>");
        sb.append("</table>");
        sb.append(NL);
        sb.append("<div class='spacer'></div>");
        sb.append(NL);
        sb.append("<table>");
        Stat[] accumulated = statsManager.getRequestStatsAccumulated();
        sb.append("<tr>");
        sb.append("<th>Request stats</th>");
        for (Stat accumulate : accumulated)
        {
            sb.append("<th>");
            sb.append(accumulate.getName().substring(statsManager.requestStatsName().length() + 1));
            sb.append("</th>");
        }
        sb.append("</tr>");
        sb.append("<tr>");
        sb.append("<td></td>");
        for (Stat accumulate : accumulated)
        {
            sb.append("<td>");
            sb.append(accumulate.read());
            sb.append("</td>");
        }
        sb.append("</tr>");
        sb.append("</table>");
        sb.append(NL);
        sb.append("<div class='spacer'></div>");
        sb.append(NL);
        sb.append("<table>");
        sb.append("<tr>");
        sb.append("<th>Metrics</th>");
        sb.append("<th>RPS 1 minute ago</th>");
        sb.append("<th>Current minute</th>");
        sb.append("<th>1 minute ago</th>");
        sb.append("<th>2 minutes ago</th>");
        sb.append("<th>3 minutes ago</th>");
        sb.append("<th>4 minutes ago</th>");
        sb.append("<th>5 minutes ago</th>");
        sb.append("<th>Current 10 minutes</th>");
        sb.append("<th>Previous 10 minutes</th>");
        sb.append("<th>This hour</th>");
        sb.append("<th>Last hour</th>");
        sb.append("<th>Last 6 hours</th>");
        sb.append("<th>Last 24 hours</th>");
        sb.append("</tr>");

        for (Enum metric : statsManager.getMetricsConstants())
        {
            sb.append("<tr>");
            sb.append("<td class='left'>");
            sb.append(metric.name());
            sb.append("</td>");

            // RPS
            sb.append("<td>");
            double rps = Math.floor(statsManager.getMetricRps(metric) * 100d + 0.5d) / 100d;
            sb.append(rps > 0 ? rps : "N/A");
            sb.append("</td>");

            // Current minute
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 0, 1));
            sb.append("</td>");

            // 1 minute ago
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 1, 1));
            sb.append("</td>");

            // 2 minute ago
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 2, 1));
            sb.append("</td>");

            // 3 minute ago
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 3, 1));
            sb.append("</td>");

            // 4 minute ago
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 4, 1));
            sb.append("</td>");

            // 5 minute ago
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 5, 1));
            sb.append("</td>");


            // 10 minutes
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 0, 10));
            sb.append("</td>");

            // 10 minutes
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 10, 10));
            sb.append("</td>");

            // 1 hour seconds
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 0, 60));
            sb.append("</td>");

            // Last 1 hour seconds
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 60, 60));
            sb.append("</td>");

            // 6 hours
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 0, 60 * 6));
            sb.append("</td>");

            // 24 hours
            sb.append("<td>");
            sb.append(statsManager.getMetricsValue(metric, 0, 60 * 24));
            sb.append("</td>");

            sb.append("</tr>");
        }

        sb.append("<tr><th>Gauges</th></tr>");

        for (Enum gauge : statsManager.getGaugesConstants())
        {
            sb.append("<tr>");
            sb.append("<td class='left'>");
            sb.append(gauge.name());
            sb.append("</td>");

            // RPS
            sb.append("<td>");
            sb.append("N/A");
            sb.append("</td>");

            // Current minute
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 0, 1));
            sb.append("</td>");

            // 1 minute ago
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 1, 1));
            sb.append("</td>");

            // 2 minute ago
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 2, 1));
            sb.append("</td>");

            // 3 minute ago
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 3, 1));
            sb.append("</td>");

            // 4 minute ago
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 4, 1));
            sb.append("</td>");

            // 5 minute ago
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 5, 1));
            sb.append("</td>");


            // 10 minutes
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 0, 10));
            sb.append("</td>");

            // 10 minutes
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 10, 10));
            sb.append("</td>");

            // 1 hour seconds
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 0, 60));
            sb.append("</td>");

            // Last 1 hour seconds
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 60, 60));
            sb.append("</td>");

            // 6 hours
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 0, 60 * 6));
            sb.append("</td>");

            // 24 hours
            sb.append("<td>");
            sb.append(statsManager.getGaugeValue(gauge, 0, 60 * 24));
            sb.append("</td>");

            sb.append("</tr>");
        }

        sb.append("</table>");

    }

    private String render()
    {
        StringBuilder sb = new StringBuilder(32768);

        sb.append("<html><head><title>Statistics</title>");
        sb.append(NL);
        sb.append("<style type='text/css'>");
        sb.append(NL);
        sb.append("<!--");
        sb.append(NL);
        sb.append("  body { color: #333333; font-family: monospace; }");
        sb.append(NL);
        sb.append("  table { width: 100%; border-collapse: collapse; border: 1px #6699CC solid; }");
        sb.append(NL);
        sb.append("  th { border: 1px #6699CC solid;}");
        sb.append(NL);
        sb.append("  td { text-align: center; border: 1px #6699CC solid;}");
        sb.append(NL);
        sb.append("  .left { text-align: left; }");
        sb.append(NL);
        sb.append("  .right { text-align: right; }");
        sb.append(NL);
        sb.append("  .spacer { height: 20px; }");
        sb.append(NL);
        sb.append("-->");
        sb.append(NL);
        sb.append("</style>");
        sb.append(NL);
        sb.append("</head>");
        sb.append(NL);
        sb.append("<body>");
        sb.append("<h1>");
        sb.append("Statistics:");
        sb.append("</h1>");
        sb.append(NL);

        for (StatsManager statsManager : statsManagers)
        {
            renderMonitor(sb, statsManager);

        }

        sb.append("</body>");
        sb.append(("</html>"));

        return sb.toString();
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, final MessageEvent e) throws Exception
    {
        byte[] data = render().getBytes(CharsetUtil.UTF_8);

        ChannelBuffer content = ChannelBuffers.buffer(data.length);
        content.writeBytes(data);

        HttpResponse response = new DefaultHttpResponse(HTTP_1_0, OK);
        response.setHeader(HttpHeaders.Names.CONTENT_TYPE, "text/html; charset=UTF-8");
        response.setContent(content);
        response.setHeader(HttpHeaders.Names.CONTENT_LENGTH, data.length);


        e.getChannel().write(response).addListener(ChannelFutureListener.CLOSE);
    }

}
