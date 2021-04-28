using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

using Newtonsoft.Json;

using NuGet.Frameworks;
using NuGet.Packaging.Core;
using NuGet.Versioning;

namespace NAnt.NuGet
{
	// cachable result of a nuget manager resolution
	internal class ResolutionResult
	{
		private delegate void PropertyWriteFunction(JsonTextWriter writer);
		private delegate void ArrayElementWriteFunction<TSource>(JsonTextWriter writer, TSource element);

		private delegate bool PropertyReadFunction(JsonTextReader reader, string propertyName);
		private delegate bool TokenReadFunction(JsonTextReader reader);

		private readonly Dictionary<PackageIdentity, NugetSource> m_identitiesToSources;
		private readonly Dictionary<string, List<PackageIdentity>> m_dependenciesForPackage;
		private readonly HashSet<(string, string)> m_missingPackages;
		private readonly NugetSource[] m_sourceRepositories;
		private readonly NuGetFramework m_nugetFramework; // for errors

		internal ResolutionResult(NuGetFramework framework, IEnumerable<NugetSource> sourceRepositories, IDictionary<PackageIdentity, NugetSource> identitiesToSources, IDictionary<string, List<PackageIdentity>> dependenciesByPackage, IEnumerable<(string, string)> missingPackages)
		{
			m_sourceRepositories = sourceRepositories.ToArray();
			m_identitiesToSources = new Dictionary<PackageIdentity, NugetSource>(identitiesToSources);
			m_dependenciesForPackage = dependenciesByPackage.ToDictionary(kvp => kvp.Key, kvp => kvp.Value.ToList());
			m_missingPackages = new HashSet<(string, string)>(missingPackages);
			m_nugetFramework = framework;
		}

		internal IEnumerable<PackageIdentity> GetResolvedPackageIdentities(IEnumerable<string> packageNames)
		{
			HashSet<PackageIdentity> flattenedPackageList = new HashSet<PackageIdentity>(PackageIdentity.Comparer);
			foreach (string packageName in packageNames)
			{
				if (m_dependenciesForPackage.TryGetValue(packageName, out List<PackageIdentity> deps))
				{
					foreach (PackageIdentity identity in deps)
					{
						flattenedPackageList.Add(identity);
					}
				}
				else
				{
					(string, string) missingMatch = m_missingPackages.FirstOrDefault(missing => packageName.Equals(missing.Item1, StringComparison.OrdinalIgnoreCase));
					if (missingMatch != default)
					{
						throw new MissingNugetPackageException(packageName, missingMatch.Item2, m_nugetFramework, m_sourceRepositories);
					}
					else
					{
						throw new UnresolvedNugetPackageException(packageName, m_nugetFramework);
					}
				}
			}
			return flattenedPackageList;
		}

		internal NugetSource GetSourceforIdentity(PackageIdentity identity)
		{
			if (!m_identitiesToSources.TryGetValue(identity, out NugetSource source))
			{
				throw new InvalidOperationException($"Couldn't find a nuget source for identity '{identity.Id} {identity.Version}'.");
			}
			return source;
		}

		internal void WriteToCacheFile(string cacheFilePath)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(cacheFilePath));
			using (StreamWriter cacheFileStream = File.CreateText(cacheFilePath))
			using (JsonTextWriter jsonTextWriter = new JsonTextWriter(cacheFileStream))
			{
				WriteObject
				(
					jsonTextWriter,
					(
						"identitiesToSources", identitiesWriter => WriteArray
						(
							identitiesWriter, m_identitiesToSources,
							(idToSourceWriter, idToSource) => WriteObject
							(
								idToSourceWriter,
								("id", idWriter => idWriter.WriteValue(idToSource.Key.Id)),
								("version", versionWriter => versionWriter.WriteValue(idToSource.Key.Version.OriginalVersion)),
								("source", sourceWriter => sourceWriter.WriteValue(idToSource.Value.Repository.PackageSource.Source))
							)
						)
					),
					(
						"dependenciesForPackage", dependenciesWriter => WriteObject
						(
							dependenciesWriter, m_dependenciesForPackage,
							packageNameToDependencies => packageNameToDependencies.Key,
							(packageToDepWriter, packageNameToDependencies) => WriteArray
							(
								packageToDepWriter,
								packageNameToDependencies.Value,
								(depWriter, depIdentity) => WriteObject
								(
									depWriter,
									("id", idWriter => idWriter.WriteValue(depIdentity.Id)),
									("version", versionWriter => versionWriter.WriteValue(depIdentity.Version.OriginalVersion))
								)
							)
						)
					),
					(
						"missingPackages", missingPackagesWriter => WriteArray
						(
							jsonTextWriter, m_missingPackages,
							(writer, missingPackage) => WriteObject
							(
								writer,
								("id", propWriter => propWriter.WriteValue(missingPackage.Item1)),
								("version", propWriter => propWriter.WriteValue(missingPackage.Item2))
							)
						)
					)
				);
			}
		}

		internal static bool TryLoadFromCacheFile(NuGetFramework framework, string cacheFilePath, IEnumerable<NugetSource> sourceRepositories, out ResolutionResult resolution)
		{
			try
			{
				if (!File.Exists(cacheFilePath))
				{
					resolution = null;
					return false;
				}

				using (StreamReader cacheFileStream = File.OpenText(cacheFilePath))
				using (JsonTextReader jsonTextReader = new JsonTextReader(cacheFileStream))
				{
					Dictionary<PackageIdentity, NugetSource> identitiesToSources = new Dictionary<PackageIdentity, NugetSource>(PackageIdentity.Comparer);
					Dictionary<string, List<PackageIdentity>> dependenciesByPackage = new Dictionary<string, List<PackageIdentity>>();
					List<(string, string)> missingPackages = new List<(string, string)>();
					if (!ReadObject
					(
						jsonTextReader,
						(
							"identitiesToSources", identityReader => ReadArray
							(
								identityReader, elementReader =>
								{
									string id = default, version = default, source = default;
									if (!ReadObject
									(
										elementReader,
										("id", idReader => ReadStringValue(idReader, out id)),
										("version", versionReader => ReadStringValue(versionReader, out version)),
										("source", sourceReader => ReadStringValue(sourceReader, out source))
									))
									{
										return false;
									}

									if (!NuGetVersion.TryParse(version, out NuGetVersion asVersion))
									{
										return false;
									}

									NugetSource matchingSource = sourceRepositories.FirstOrDefault(nugetSource => nugetSource.Repository.PackageSource.Source == source);
									if (matchingSource == null)
									{
										return false;
									}

									identitiesToSources[new PackageIdentity(id, asVersion)] = matchingSource;
									return true;
								}
							)
						),
						(
							"dependenciesForPackage", packageToDepsReader => ReadObject
							(
								packageToDepsReader, (depsReader, packageName) =>
								{
									List<PackageIdentity> dependencies = new List<PackageIdentity>();
									if (!ReadArray
									(
										depsReader,
										depReader =>
										{
											string id = default, version = default;
											if (!ReadObject(
												depReader,
												("id", idReader => ReadStringValue(idReader, out id)),
												("version", versionReader => ReadStringValue(versionReader, out version))
											))
											{
												return false;
											}

											if (!NuGetVersion.TryParse(version, out NuGetVersion asVersion))
											{
												return false;
											}

											dependencies.Add(new PackageIdentity(id, asVersion));
											return true;
										}
									))
									{
										return false;
									}

									dependenciesByPackage[packageName] = dependencies;
									return true;
								}
							)
						),
						(
							"missingPackages", missingPackagesReader => ReadArray
							(
								missingPackagesReader, missingReader =>
								{
									string id = default, version = default;
									if (!ReadObject
									(
										missingReader,
										("id", idReader => ReadStringValue(idReader, out id)),
										("version", versionReader => ReadStringValue(versionReader, out version))
									))
									{
										return false;
									}

									missingPackages.Add((id, version));
									return true;
								}
							)
						)
					))
					{
						resolution = null;
						return false;
					}

					resolution = new ResolutionResult
					(
						framework,
						sourceRepositories,
						identitiesToSources,
						dependenciesByPackage,
						missingPackages
					);
					return true;
				}
			}
			catch
			{
				resolution = null;
				return false;
			}
		}

		private static void WriteObject(JsonTextWriter jsonTextWriter, params (string propertyName, PropertyWriteFunction propertyWriteFunc)[] propertyWrites)
		{
			jsonTextWriter.WriteStartObject();
			foreach ((string propertyName, PropertyWriteFunction propertyWriteFunc) in propertyWrites)
			{
				jsonTextWriter.WritePropertyName(propertyName);
				propertyWriteFunc(jsonTextWriter);
			}
			jsonTextWriter.WriteEndObject();
		}

		private static void WriteObject<TSource>(JsonTextWriter jsonTextWriter, IEnumerable<TSource> source, Func<TSource, string> propertyNameSelector, Action<JsonTextWriter, TSource> propertyValueWriter)
		{
			WriteObject
			(
				jsonTextWriter,
				source.Select
				(
					element => 
					(
						propertyNameSelector(element),
						(PropertyWriteFunction)(writer => propertyValueWriter(writer, element))
					)
				)
				.ToArray()
			);
		}

		private static void WriteArray<TSource>(JsonTextWriter jsonTextWriter, IEnumerable<TSource> source, ArrayElementWriteFunction<TSource> writeFunction)
		{
			jsonTextWriter.WriteStartArray();
			foreach (TSource element in source)
			{
				writeFunction(jsonTextWriter, element);
			}
			jsonTextWriter.WriteEndArray();
		}

		private static bool ReadObject(JsonTextReader jsonTextReader, params (string, TokenReadFunction)[] properties)
		{
			if (!jsonTextReader.Read() || jsonTextReader.TokenType != JsonToken.StartObject)
			{
				return false;
			}

			foreach ((string property, TokenReadFunction propertyReadFunc) in properties)
			{
				if (!jsonTextReader.Read() || jsonTextReader.TokenType != JsonToken.PropertyName || !jsonTextReader.Value.Equals(property))
				{
					return false;
				}

				if (!propertyReadFunc(jsonTextReader))
				{
					return false;
				}
			}

			return jsonTextReader.Read() && jsonTextReader.TokenType == JsonToken.EndObject;
		}

		private static bool ReadObject(JsonTextReader jsonTextReader, PropertyReadFunction propertyReadFunc)
		{
			if (!jsonTextReader.Read() || jsonTextReader.TokenType != JsonToken.StartObject)
			{
				return false;
			}

			while (jsonTextReader.Read())
			{
				if (jsonTextReader.TokenType == JsonToken.EndObject)
				{
					return true;
				}
				else if (jsonTextReader.TokenType == JsonToken.PropertyName)
				{
					string property = (string)jsonTextReader.Value;
					if (!propertyReadFunc(jsonTextReader, property))
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			return false;
		}

		private static bool ReadArray(JsonTextReader jsonTextReader, TokenReadFunction elementReaderFunc)
		{
			if (!jsonTextReader.Read() && jsonTextReader.TokenType != JsonToken.StartArray)
			{
				return false;
			}

			while (elementReaderFunc(jsonTextReader)) { }
			return jsonTextReader.TokenType == JsonToken.EndArray;
		}

		private static bool ReadStringValue(JsonTextReader jsonTextReader, out string value)
		{
			if (!jsonTextReader.Read() && jsonTextReader.TokenType != JsonToken.String)
			{
				value = null;
				return false;
			}

			value = (string)jsonTextReader.Value;
			return true;
		}
	}
}