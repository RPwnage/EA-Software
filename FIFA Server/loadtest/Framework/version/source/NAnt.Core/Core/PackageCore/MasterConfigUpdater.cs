using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Xml;
using NAnt.Core.Logging;
using NAnt.Core.Util;

namespace NAnt.Core.PackageCore
{
	// TODO MasterConfigUpdater improvements
	// dvaliant 2016/08/22
	/*
		- error messages don't give line information, should track line and column number as we read
		- error don't give context information (what was being parsed, what invalid character was encountered, etc)
		- currently throws it's own MasterConfigException type - should probably be unified with whatever other
		  masterconfig parsing code does
		- doesn't handle many advanced xml concepts which are technically valid inside masterconfig.xml:
			* namespaces
			* entities
			* references
			* standalone declaration
			* processing instructions
			* CData
		- fragment and package exceptions are completely ignored right now in order to resolve these
		  we need give MasterConfigReader a way to resolve properties andd expressions (callbacks?)
		  and then update operations need to find the relevant package references based on the resolved
		  exceptions
		- doesn't check full range of valid name characters for attributes or element names
		- encoding attribute in xml declaration isn't used, just quickly verified for validity
	*/

	public class MasterConfigUpdater
	{
		public class MasterConfigException : Exception
		{
			public MasterConfigException(string message)
				: base(message)
			{         
			}
		}

		public class MasterConfigPackage
		{
			// track "active" references, for a given package it may have multiple references spread across different
			// fragments or the main masterconfig this is valid as long fragments specify allowoverride=true when
			// they override the version OR if they version is the same as previous definiition
			private List<MasterConfigElementReference> m_references = new List<MasterConfigElementReference>();

			internal MasterConfigPackage(string packageName, MasterConfigElement element, MasterConfigBaseDocument document, int line, int column)
			{
				UpdateReference(packageName, element, document, line, column);
			}

			public string GetAttributeValue(string attributeName)
			{
				MasterConfigElement firstReference = m_references.First().Element;
				MasterConfigAttribute attr = firstReference.GetAttribute(attributeName);
				if (attr != null)
				{
					return attr.Value;
				}
				return null;
			}

			public void AddOrUpdateAttribute(string name, string value)
			{
				foreach (MasterConfigElementReference reference in m_references)
				{
					MasterConfigAttribute packageVersionAttribute = reference.Element.GetAttribute(name);
					if (packageVersionAttribute == null)
					{
						reference.Document.Modified = true;
						reference.Element.AddAttribute(name, value);
					}
					else if (packageVersionAttribute.Value != value)
					{
						reference.Document.Modified = true;
						packageVersionAttribute.Value = value;
					}
				}
			}

			public bool RemoveAttribute(string attributeName)
			{
				bool anyReferenceRemoved = false;
				foreach (MasterConfigElementReference reference in m_references)
				{
					if (reference.Element.RemoveAttribute(attributeName))
					{
						anyReferenceRemoved = true;
						reference.Document.Modified = true;
					}
				}
				return anyReferenceRemoved;
			}

			internal void UpdateReference(string packageName, MasterConfigElement element, MasterConfigBaseDocument document, int line, int column)
			{
				// create and validate referemce
				MasterConfigElementReference newReference = new MasterConfigElementReference(element, document);
				MasterConfigAttribute packageVersionAttribute = newReference.Element.GetAttribute("version");
				if (packageVersionAttribute == null)
				{
					throw new MasterConfigException(String.Format("Package '{0}' at {1} missing required attribute 'version'!", packageName, MasterConfigReader.FormatLocation(line, column, document)));
				}
				string newVersion = packageVersionAttribute.Value;
				string invalidVersionNameReason = null;
				if (!NAntUtil.IsPackageVersionStringValid(newVersion, out invalidVersionNameReason))
				{
					throw new MasterConfigException(String.Format("Package '{0}' at {1} has invalid version name '{2}'. {3}.", packageName, MasterConfigReader.FormatLocation(line, column, document), newVersion, invalidVersionNameReason));
				}

				// update active reference list, if this is the first reference then lastReference will be null and
				// we just take the new reference. If this package is allowed to override previous versions then
				// we discard previous references and just accept new reference. In remaining case we have multiple
				// non-override references which is valid provided all of them resolve to same package version in which
				// case we append them to reference list
				MasterConfigElementReference lastReference = m_references.LastOrDefault();
				if (lastReference != null)
				{
					// check the last reference wasn't from same document, package can only be defined once per document
					if (newReference.Document == lastReference.Document)
					{
						throw new MasterConfigException(String.Format("More than one definition of package '{0}'! Duplicate definition at {1}.", packageName, MasterConfigReader.FormatLocation(line, column, document)));
					}

					// check for override or version match
					MasterConfigAttribute overrideAttribute = newReference.Element.GetAttribute("allowoverride");
					bool allowedToOverride = overrideAttribute != null && overrideAttribute.Value.ToLowerInvariant() == "true";
					if (allowedToOverride)
					{
						m_references.Clear(); // clear previous references as this overrides them             
					}
					else if (lastReference.Element.GetAttribute("version").Value != newVersion)
					{
						throw new MasterConfigException(String.Format("Fragment '{0}' has mismatched version for package '{1}' from '{2}' and does not specify 'allowoverride=\"true\"'.", newReference.Document.FilePath, packageName, lastReference.Document.FilePath));
					}
					m_references.Add(newReference);
				}
				else
				{
					m_references.Add(newReference);
				}
			}

			// TOOO exception evaluation
			// dvaliant 2016/09/11
			/*
			leaving the below code in but commented out for now, these two functions (with some work) can be brought in instead
			of UpdateReference if we want to be able evaluate masterconfig exceptions (given some method to evaluate properties)
			as we might want to load masterconfig prior to having context for resolving properties
			
			internal void AddReference(MasterConfigElement element, MasterConfigBaseDocument document)
			{
				m_references.Add(new PackageReference(element, document));
			}
			
			// package version may be overridden by fragments, however if fragments match and allowoverride
			// isn't specified then multiple matches are valid
			private IEnumerable<PackageReference> GetActiveReferences()
			{
				// references are stored in fragment declaration order, iterate in reverse order
				// to start with last override
				string firstValue = null;
				foreach (PackageReference reference in m_references.Reverse<PackageReference>())
				{
					MasterConfigAttribute packageVersionAttribute = reference.Element.GetAttribute("version");
					MasterConfigAttribute overrideAttribute = reference.Element.GetAttribute("allowoverride");
					if (firstValue == null)
					{
						// first time through this loop we record the last version set for this package
						firstValue = packageVersionAttribute.Value;
						yield return reference;
					}
					else if (packageVersionAttribute.Value == firstValue)
					{
						// if we got here then we haven't hit an override yet, in this case duplicate versions must be equal
						// and so this reference should also be updated
						yield return reference;
					}
					else
					{
						// non override duplicates did not match, masterconfig must be invalid
						throw new MasterConfigException("Fragment has mismatched version!");
					}

					// if this version is allowed to override, we don't need to worry about what earlier versions are set to
					if (overrideAttribute != null && overrideAttribute.Value.ToLowerInvariant() == "true")
					{
						break;
					}
				}

				yield break;
			}*/
		}

		public class MasterConfigGroupType
		{
			// track references, for a given grouptype it may have multiple references spread across different
			// fragments or the main masterconfig
			private List<MasterConfigElementReference> m_references = new List<MasterConfigElementReference>();

			internal MasterConfigGroupType(string groupTypeName, MasterConfigElement element, MasterConfigBaseDocument document)
			{
				UpdateReference(groupTypeName, element, document);
			}

			public string GetAttributeValue(string attributeName)
			{
				MasterConfigElement firstReference = m_references.First().Element;
				MasterConfigAttribute attr = firstReference.GetAttribute(attributeName);
				if (attr != null)
				{
					return attr.Value;
				}
				return null;
			}

			public void AddOrUpdateAttribute(string name, string value)
			{
				foreach (MasterConfigElementReference reference in m_references)
				{
					MasterConfigAttribute groupTypeAttribute = reference.Element.GetAttribute(name);
					if (groupTypeAttribute == null)
					{
						reference.Document.Modified = true;
						reference.Element.AddAttribute(name, value);
					}
					else if (groupTypeAttribute.Value != value)
					{
						reference.Document.Modified = true;
						groupTypeAttribute.Value = value;
					}
				}
			}

			internal void AddElement(MasterConfigElement addElement)
			{
				// only add new element to the first reference of a group
				MasterConfigElementReference reference = m_references.First();
				MasterConfigElement element = reference.Element;
				reference.Document.Modified = true;
				reference.Element.AddElementAfterSiblings(addElement);
			}

			internal void RemoveElement(Func<MasterConfigElement, bool> matchFunc)
			{
				foreach (MasterConfigElementReference reference in m_references)
				{
					MasterConfigElement element = reference.Element;
					if (element.RemoveElement(matchFunc))
					{
						reference.Document.Modified = true;
					}
				}
			}

			public bool RemoveAttribute(string attributeName)
			{
				bool anyReferenceRemoved = false;
				foreach (MasterConfigElementReference reference in m_references)
				{
					if (reference.Element.RemoveAttribute(attributeName))
					{
						anyReferenceRemoved = true;
						reference.Document.Modified = true;
					}
				}
				return anyReferenceRemoved;
			}

			internal void UpdateReference(string groupTypeName, MasterConfigElement element, MasterConfigBaseDocument document)
			{
				MasterConfigElementReference newReference = new MasterConfigElementReference(element, document);
				m_references.Add(newReference);
			}
		}

		public class MasterConfigPackageRoot
		{
			private List<MasterConfigElementReference> m_references = new List<MasterConfigElementReference>();

			internal MasterConfigPackageRoot(MasterConfigElement element, MasterConfigBaseDocument document)
			{
				UpdateReference(element, document);
			}

			internal void UpdateReference(MasterConfigElement element, MasterConfigBaseDocument document)
			{
				m_references.Add(new MasterConfigElementReference(element, document));
			}

			public void AddAttribute(string attrName, string attrValue)
			{
				foreach (MasterConfigElementReference reference in m_references)
				{
					reference.Element.AddAttribute(attrName, attrValue);
					reference.Document.Modified = true;
				}
			}
		}

		private class MasterConfigElementReference
		{
			internal readonly MasterConfigElement Element;
			internal readonly MasterConfigBaseDocument Document;

			internal MasterConfigElementReference(MasterConfigElement element, MasterConfigBaseDocument document)
			{
				Element = element;
				Document = document;
			}
		}

		internal class MasterConfigElement
		{
			internal readonly string Name;

			private readonly List<MasterConfigAttribute> m_attributes;
			private readonly List<object> m_content;     // handle types at runtime, for our purpose we only care about two types of content:
														 // string - whitespace / text content / comments
														 // MasterConfigElement - a nested element

			private string m_whiteSpaceAfterNameOrAttributes; // element names can have trailng whitespace: e.g. <element        > or <element    />
			private string m_whiteSpaceAfterEndTag;           // element end tags can also have trailing whitespace: e.g. </element       > 
			private bool m_empty;							  // true if uses <empty/> syntax

			internal MasterConfigElement(string name, List<MasterConfigAttribute> attributes, List<object> content, string whiteSpaceAfterName, bool empty, string whiteSpaceAfterEndTag = null)
			{
				Name = name;
				m_attributes = attributes;
				m_content = content;
				m_whiteSpaceAfterNameOrAttributes = whiteSpaceAfterName;
				m_whiteSpaceAfterEndTag = whiteSpaceAfterEndTag;
				m_empty = empty;
			}

			internal MasterConfigAttribute GetAttribute(string attributeName)
			{
				return m_attributes.FirstOrDefault(attr => attr.Name == attributeName);
			}

			internal void AddAttribute(string attributeName, string value)
			{
				char mostCommonQuote = FindMostCommon(m_attributes.Select(attr => attr.QuoteType), '"');
				string mostCommonPreceedingWhiteSpace = FindMostCommon(m_attributes.Select(attr => attr.PreceedingwhiteSpace), " ");
				string mostCommonPreEqualsWhiteSpace = FindMostCommon(m_attributes.Select(attr => attr.PreEqualsWhiteSpace), "");
				string mostCommonPostEqualsWhiteSpace = FindMostCommon(m_attributes.Select(attr => attr.PostEqualsWhiteSpace), "");
				m_attributes.Add(new MasterConfigAttribute(attributeName, value, mostCommonQuote, mostCommonPreceedingWhiteSpace, mostCommonPreEqualsWhiteSpace, mostCommonPostEqualsWhiteSpace));
			}

			internal bool RemoveAttribute(string attributeName)
			{
				return m_attributes.RemoveAll(attr => attr.Name == attributeName) > 0;
			}

			internal string GetInnerText()
			{
				if (m_empty)
				{
					return null;
				}

				StringWriter innerTextBuilder = new StringWriter();
				WriteContentTo(innerTextBuilder);
				return innerTextBuilder.ToString();
			}

			internal void WriteTo(StringWriter destination)
			{
				// open start tag
				destination.Write('<');
				destination.Write(Name);

				// write attributes (each attribute tracks its leading whitespace)
				foreach (MasterConfigAttribute attr in m_attributes)
				{
					attr.WriteTo(destination);
				}
				destination.Write(m_whiteSpaceAfterNameOrAttributes); // trailing space

				// close start tag or, if empty tag, close element and finish
				if (m_empty)
				{
					destination.Write("/>");
					return;
				}
				destination.Write('>');

				// write contemt
				WriteContentTo(destination);

				// write end tag
				destination.Write("</");
				destination.Write(Name);
				destination.Write(m_whiteSpaceAfterEndTag);
				destination.Write('>');
			}

			internal void AddElementAfterSiblings(MasterConfigElement newElement)
			{   
				// if this was an empty element, handle specially
				if (m_empty)
				{
					m_empty = false;
					// TODO find command end tag whitespace
					// dvaliant 26/01/2106
					// using empty string, but really we should look at siblings for common end tag
					m_whiteSpaceAfterEndTag = String.Empty;

					// don't make any assumptions about layout if element was previously empty
					// TODO look at sibling layout
					// dvaliant 26/01/2106
					// in theory we might have siblings we can look at for layout conventions
					m_content.Add(newElement);

					return;
				}

				// determine last element in comment, we want to insert after it
				int lastElementIndex = m_content.Count;
				while (lastElementIndex > 0)
				{
					if (m_content[lastElementIndex - 1] is MasterConfigElement)
					{
						break;
					}
					lastElementIndex -= 1;
				}

				// insertIndex is 0, it means we only found string elements,
				// assume we shoukd insert after
				if (lastElementIndex == 0)
				{
					// TODO find better whitespace handling
					// dvaliant 26/01/2106
					// could do a better job here, by looking at text do determine whitespace
					// and looking at siblings to determine conventions
					m_content.Add(newElement);
				}

				// build up list of whitespace patterns that preceed elements
				List<string> preceedingWhiteSpaces = new List<string>();
				for (int i = 1; i < m_content.Count; ++i) // start at index 1 because we are looking for preceeding whitespace blocks so there is no reason to consider 0th element
				{
					if (m_content[i] is MasterConfigElement && m_content[i - 1] is String)
					{
						preceedingWhiteSpaces.Add(GetTrailingWhiteSpace(m_content[i - 1] as String));
					}
				}
				
				// find most common preceeding whitespace and insert new whitespace then new element
				string mostCommonPreceedingWhiteSpace = FindMostCommon(preceedingWhiteSpaces, null);
				if (mostCommonPreceedingWhiteSpace == null)
				{
					m_content.Insert(lastElementIndex, newElement);
				}
				else
				{
					m_content.Insert(lastElementIndex, mostCommonPreceedingWhiteSpace);
					m_content.Insert(lastElementIndex + 1, newElement);
				}
			}

			internal bool RemoveElement(Func<MasterConfigElement, bool> matchFunc)
			{
				// if this was an empty element, handle specially
				if (m_empty)
				{
					return false;
				}

				// store current content and clear existing - we readd every element if
				// uunless it matches remove criteria
				object[] oldContent = m_content.ToArray();
				m_content.Clear();

				// remove elements that match removal criteria function, string content
				// will never be contiguous but elements can be - if we encounter a string
				// store it and add it only if followig element is preserved or if it is
				// the final piece of content
				bool foundMatch = false;
				string preceedingContent = null;
				foreach (object content in oldContent)
				{
					if (content is string)
					{
						preceedingContent = (string)content;
					}
					if (content is MasterConfigElement)
					{
						MasterConfigElement element = (MasterConfigElement)(content);
						if (matchFunc(element))
						{
							foundMatch = true;
						}
						else
						{
							if (preceedingContent != null)
							{
								m_content.Add(preceedingContent);
							}
							m_content.Add(content);
						}
						preceedingContent = null;
					}
				}
				if (preceedingContent != null)
				{
					// if last content entry was string append to end
					m_content.Add(preceedingContent);
				}
				return foundMatch;
			}

			internal string GetChildrenMostCommonContentPreceedingWhiteSpace(string childNameFilter = null)
			{
				return FindMostCommon(FilteredChildren(childNameFilter).Select(child => child.GetContentLeadingWhiteSpace()).Where(whitespace => whitespace != null), String.Empty);
			}

			internal string GetChildrenMostCommonContentTrailingWhiteSpace(string childNameFilter = null)
			{
				return FindMostCommon(FilteredChildren(childNameFilter).Select(child => child.GetContentTrailingWhiteSpace()).Where(whitespace => whitespace != null), String.Empty);
			}

			internal string GetChildrenMostCommonNameTrailingWhiteSpace(string childNameFilter = null)
			{
				return FindMostCommon(FilteredChildren(childNameFilter).Select(child => child.m_whiteSpaceAfterNameOrAttributes), String.Empty);
			}

			internal string GetChildrenMostCommonEndTagTrailingWhiteSpace(string childNameFilter = null)
			{
				return FindMostCommon(FilteredChildren(childNameFilter).Select(child => child.m_whiteSpaceAfterEndTag).Where(whitespace => whitespace != null), String.Empty);
			}

			private IEnumerable<MasterConfigElement> FilteredChildren(string childNameFilter)
			{
				IEnumerable<MasterConfigElement> elementChildren = m_content.Select(content => content as MasterConfigElement).Where(content => content != null);
				if (childNameFilter == null)
				{
					return elementChildren;
				}
				return elementChildren.Where(element => element.Name == childNameFilter);
			}

			private T FindMostCommon<T>(IEnumerable<T> properties, T defaultValue = default(T))
			{
				// if none return default value
				if (!properties.Any())
				{
					return defaultValue;
				}

				// count occurences of each value
				Dictionary<T, int> occurences = new Dictionary<T, int>();
				foreach(T property in properties)
				{
					if (occurences.ContainsKey(property))
					{
						occurences[property] += 1;
					}
					else
					{
						occurences[property] = 1;
					}
				}

				// find most common but so it reverse order - if multiple
				// most common favour later elements
				T mostCommon = properties.Last();
				int number = occurences[mostCommon];
				foreach (T property in properties.Reverse().Skip(1))
				{
					if (occurences[property] > number)
					{
						number = occurences[property];
						mostCommon = property;
					}
				}
				return mostCommon;
			}

			private string GetContentLeadingWhiteSpace()
			{
				string startingStringContent = m_content.FirstOrDefault() as String;
				if (startingStringContent != null)
				{
					return GetLeadingWhiteSpace(startingStringContent);
				}
				return null;
			}

			private string GetContentTrailingWhiteSpace()
			{
				string trailingStringContent = m_content.LastOrDefault() as String;
				if (trailingStringContent != null)
				{
					return GetTrailingWhiteSpace(trailingStringContent);
				}
				return null;
			}

			private void WriteContentTo(StringWriter destination)
			{
				foreach (object contentType in m_content)
				{
					if (contentType is String)
					{
						destination.Write((String)contentType);
					}
					else if (contentType is MasterConfigElement)
					{
						((MasterConfigElement)contentType).WriteTo(destination);
					}
					else
					{
						throw new ApplicationException("Internal error. Invalid content type."); // shouldn't get here, if we do this is a code problem - not user error
					}
				}
			}

			private static string GetLeadingWhiteSpace(string content)
			{
				for (int i = 0; i < content.Length; ++i)
				{
					if (!Char.IsWhiteSpace(content[i]))
					{
						return content.Substring(0, i);
					}
				}
				return content;
			}

			private static string GetTrailingWhiteSpace(string content)
			{
				for (int i = content.Length - 1; i >= 0; --i)
				{
					if (!Char.IsWhiteSpace(content[i]))
					{
						return content.Substring(i + 1);
					}
				}
				return content;
			}
		}

		internal class MasterConfigAttribute
		{
			internal string Value;

			internal readonly string Name;
			internal readonly char QuoteType;
			internal readonly string PreceedingwhiteSpace;
			internal readonly string PreEqualsWhiteSpace;
			internal readonly string PostEqualsWhiteSpace;

			internal MasterConfigAttribute(string name, string value, char quote, string preceedingWhiteSpace, string preEqualsWhiteSpace, string postEqualsWhiteSpace)
			{
				Name = name;
				Value = value;
				QuoteType = quote;
				PreceedingwhiteSpace = preceedingWhiteSpace;
				PreEqualsWhiteSpace = preEqualsWhiteSpace;
				PostEqualsWhiteSpace = postEqualsWhiteSpace;
			}

			internal void WriteTo(StringWriter destination)
			{
				destination.Write(PreceedingwhiteSpace);
				destination.Write(Name);
				destination.Write(PreEqualsWhiteSpace);
				destination.Write('=');
				destination.Write(PostEqualsWhiteSpace);
				destination.Write(QuoteType);
				destination.Write(MasterConfigReader.EscapeXml(Value));
				destination.Write(QuoteType);
			}
		}

		internal abstract class MasterConfigBaseDocument
		{
			public readonly string FilePath;

			internal bool Modified = false;
			internal ReadOnlyCollection<MasterConfigFragmentDocument> Fragments;

			protected MasterConfigElement m_root;
			protected MasterConfigElement m_packageRootsElement;
			protected MasterConfigElement m_masterVersionsElement;
			protected List<MasterConfigFragmentDocument> m_fragments;

			private string m_prolog;
			private string m_trailingMisc;

			internal MasterConfigBaseDocument(string filePath)
			{
				FilePath = filePath;
			}

			public override string ToString()
			{
				StringWriter documentWriter = new StringWriter();
				documentWriter.Write(m_prolog);
				m_root.WriteTo(documentWriter);
				documentWriter.Write(m_trailingMisc);
				return documentWriter.ToString();
			}

			internal string ResolveAndNormalizeRootPath(string rootPath)
			{
				if (Path.IsPathRooted(rootPath))
				{
					// path is absolute
					return PathNormalizer.Normalize(rootPath);
				}
				else
				{
					// path is relative, resolve against masterconfig location
					return PathNormalizer.Normalize(Path.Combine(Path.GetDirectoryName(FilePath), rootPath));
				}
			}

			protected void Initalize(string prolog, MasterConfigElement root, MasterConfigElement packageRoots, MasterConfigElement masterVersions, List<MasterConfigFragmentDocument> fragments, string trailingMisc)
			{
				// record content
				m_root = root;
				m_prolog = prolog;
				m_trailingMisc = trailingMisc;
				m_fragments = fragments;

				Fragments = new ReadOnlyCollection<MasterConfigFragmentDocument>(m_fragments);

				// record special reference to content (for editing)
				m_packageRootsElement = packageRoots;
				m_masterVersionsElement = masterVersions;
			}
		}

		private class MasterConfigDocument : MasterConfigBaseDocument
		{
			internal ReadOnlyDictionary<string, MasterConfigPackage> Packages;
			internal ReadOnlyDictionary<string, MasterConfigGroupType> GroupTypes;
			internal ReadOnlyDictionary<string, MasterConfigPackageRoot> PackageRoots;

			private Dictionary<string, MasterConfigPackage> m_packages;
			private Dictionary<string, MasterConfigGroupType> m_groupTypes;
			private Dictionary<string, MasterConfigPackageRoot> m_packageRoots;

			internal MasterConfigDocument(string masterConfigPath)
				: base(masterConfigPath)
			{
			}

			internal void Initalize(string prolog, MasterConfigElement root, MasterConfigElement packageRootsElement, MasterConfigElement masterVersionsElement, List<MasterConfigFragmentDocument> fragments, Dictionary<string, MasterConfigPackage> packages, Dictionary<string, MasterConfigGroupType> groupTypes, Dictionary<string, MasterConfigPackageRoot> packageRoots, string trailingMisc)
			{
				base.Initalize(prolog, root, packageRootsElement, masterVersionsElement, fragments, trailingMisc);

				m_packages = packages;
				m_groupTypes = groupTypes;
				m_packageRoots = packageRoots;

				Packages = new ReadOnlyDictionary<string, MasterConfigPackage>(m_packages);
				GroupTypes = new ReadOnlyDictionary<string, MasterConfigGroupType>(m_groupTypes);
				PackageRoots = new ReadOnlyDictionary<string, MasterConfigPackageRoot>(m_packageRoots);
			}

			internal MasterConfigPackageRoot AddPackageRoot(string packageRoot)
			{
				// TODO assuming package root order is irrelevant
				// dvaliant 26/01/2106
				// we're assuming package root being added here isn't a parent folder of an existing package root
				// if it were we would need to insert it before the child HOWEVER the better fix here is just to 
				// have framework sort the package roots automatically which would also fix some issues with 
				// fragments not being able to specify parent roots

				// check if root already exists
				string resolvedRoot = ResolveAndNormalizeRootPath(packageRoot);
				if (m_packageRoots.ContainsKey(resolvedRoot))
				{
					return m_packageRoots[resolvedRoot];
				}

				// we need to modify xml so mark modifed for saving
				Modified = true;

				// first create packageroots container element if required
				if (m_packageRootsElement == null)
				{
					string rootsNameTrailingWhiteSpace = m_root.GetChildrenMostCommonNameTrailingWhiteSpace();

					m_packageRootsElement = new MasterConfigElement("packageroots", new List<MasterConfigAttribute>(), new List<object>(), rootsNameTrailingWhiteSpace, true);
					m_root.AddElementAfterSiblings(m_packageRootsElement);
				}

				// add new package roots element
				string contentPreceedingWhiteSpace = m_packageRootsElement.GetChildrenMostCommonContentPreceedingWhiteSpace("packageroot");
				string contentTrailingWhiteSpace = m_packageRootsElement.GetChildrenMostCommonContentTrailingWhiteSpace("packageroot");
				string content = contentPreceedingWhiteSpace + packageRoot + contentTrailingWhiteSpace;

				string nameTrailingWhiteSpace = m_packageRootsElement.GetChildrenMostCommonNameTrailingWhiteSpace("packageroot");
				string endTagTrailingWhiteSpace = m_packageRootsElement.GetChildrenMostCommonEndTagTrailingWhiteSpace("packgeroot");

				MasterConfigElement newPackageRootElement = new MasterConfigElement("packageroot", new List<MasterConfigAttribute>(), new List<object>() { content }, nameTrailingWhiteSpace, false, endTagTrailingWhiteSpace);
				m_packageRootsElement.AddElementAfterSiblings(newPackageRootElement);

				// update element tracking
				MasterConfigPackageRoot newPackageRoot = new MasterConfigPackageRoot(newPackageRootElement, this);
				m_packageRoots.Add(resolvedRoot, newPackageRoot);

				return newPackageRoot;
			}

			internal MasterConfigPackage AddPackage(string package, string version, string uri = null, string groupType = null)
			{
				// check if package already exists
				if (m_packages.ContainsKey(package))
				{
					return m_packages[package];
				}

				// we need to modify xml so mark modifed for saving
				Modified = true;

				string rootsNameTrailingWhiteSpace = m_root.GetChildrenMostCommonNameTrailingWhiteSpace();
				// first create masterversions container element if required
				if (m_masterVersionsElement == null)
				{
					m_masterVersionsElement = new MasterConfigElement("masterversions", new List<MasterConfigAttribute>(), new List<object>(), rootsNameTrailingWhiteSpace, true);
					m_root.AddElementAfterSiblings(m_masterVersionsElement);
				}

				// add new package element
				string nameTrailingWhiteSpace = m_masterVersionsElement.GetChildrenMostCommonNameTrailingWhiteSpace("package");
				string endTagTrailingWhiteSpace = m_masterVersionsElement.GetChildrenMostCommonEndTagTrailingWhiteSpace("package");

				MasterConfigElement newPackageElement = new MasterConfigElement("package", new List<MasterConfigAttribute>(), new List<object>(), nameTrailingWhiteSpace, true, endTagTrailingWhiteSpace);
				newPackageElement.AddAttribute("name", package);
				newPackageElement.AddAttribute("version", version);

				if (uri != null)
				{
					newPackageElement.AddAttribute("uri", uri);
				}

				if (groupType == null)
				{
					m_masterVersionsElement.AddElementAfterSiblings(newPackageElement);
				}
				else
				{
					//find groupType
					MasterConfigGroupType groupTypeRef= null;
					if (!m_groupTypes.TryGetValue(groupType, out groupTypeRef))
					{
						//create groupType
						string groupTypeNameTrailingWhiteSpace = m_masterVersionsElement.GetChildrenMostCommonNameTrailingWhiteSpace("grouptype");
						string groupTypeEndTagTrailingWhiteSpace = m_masterVersionsElement.GetChildrenMostCommonEndTagTrailingWhiteSpace("grouptype");

						MasterConfigElement groupTypeElement = new MasterConfigElement("grouptype", new List<MasterConfigAttribute>(), new List<object>(), groupTypeNameTrailingWhiteSpace, true, groupTypeEndTagTrailingWhiteSpace);
						groupTypeElement.AddAttribute("name", groupType);
						groupTypeRef = new MasterConfigGroupType(groupType, groupTypeElement, this);

						m_masterVersionsElement.AddElementAfterSiblings(groupTypeElement);
						m_groupTypes.Add(groupType, groupTypeRef);
					}
					groupTypeRef.AddElement(newPackageElement);
				}

				// update package tracking
				MasterConfigPackage newPackage = new MasterConfigPackage(package, newPackageElement, this, 0, 0);
				m_packages.Add(package, newPackage);

				return null;
			}

			internal bool RemovePackage(string packageName)
			{
				// check if package exists
				if (!m_packages.ContainsKey(packageName))
				{
					return false;
				}

				// remove package if it exists
				MasterConfigPackage package = null;
				if (m_packages.TryGetValue(packageName, out package))
				{
					// we need to modify xml so mark modifed for saving
					Modified = true;

					Func<MasterConfigElement, bool> packageNameCheck = (element) =>
					{
						if (element.Name == "package")
						{
							MasterConfigAttribute nameAttr = element.GetAttribute("name");
							if (nameAttr != null)
							{
								return nameAttr.Value == packageName;
							}
						}
						return false;
					};

					m_masterVersionsElement.RemoveElement(packageNameCheck);
					foreach (KeyValuePair<string, MasterConfigGroupType> groupType in m_groupTypes)
					{
						m_groupTypes[groupType.Key].RemoveElement(packageNameCheck);
					}
					m_packages.Remove(packageName);
				}

				return Modified;
			}
		}

		internal class MasterConfigFragmentDocument : MasterConfigBaseDocument
		{
			internal MasterConfigFragmentDocument(string fragmentPath)
				: base(fragmentPath)
			{
			}

			internal new void Initalize(string prolog, MasterConfigElement root, MasterConfigElement packageRoots, MasterConfigElement masterVersionsElement, List<MasterConfigFragmentDocument> fragments, string trailingMisc)
			{
				base.Initalize(prolog, root, packageRoots, masterVersionsElement, fragments, trailingMisc);
			}
		}

		private sealed class MasterConfigReader
		{
			// accumulates references to special elements from main masterconfig and fragments we want to expose
			private class MasterConfigTopLevelReader
			{       
				internal readonly Dictionary<string, MasterConfigPackage> Packages = new Dictionary<string, MasterConfigPackage>();
				internal readonly Dictionary<string, MasterConfigGroupType> GroupTypes = new Dictionary<string, MasterConfigGroupType>();
				internal readonly Dictionary<string, MasterConfigPackageRoot> PackageRoots = new Dictionary<string, MasterConfigPackageRoot>();
                internal readonly Project Project = null;
                internal MasterConfigTopLevelReader(Project project)
                {
                    Project = project;
                }
            }

			public readonly MasterConfigBaseDocument Document;

			private string m_sourceFile;
			private int m_readPostion = 0;
			private int m_readColumn = 1;
			private int m_readLine = 1;
			private char m_lastAdvanceChar;

			private MasterConfigElement m_packageRootsElement = null;
			private MasterConfigElement m_masterVersionsElement = null;

			private readonly char[] m_charBuffer;

			private readonly bool m_isFragment;
			private readonly MasterConfigTopLevelReader m_topLevelReader;
			private readonly Stack<string> m_elementStack = new Stack<string>();
			private readonly List<string> m_fragmentPatterns = new List<string>();

            // main masterconfig constructor
            private MasterConfigReader(string masterConfigPath, Project project, List<string> cmdlineFragments)
                : this(masterConfigPath, false, new MasterConfigTopLevelReader(project), cmdlineFragments)
            {
            }

            // fragment constructor
            private MasterConfigReader(string masterConfigPath, MasterConfigTopLevelReader topLevelReader)
                : this(masterConfigPath, true, topLevelReader, null)
            {
            }

            private MasterConfigReader(string masterConfigPath, bool isFragment, MasterConfigTopLevelReader topLevelReader, List<string> cmdlineFragments)
            {
                m_topLevelReader = topLevelReader;
                m_isFragment = isFragment;
				m_sourceFile = masterConfigPath;
				m_charBuffer = File.ReadAllText(masterConfigPath).ToArray<char>();
				m_lastAdvanceChar = m_charBuffer.FirstOrDefault();

				// create blank documents, so child elements can reference them, initalized fully at end of function
				if (isFragment)
				{
					Document = new MasterConfigFragmentDocument(m_sourceFile);
				}
				else
				{
					Document = new MasterConfigDocument(m_sourceFile);
				}

				// handle pre root mark up
				string prolog = ReadProlog(); // copies prolog (if exists) and any trailing whitespace / comments

				// handle root element
				MasterConfigElement rootElement = ReadElement();
				if (rootElement == null)
				{
					throw new Exception("Missing root element!");
				}

				// handle trailing whitespace
				string trailingMisc = ReadMisc();

				List<MasterConfigFragmentDocument> fragmentDocuments = new List<MasterConfigFragmentDocument>();

				if (!isFragment && cmdlineFragments != null)
				{
					// only load cmdline fragments when processing the main masterconfig
					m_fragmentPatterns.AddRange(cmdlineFragments);
				}

				// resolve fragments
				foreach (string fragmentPattern in m_fragmentPatterns)
				{
					foreach (string fragmentPath in GetMatchingFragmentsFilePaths(fragmentPattern))
					{
						fragmentDocuments.Add(ReadFragment(fragmentPath));
					}
				}

				if (!isFragment)
				{
					((MasterConfigDocument)Document).Initalize(prolog, rootElement, m_packageRootsElement, m_masterVersionsElement, fragmentDocuments, m_topLevelReader.Packages, m_topLevelReader.GroupTypes, m_topLevelReader.PackageRoots, trailingMisc);
				}
				else
				{
					((MasterConfigFragmentDocument)Document).Initalize(prolog, rootElement, m_packageRootsElement, m_masterVersionsElement, fragmentDocuments, trailingMisc);
				}
			}

			internal static MasterConfigDocument ReadMasterConfig(string masterconfigPath, Project project, List<string> cmdlineFragments)  
			{
				return (MasterConfigDocument)(new MasterConfigReader(masterconfigPath, project, cmdlineFragments).Document);
			}

			internal static string EscapeXml(string text)
			{
				XmlDocument doc = new XmlDocument();
				XmlNode node = doc.CreateElement("root");
				node.InnerText = text;
				return node.InnerXml;
			}

			internal static string UnescapeXml(string xml)
			{
				XmlDocument doc = new XmlDocument();
				XmlNode node = doc.CreateElement("root");
				node.InnerXml = xml;
				return node.InnerText;
			}

			internal static string FormatLocation(int line, int column, MasterConfigBaseDocument document)
			{
				return String.Format("line {0} (column {1}) in '{2}'", line, column, document.FilePath);
			}

			private string ReadProlog()
			{
				// xml declaration
				StringWriter prologBuilder = new StringWriter();
				if (Peek(5) == "<?xml")
				{
					AdvanceAndWrite(prologBuilder, "<?xml");
					string postNameAttributesWhiteSpace = ReadWhiteSpace();

					// version attribute
					MasterConfigAttribute versionAttribute = ReadAttribute(postNameAttributesWhiteSpace, isXmlDeclaration: true);
					if (versionAttribute == null)
					{
						throw new MasterConfigException("XML declaration missing 'version'!");
					}
					if (versionAttribute.Name != "version")
					{
						throw new MasterConfigException("First attribute in XML declaration must be 'version'!");
					}

					// validate version value
					if (versionAttribute.Value.Length <= 2 || versionAttribute.Value.Substring(0, 2) != "1.")
					{
						throw new MasterConfigException("Invalid version in XML declaration.");
					}
					if (versionAttribute.Value.ToArray<char>().Skip(2).Any(c => !Char.IsDigit(c)))
					{
						throw new MasterConfigException("Invalid version in XML declaration."); // everything after "1." should be digit
					}

					versionAttribute.WriteTo(prologBuilder);
					postNameAttributesWhiteSpace = ReadWhiteSpace();

					// read optional encoding attribute  
					MasterConfigAttribute encodingAttribute = ReadAttribute(postNameAttributesWhiteSpace, isXmlDeclaration: true);
					if (encodingAttribute != null)
					{
						// validate attribute order
						if (encodingAttribute.Name != "encoding")
						{
							throw new MasterConfigException("Second attribute in XML declaration must be 'encoding'!");
						}

						// validate encoding value
						string encodingValue = encodingAttribute.Value;
						if (encodingValue.Length == 0)
						{
							throw new MasterConfigException("XML declaration encoding cannot have empty value!");
						}
						char firstEncodingChar = encodingValue.First();
						if (!Char.IsLetter(firstEncodingChar))
						{
							throw new MasterConfigException("XML declaration encoding must begin with character!");
						}
						foreach (char encodingChar in encodingValue.Skip(1))
						{
							if (!Char.IsLetter(encodingChar) && !Char.IsDigit(encodingChar) && encodingChar != '.' && encodingChar != '_' && encodingChar != '-')
							{
								throw new MasterConfigException("Invalid character in XML declaration encoding attribute!");
							}
						}

						encodingAttribute.WriteTo(prologBuilder);
						postNameAttributesWhiteSpace = ReadWhiteSpace();

						// check there is no third attribute
						MasterConfigAttribute invalidAttrbiute = ReadAttribute(postNameAttributesWhiteSpace, isXmlDeclaration: true);
						if (invalidAttrbiute != null)
						{
							throw new MasterConfigException("Invalid additional attribute in XML decalaration!");
						}
					}

					// whitespace between attributes and end of element
					prologBuilder.Write(postNameAttributesWhiteSpace);

					if (Peek(2) != "?>")
					{
						throw new MasterConfigException("Invalid XML declaration.");
					}
					AdvanceAndWrite(prologBuilder, "?>");
				}

				// prolog trailing white sapce
				prologBuilder.Write(ReadMisc());

				return prologBuilder.ToString();
			}

			private MasterConfigElement ReadElement()
			{
				// record line and column of element start
				int line = m_readLine;
				int column = m_readColumn;

				// read name
				string elementName = ReadElementStartName();
				if (elementName == null)
				{
					return null;
				}

				// now inside element push onto stack
				m_elementStack.Push(elementName);

				// read attributes
				List<MasterConfigAttribute> attributes = new List<MasterConfigAttribute>();
				string postNameAttributesWhiteSpace = ReadWhiteSpace();
				MasterConfigAttribute attr = ReadAttribute(postNameAttributesWhiteSpace);
				while (attr != null)
				{
					attributes.Add(attr);
					postNameAttributesWhiteSpace = ReadWhiteSpace();
					attr = ReadAttribute(postNameAttributesWhiteSpace);
				}

				// read end of start tag
				if (Peek(2) == "/>")
				{
					// if empty element, return element with no content
					Advance(2);
					MasterConfigElement emptyElement = new MasterConfigElement(elementName, attributes, new List<object>(), postNameAttributesWhiteSpace, empty: true);
					HandleSpecialElement(emptyElement, line, column);
					
					// done reading element, pop stack
					m_elementStack.Pop();

					return emptyElement;
				}
				else if (Peek() != '>')
				{
					throw new MasterConfigException(String.Format("Unexpected character! Expected closing '>' or '/> for element '{0}' start tag at {1}.", elementName, FormatLocation(line, column, Document)));
				}
				else
				{
					Advance();
				}

				// read content
				List<object> content = new List<object>();
				while (true)
				{
					// try and read text content
					string textCommentWhiteSpace = ReadCharDataAndMisc();
					bool hasContent = textCommentWhiteSpace != String.Empty;
					if (hasContent)
					{
						content.Add(textCommentWhiteSpace);
					}

					// try and read element
					MasterConfigElement nestedElement = ReadElement();
					bool hasElement = nestedElement != null;
					if (hasElement)
					{
						content.Add(nestedElement);
					}

					// if neither match next element it should be end tag
					if (!hasContent && !hasElement)
					{
						break;
					}
				}

				// read end tag
				if (Peek(2) != "</")
				{
					throw new MasterConfigException("Unexpected character!");
				}
				Advance(2);

				if (Peek(elementName.Length) != elementName)
				{
					throw new MasterConfigException("Mismatched end tag!");
				}
				Advance(elementName.Length);

				string postEndTagWhiteSpace = ReadWhiteSpace();

				if (Peek() != '>')
				{
					throw new MasterConfigException("Unexpected character!");
				}
				Advance();

				// assemble element
				MasterConfigElement element = new MasterConfigElement(elementName, attributes, content, postNameAttributesWhiteSpace, false, postEndTagWhiteSpace);
				HandleSpecialElement(element, line, column);

				// done reading element, pop stack
				m_elementStack.Pop();

				return element;
			}

			private void HandleSpecialElement(MasterConfigElement element, int line, int column)
			{
				// track fragments
				if (StackIs("project", "fragments", "include"))
				{
					MasterConfigAttribute includeNameAttribute = element.GetAttribute("name");
					if (includeNameAttribute == null)
					{
						throw new MasterConfigException(String.Format("Fragment include missing required attribute 'name' at {0}!", FormatLocation(line, column, Document)));
					}
                    MasterConfigAttribute conditionalIfAttribute = element.GetAttribute("if");
                    if (conditionalIfAttribute == null)
                    {
                        m_fragmentPatterns.Add(includeNameAttribute.Value);
                    }
                    else
                    {
                        bool includeFragment = Expression.Evaluate(m_topLevelReader.Project.ExpandProperties(conditionalIfAttribute.Value));   
                        if (includeFragment)
                        {
                            m_fragmentPatterns.Add(includeNameAttribute.Value);
                        }
                    }
				}

				// track packages
				else if (StackIs("project", "masterversions", "package") || StackIs("project", "masterversions", "grouptype", "package"))
				{
					// validate package name
					MasterConfigAttribute packageNameAttribute = element.GetAttribute("name");
					if (packageNameAttribute == null)
					{
						throw new MasterConfigException(String.Format("Package element missing required attribute 'name' at {0}!", FormatLocation(line, column, Document)));
					}
					string packageName = packageNameAttribute.Value;
					string invalidNameReason = null;
					if (!NAntUtil.IsPackageNameValid(packageName, out invalidNameReason))
					{
						throw new MasterConfigException(String.Format("Package name '{0}' at {1} is invalid. {2}.", packageName, MasterConfigReader.FormatLocation(line, column, Document), invalidNameReason));
					}

					// add to top level reader accumulator
					MasterConfigPackage package = null;
					if (!m_topLevelReader.Packages.TryGetValue(packageName, out package))
					{
						package = new MasterConfigPackage(packageName, element, Document, line, column);
						m_topLevelReader.Packages[packageName] = package;
					}
					else
					{
						package.UpdateReference(packageName, element, Document, line, column);
					}
				}

				// track masterversions
				else if (StackIs("project", "masterversions"))
				{
					if (m_masterVersionsElement != null)
					{
						throw new MasterConfigException(String.Format("masterversions multiply defined at {0}!", FormatLocation(line, column, Document)));
					}
					else
					{
						m_masterVersionsElement = element;
					}
				}

				// track grouptypes
				else if (StackIs("project", "masterversions", "grouptype"))
				{
					// validate grouptype name
					MasterConfigAttribute groupTypeNameAttribute = element.GetAttribute("name");
					if (groupTypeNameAttribute == null)
					{
						throw new MasterConfigException(String.Format("Grouptype element missing required attribute 'name' at {0}!", FormatLocation(line, column, Document)));
					}
					string groupTypeName = groupTypeNameAttribute.Value;

					// add to top level reader accumulator
					MasterConfigGroupType groupType = null;
					if (!m_topLevelReader.GroupTypes.TryGetValue(groupTypeName, out groupType))
					{
						groupType = new MasterConfigGroupType(groupTypeName, element, Document);
						m_topLevelReader.GroupTypes[groupTypeName] = groupType;
					}
					else
					{
						groupType.UpdateReference(groupTypeName, element, Document);
					}
				}

				// track package roots
				else if (StackIs("project", "packageroots"))
				{
					if (m_packageRootsElement != null)
					{
						throw new MasterConfigException(String.Format("Package roots multiply defined at {0}!", FormatLocation(line, column, Document)));
					}
					else
					{
						m_packageRootsElement = element;
					}
				}
				else if (StackIs("project", "packageroots", "packageroot"))
				{
					string rootPath = element.GetInnerText().TrimWhiteSpace();
					string resolvedRootPath = Document.ResolveAndNormalizeRootPath(rootPath);
										
					// add to top level reader accumulator
					MasterConfigPackageRoot root = null;
					if (!m_topLevelReader.PackageRoots.TryGetValue(resolvedRootPath, out root))
					{
						root = new MasterConfigPackageRoot(element, Document);
						m_topLevelReader.PackageRoots[resolvedRootPath] = root;
					}
					else
					{
						root.UpdateReference(element, Document);
					}
				}
			}

			// misc means white space, comments and processing instructions, currently we just handle whitespace and comments
			// misc can occur after root element so is the only function that handles end of documents
			private string ReadMisc()
			{
				StringWriter whiteSpaceAndCommentBuilder = new StringWriter();
				while (true)
				{
					// handle whitepsace
					whiteSpaceAndCommentBuilder.Write(ReadWhiteSpace());
					if (IsEndOfDocument())
					{
						return whiteSpaceAndCommentBuilder.ToString();
					}

					// handle comments
					if (Peek(4) != "<!--")
					{
						// next element was not a comment or whitespace
						break;
					}

					AdvanceAndWrite(whiteSpaceAndCommentBuilder, "<!--");

					while (true)
					{
						char peek = Peek();
						if (peek != '-')
						{
							AdvanceAndWrite(whiteSpaceAndCommentBuilder, peek);
						}
						else if (Peek(3) == "-->")
						{
							AdvanceAndWrite(whiteSpaceAndCommentBuilder, "-->");
							break;
						}
						else if (Peek(2) == "--")
						{
							throw new MasterConfigException("Invalid comment syntax!");
						}
						else
						{
							AdvanceAndWrite(whiteSpaceAndCommentBuilder, "-");
						}
					}
				}
				return whiteSpaceAndCommentBuilder.ToString();
			}

			private MasterConfigAttribute ReadAttribute(string preceedingWhiteSpace, bool isXmlDeclaration = false)
			{
				// check for valid start character or end of element charaacter
				char firstPeek = Peek();
				if (firstPeek == '/' || firstPeek == '>' || (isXmlDeclaration && firstPeek == '?'))
				{
					return null;
				}
				else if (IsNameStartChar(firstPeek))
				{
					if (preceedingWhiteSpace.Length == 0)
					{
						throw new MasterConfigException(String.Format("Unexpected character '{0}' at {1}! Expected whitespace before attribute.", firstPeek, FormatLocation(m_readLine, m_readColumn + 1, Document)));
					}
				}
				else
				{
					throw new MasterConfigException("Unexpected character!");
				}

				// read name
				string name = ReadName();

				// read whitespace between attribute name and equals
				string preEqualsWhiteSpace = ReadWhiteSpace();

				// validate attribute has equals between name and value
				if (Peek() != '=')
				{
					throw new MasterConfigException("Unexpected character!");
				}
				else
				{
					Advance();
				}

				// read whitespace between attribute name and equals
				string postEqualsWhiteSpace = ReadWhiteSpace();

				// read attribute value
				char quote = Peek();
				if (quote != '\'' && quote != '\"')
				{
					throw new MasterConfigException("Unexpected character!");
				}
				else
				{
					Advance();
				}
				StringWriter valueBuilder = new StringWriter();
				while (true)
				{
					char peek = Peek();
					if (peek == '<' || peek == '>')
					{
						throw new MasterConfigException("Unexpected character!");
					}
					else if (peek == quote)
					{
						Advance();
						break;
					}
					else
					{
						AdvanceAndWrite(valueBuilder, peek);
					}
				}

				return new MasterConfigAttribute
				(
					name,
					UnescapeXml(valueBuilder.ToString()),
					quote,
					preceedingWhiteSpace,
					preEqualsWhiteSpace,
					postEqualsWhiteSpace
				);
			}

			private string ReadElementStartName()
			{
				if (Peek() != '<')
				{
					return null;
				}
				else if (Peek(2) == "</")
				{
					return null;
				}
				else
				{
					Advance();
				}
				return ReadName();
			}

			private string ReadName()
			{
				// first character has extra restrictions
				StringWriter nameBuilder = new StringWriter();
				char firstChar = Peek();
				if (!IsNameStartChar(firstChar))
				{
					throw new MasterConfigException("Unexpected character at!");
				}
				AdvanceAndWrite(nameBuilder, firstChar);

				while (true)
				{
					char peek = Peek();
					if (IsNameChar(peek))
					{
						AdvanceAndWrite(nameBuilder, peek);
					}
					else
					{
						break;
					}
				}
				return nameBuilder.ToString();
			}

			private string ReadCharDataAndMisc()
			{
				StringWriter contentBuilder = new StringWriter();
				while (true)
				{
					contentBuilder.Write(ReadMisc());
					char peek = Peek();
					if (peek == '<')
					{
						if(Peek(4) != "<!--")
						{
							break; // not a comment, must be an element stop reading
						}
					}
					else
					{
						contentBuilder.Write(peek);
						Advance(1);
					}
				}           
				return contentBuilder.ToString();
			}

			private string ReadWhiteSpace()
			{
				StringWriter whiteSpaceBuilder = new StringWriter();
				while (m_readPostion < m_charBuffer.Length) // this is the only place we check for end of document as we always check for trailing whitespace last
				{
					char peek = Peek();
					if (Char.IsWhiteSpace(peek))
					{
						Advance();
						whiteSpaceBuilder.Write(peek);
					}
					else
					{
						break;
					}
				}
				return whiteSpaceBuilder.ToString();
			}

			// helper function, for consuming what has just been peeked while writing to copy destination
			private void AdvanceAndWrite(StringWriter destination, char value)
			{
				Advance();
				destination.Write(value);
			}

			// helper function, for consuming what has just been peeked while writing to copy destination
			private void AdvanceAndWrite(StringWriter destination, string value)
			{
				Advance(value.Length);
				destination.Write(value);
			}

			private void Advance(int offset = 1)
			{
				int newReadPostion = m_readPostion + offset;
				for (int i = m_readPostion ; i < newReadPostion; ++i)
				{
					char advanceChar = m_charBuffer[i];
					switch (advanceChar)
					{
						case '\n':
							if (m_lastAdvanceChar != '\r')
							{
								m_readColumn = 1;
								m_readLine += 1;
							}
							break;
						case '\r':
							m_readColumn = 1;
							m_readLine += 1;
							break;
						default:
							m_readColumn += 1;
							break;
					}
					m_lastAdvanceChar = advanceChar;
				}
				m_readPostion = newReadPostion;

			}

			private char Peek()
			{
				if (m_readPostion >= m_charBuffer.Length)
				{
					throw new MasterConfigException("Unexpected end of file!");
				}
				return m_charBuffer[m_readPostion];
			}

			private string Peek(int size)
			{
				if (m_readPostion + size >= m_charBuffer.Length)
				{
					throw new MasterConfigException("Unexpected end of file!");
				}
				return new string(m_charBuffer, m_readPostion, size);
			}

			private bool IsEndOfDocument()
			{
				return m_readPostion == m_charBuffer.Length;
			}

			private bool StackIs(params string[] elements)
			{
				if (m_elementStack.Count != elements.Length)
				{
					return false;
				}

				for (int i = 0; i < elements.Length; ++i)
				{
					if (m_elementStack.Reverse().ElementAt(i) != elements[i])
					{
						return false;
					}
				}

				return true;
			}

			private IEnumerable<string> GetMatchingFragmentsFilePaths(string fragmentPattern)
			{
				// convert file path to full path using fileset, for same behaviour as other masterconfig code
				FileSet fragmentFileSet = new FileSet();
				fragmentFileSet.BaseDirectory = Path.GetDirectoryName(m_sourceFile);
				fragmentFileSet.Include(fragmentPattern);
				return fragmentFileSet.FileItems.Select(fi => fi.FullPath);
			}

			private MasterConfigFragmentDocument ReadFragment(string fragmentPath)
			{
				return (MasterConfigFragmentDocument)(new MasterConfigReader(fragmentPath, m_topLevelReader).Document);
			}

			private static bool IsNameChar(char nameChar)
			{
				return IsNameStartChar(nameChar) || Char.IsDigit(nameChar) || nameChar == '-' || nameChar == '.'; // this is actually a subset of allowed name chars, but should be enough for masterconfig purposes, https://www.w3.org/TR/REC-xml/#NT-NameChar
			}

			private static bool IsNameStartChar(char startChar)
			{
				return Char.IsLetter(startChar) || startChar == ':' || startChar == '_'; // this is actually a subset of allowed name start chars, but should be enough for masterconfig purposes, https://www.w3.org/TR/REC-xml/#NT-NameStartChar
			}
		}

		private MasterConfigDocument m_masterconfigDoc;

		private MasterConfigUpdater(string filePath, Project project, List<string> cmdlineFragments)
		{
			m_masterconfigDoc = MasterConfigReader.ReadMasterConfig(filePath, project, cmdlineFragments);
		}

		public MasterConfigPackage GetPackage(string packageName)
		{
			MasterConfigPackage package = null;
			m_masterconfigDoc.Packages.TryGetValue(packageName, out package);
			return package;
		}

		public MasterConfigGroupType GetGroupType(string groupTypeName)
		{
			MasterConfigGroupType groupType = null;
			m_masterconfigDoc.GroupTypes.TryGetValue(groupTypeName, out groupType);
			return groupType;
		}

		public MasterConfigPackageRoot AddPackageRoot(string packageRoot)
		{
			return m_masterconfigDoc.AddPackageRoot(packageRoot);
		}

		public MasterConfigPackage AddPackage(string packageName, string version, string uri = null, string groupType = null)
		{
			return m_masterconfigDoc.AddPackage(packageName, version, uri, groupType);
		}

		public bool RemovePackage(string packageName)
		{
			return m_masterconfigDoc.RemovePackage(packageName);
		}

		private void SaveFragment(MasterConfigFragmentDocument fragment)
		{
			if (fragment.Modified)
			{
				fragment.Modified = false;
				File.WriteAllText(fragment.FilePath, fragment.ToString());
			}
			foreach (MasterConfigFragmentDocument subfragment in fragment.Fragments)
			{
				SaveFragment(subfragment);
			}
		}

		public void Save()
		{
			if (m_masterconfigDoc.Modified)
			{
				m_masterconfigDoc.Modified = false;
				File.WriteAllText(m_masterconfigDoc.FilePath, m_masterconfigDoc.ToString());
			}
			foreach (MasterConfigFragmentDocument fragment in m_masterconfigDoc.Fragments)
			{
				SaveFragment(fragment);
			}
		}

		private IEnumerable<string> GetFragmentPathsToModifyOnSave(MasterConfigFragmentDocument fragment)
		{
			if (fragment.Modified)
			{
				yield return fragment.FilePath;
			}
			foreach (MasterConfigFragmentDocument subfragment in fragment.Fragments)
			{
				foreach (string fragmentFilepaths in GetFragmentPathsToModifyOnSave(subfragment))
				{
					yield return fragmentFilepaths;
				}
			}
			yield break;
		}

		public IEnumerable<string> GetPathsToModifyOnSave()
		{
			if (m_masterconfigDoc.Modified)
			{
				yield return m_masterconfigDoc.FilePath;
			}
			foreach (MasterConfigFragmentDocument fragment in m_masterconfigDoc.Fragments)
			{
				foreach (string fragmentFilepaths in GetFragmentPathsToModifyOnSave(fragment))
				{
					yield return fragmentFilepaths;
				}
			}
			yield break;
		}

		public static MasterConfigUpdater Load(string filePath, Project project = null, List<string> cmdlineFragments = null)
		{
			Project inputProject = project ?? new Project(Log.Silent);
			return new MasterConfigUpdater(filePath, inputProject, cmdlineFragments);
		}
	}
}