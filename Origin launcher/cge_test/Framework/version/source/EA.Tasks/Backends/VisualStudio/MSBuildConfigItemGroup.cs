// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;

using NAnt.Core;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig.Backends.VisualStudio
{
	internal class MSBuildConfigItemGroup
	{
		private class ConfigItem
		{
			internal readonly string Type;																// Item type, None, ClCompile, etc

			private readonly HashSet<Configuration> m_configurations = new HashSet<Configuration>();	// set of configurations that reference this item
			private readonly Dictionary<string, Dictionary<string, HashSet<Configuration>>> m_meta =		// meta data for this item, m_meta[metaDataName][metaDataValue] = <list of configs that specify metaDataName with metaDataValue
				new Dictionary<string, Dictionary<string, HashSet<Configuration>>>();
			private List<KeyValuePair<string, KeyValuePair<string, List<Configuration>>>> m_cachedMeta;

			internal ConfigItem(string type)
			{
				Type = type;
			}

			internal void AddModuleReference(Configuration config, Dictionary<string, string> metaData = null)
			{
				// update list of configs that have this item
				m_configurations.Add(config);

				// update meta data and track every config which shares key/value pair
				if (metaData != null)
				{
					foreach (KeyValuePair<string, string> metaDataPair in metaData)
					{
						Dictionary<string, HashSet<Configuration>> valueMap = null;
						if (!m_meta.TryGetValue(metaDataPair.Key, out valueMap))
						{
							valueMap = new Dictionary<string, HashSet<Configuration>>();
							m_meta[metaDataPair.Key] = valueMap;
						}

						HashSet<Configuration> moduleList = null;
						if (!valueMap.TryGetValue(metaDataPair.Value, out moduleList))
						{
							moduleList = new HashSet<Configuration>();
							valueMap[metaDataPair.Value] = moduleList;
						}

						moduleList.Add(config);
					}

					m_cachedMeta = null;
				}
			}

			internal void WriteTo(IXmlWriter writer, string includePath, IEnumerable<Configuration> allConfiguartions, Func<Configuration, string> configFormatter, Func<string, string> getProjectPath)
			{
				writer.WriteStartElement(Type);
				writer.WriteAttributeString("Include", getProjectPath(includePath));

				// wrap the whole item in conditions for configs if building against specific configs
				MSBuildConfigConditions.Condition condition = MSBuildConfigConditions.GetCondition(m_configurations, allConfiguartions);
				if (condition.ConditionType != MSBuildConfigConditions.Condition.Type.None)
				{
					writer.WriteAttributeString("Condition", MSBuildConfigConditions.FormatCondition("$(Configuration)|$(Platform)", condition, configFormatter));
				}


				if (m_cachedMeta == null)
				{
					m_cachedMeta = new List<KeyValuePair<string, KeyValuePair<string, List<Configuration>>>>();
					foreach (KeyValuePair<string, Dictionary<string, HashSet<Configuration>>> metaDataEntry in m_meta.OrderBy(kvp => kvp.Key))
						foreach (KeyValuePair<string, HashSet<Configuration>> valueToConfigs in metaDataEntry.Value.OrderBy(kvp => kvp.Key))
							m_cachedMeta.Add(new KeyValuePair<string, KeyValuePair<string, List<Configuration>>>(metaDataEntry.Key, new KeyValuePair<string, List<Configuration>>(valueToConfigs.Key, valueToConfigs.Value.ToList())));
				}

				// write out meta data conditionally by config
				foreach (var entry in m_cachedMeta)
				{
                    writer.WriteStartElement(entry.Key);

                    MSBuildConfigConditions.Condition metaDataCondition = MSBuildConfigConditions.GetCondition(entry.Value.Value, m_configurations);
					if (metaDataCondition.ConditionType != MSBuildConfigConditions.Condition.Type.None)
					{
						writer.WriteAttributeString("Condition", MSBuildConfigConditions.FormatCondition("$(Configuration)|$(Platform)", metaDataCondition, configFormatter));
					}

					writer.WriteString(entry.Value.Key);
                    writer.WriteEndElement();
				}

				writer.WriteEndElement();
			}
		}

		private readonly IEnumerable<Configuration> m_allConfigurations;
		private readonly Dictionary<string, ConfigItem> m_items = new Dictionary<string, ConfigItem>();
		private readonly string m_label;

		internal MSBuildConfigItemGroup(IEnumerable<Configuration> configs, string label = null)
		{
			m_allConfigurations = configs;
			m_label = label;
		}

		internal void AddItem(Configuration config, string type, string path, Dictionary<string, string> metaData = null)
		{
			ConfigItem item = null;
			if (!m_items.TryGetValue(path, out item))
			{
				item = new ConfigItem(type);
				m_items[path] = item;
			}
			else
			{
				if (item.Type != type)
				{
					throw new BuildException(String.Format("Item group definition contains multiple types ({0}, {1}) for path '{2}'.", item.Type, type, path));
				}
			}
			item.AddModuleReference(config, metaData);
		}

		internal void WriteTo(IXmlWriter writer, Func<Configuration, string> configFormatter, Func<string, string> getProjectPath)
		{
			if (m_items.Any())
			{
                writer.WriteStartElement("ItemGroup");
				writer.WriteNonEmptyAttributeString("Label", m_label);
				foreach (KeyValuePair<string, ConfigItem> pathToItem in m_items.OrderBy(kvp => kvp.Key))
				{
					ConfigItem item = pathToItem.Value;
					string includePath = pathToItem.Key;
					item.WriteTo(writer, includePath, m_allConfigurations, configFormatter, getProjectPath);
				}
				writer.WriteEndElement();
			}
		}
	}
}