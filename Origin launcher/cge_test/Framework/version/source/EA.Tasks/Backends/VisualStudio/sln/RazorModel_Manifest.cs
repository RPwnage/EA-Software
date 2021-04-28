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
using System.IO;

using NAnt.Core;
using NAnt.Core.Util;

namespace EA.Eaconfig.Backends.VisualStudio
{
	public class RazorModel_Manifest
	{
		private readonly string _optionsetname;
		private readonly PathString _imageFolder;
		private readonly PathString _vsProjectDir;
		private string _imageRelPath;


		public RazorModel_Manifest(string optionsetname, OptionSet optionset, PathString imageFolder, PathString vsProjectDir, string vsProjectFileNameWithoutExtension, string vsProjectName)
		{
			_optionsetname = optionsetname;
			Options = new OptionsAcessor(optionset);
			_imageFolder = imageFolder;
			_vsProjectDir = vsProjectDir;
			VsProjectFileNameWithoutExtension = vsProjectFileNameWithoutExtension;
			VsProjectName = vsProjectName;

			_imageRelPath = PathUtil.RelativePath(imageFolder.Path, _vsProjectDir.Path);
		}

		public string VsProjectName { get; }

		public string VsProjectDir
		{
			get { return _vsProjectDir.Path; }
		}

		public string VsProjectFileNameWithoutExtension { get; }

		public string GetImageRelativePath(string origpath)
		{
			if (!String.IsNullOrEmpty(origpath))
			{
				return Path.Combine(_imageRelPath, Path.GetFileName(origpath));
			}
			return String.Empty;
		}

		public Guid ParseGuidOrDefault(string input, Guid def)
		{
			Guid result;
			if (!Guid.TryParse(input ?? String.Empty, out result))
			{
				result = def;
			}
			return result;
		}

		public OptionsAcessor Options { get; }



		public class OptionsAcessor
		{
			private OptionSet _optionset;

			internal OptionsAcessor(OptionSet optionset)
			{
				_optionset = optionset;
			}

			public string this[string name]
			{
				get
				{
					var val = _optionset.Options[name];
					return (val == null)? val : val.TrimWhiteSpace();
				}
			}

		}
	}
}
