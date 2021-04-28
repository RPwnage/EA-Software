using System;
using System.Collections.Generic;
using System.Configuration;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading;

using Microsoft.ApplicationInsights.DataContracts;
using Microsoft.ApplicationInsights.Channel;
using Microsoft.ApplicationInsights.Extensibility;

using Newtonsoft.Json;

using NAnt.Core.Metrics;

using AppInsightsClient = Microsoft.ApplicationInsights.TelemetryClient;

namespace NAnt.TelemetryClient
{
	public class Program
	{
		private static void Main(string[] args)
		{
			string debugOnStart = Environment.GetEnvironmentVariable("FRAMEWORK_TELEMETRY_DEBUGONSTART");
			if (debugOnStart != null)
			{
				if (debugOnStart == "1" || String.Compare(debugOnStart, "true", true) == 0)
				{
					Debugger.Launch();
				}
			}

			string telemetryFile = args.FirstOrDefault();
			if (telemetryFile == null)
			{
				return;
			}

			// load events file
			string serializedEvents = File.ReadAllText(telemetryFile);
			try
			{
				// try to delete file as soon as possible
				File.Delete(telemetryFile);
			}
			finally
			{
			}
			List<Event> events = JsonConvert.DeserializeObject<List<Event>>(serializedEvents);

			// transmit events
			if (events.Any())
			{
				string instrumentationKey = ConfigurationManager.AppSettings["ApplicationInsightsInstrumentationKey"];
				if (!String.IsNullOrEmpty(instrumentationKey))
				{
					// setup telemetry client with common context for all events
#if NETFRAMEWORK
					TelemetryConfiguration.Active.InstrumentationKey = instrumentationKey;
					AppInsightsClient clientInstance = new AppInsightsClient();
#else
					TelemetryConfiguration telemetryConfiguration = new TelemetryConfiguration(instrumentationKey);
					AppInsightsClient clientInstance = new AppInsightsClient(telemetryConfiguration);
#endif
					clientInstance.Context.User.Id = System.Security.Principal.WindowsIdentity.GetCurrent().Name;
					clientInstance.Context.Session.Id = Guid.NewGuid().ToString();
					clientInstance.Context.Device.Id = Environment.MachineName;
					clientInstance.Context.Device.OperatingSystem = Environment.OSVersion.ToString();
					clientInstance.Context.Component.Version = Assembly.GetExecutingAssembly().GetName().Version.ToString();
					clientInstance.Context.GlobalProperties["ApplicationName"] = "Framework";

					// queue up events
					foreach (Event ev in events)
					{
						EventTelemetry eventTelemetry = new EventTelemetry();
						eventTelemetry.Name = ev.Name;
						eventTelemetry.Timestamp = ev.TimeStamp;
						foreach (KeyValuePair<string, string> eventProperty in ev.Properties)
						{
							eventTelemetry.Properties[eventProperty.Key] = eventProperty.Value;
						}
						clientInstance.TrackEvent(eventTelemetry);
					}

					// wait for event channel to flush
#if NETFRAMEWORK					
					InMemoryChannel channel = TelemetryConfiguration.Active.TelemetryChannel as InMemoryChannel;
#else
					InMemoryChannel channel = telemetryConfiguration.TelemetryChannel as InMemoryChannel;
#endif
					if (channel != null)
					{
						channel.Flush(new TimeSpan(hours: 0, minutes: 2, seconds: 0));
					}
					else
					{
						// this else case is probably unnecessary since the channel should always ne InMemoryChannel, but it's left here as some level of future safety against upgrades to the
						// ApplicationInsights api - only the InMemoryChannel supports a blocking call to Flush (with timeout)
						// the generic flush implementation doesn't guarantee anything with regardless to blocking until flush is complete nor does it offer a timeout, we just call it and wait
						// for a brief peroid to try and allow any lagging telemetry to be sent
						clientInstance.Flush();
						Thread.Sleep(10000);
					}
				}
			}
		}
	}
}