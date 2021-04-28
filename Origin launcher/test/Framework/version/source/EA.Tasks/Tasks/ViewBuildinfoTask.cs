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
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

using NAnt.Core;
using NAnt.Core.Attributes;

using EA.FrameworkTasks.Model;

namespace EA.Eaconfig
{
	// Create some wrapper classes to allow data binding to the GUI controls
	public class ConfigWrapper
	{
		public ConfigWrapper(Configuration config)
		{
			Config = config;
		}
		public string ConfigName { get { return Config.Name; } }
		public Configuration Config { get; set; }
	}

	public class ListBoxItem
	{
		public ListBoxItem(IModule module, IPackage package, string buildgroup, string dependencyType)
		{
			Module = module;
			Package = package;
			BuildGroup = buildgroup;
			DependencyType = dependencyType;
		}
		public IModule Module { get; set; }
		public IPackage Package { get; set; }
		public string BuildGroup { get; set; }
		public string DependencyType { get; set; }
		public string ValueMember
		{
			get
			{
				if (Module == null)
				{
					if (Package == null)
						return BuildGroup;
					else
						return Package.Name;
				}
				else
				{
					return Module.Name;
				}
			}
		}
		public string DisplayMember
		{
			get
			{
				if (Module == null)
				{
					if (Package == null)
						return "    " + BuildGroup;
					else
						return Package.Name;
				}
				else
				{
					string moduleName = Module.Name;
					if (moduleName == Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME)
					{
						moduleName = Package.Name;
					}
					if (!String.IsNullOrEmpty(DependencyType))
						return "        " + moduleName + " (" + DependencyType + ")";
					else
						return "        " + moduleName;
				}
			}
		}
	}

	/// <summary>
	/// Show a simple GUI dialog box to allow user interactively view the build info of each build module.
	/// </summary>
	[TaskName("viewbuildinfo")]
	public class ViewBuildInfoTask : Task
	{
		public ViewBuildInfoTask() : base("viewbuildinfo") { }

		protected override void ExecuteTask()
		{
			mCurrBuildGraph = Project.BuildGraph();

			if (!mCurrBuildGraph.IsBuildGraphComplete)
			{
				throw new BuildException("Build graph needs to be setup before you can execute the <viewbuildinfo> task.");
			}

			if (mCurrBuildGraph.TopModules == null || !mCurrBuildGraph.TopModules.Any())
			{
				Log.Minimal.WriteLine(LogPrefix + "Build graph doesn't have any top modules!  Nothing to view.");
				return;
			}

			try
			{
				Log.Minimal.WriteLine(LogPrefix + "Setting up buildinfo dialog box ...");

				mConfigList = mCurrBuildGraph.ConfigurationList.Select(cfg => new ConfigWrapper(cfg)).OrderBy(cfg => cfg.ConfigName).ToList();
				mAllModules = mCurrBuildGraph.TopModules.Union(mCurrBuildGraph.TopModules.SelectMany(mod => mod.Dependents.FlattenAll(), (m, d) => d.Dependent)).ToList();

				mCurrSelectedPackage = mCurrBuildGraph.TopModules.Where(m => m.Configuration.Name == mConfigList[mCurrConfigIndex].Config.Name).First().Package;
				mCurrSelectedModule = null;
				AddModulesToList(mCurrSelectedPackage.Modules.Where(m => m.Configuration.Name == mConfigList[mCurrConfigIndex].Config.Name), ref mCurrentBoxModuleList);

				mMainForm = new Form();
				mMainForm.Text = "Interactive BuildInfo Viewer";
				mMainForm.Size = new Size(mMainForm.Size.Width + 700, mMainForm.Size.Height + 300);
				mMainForm.Resize += new EventHandler(MainForm_Resize);

				mCurrConfigLabel = new Label();
				mCurrConfigLabel.Visible = true;
				mCurrConfigLabel.Text = "Current config:";
				mMainForm.Controls.Add(mCurrConfigLabel);

				mConfigSelectBox = new ComboBox();
				BindingSource configSelectBoxBindingSource = new BindingSource();
				configSelectBoxBindingSource.DataSource = mConfigList;
				mConfigSelectBox.DataSource = configSelectBoxBindingSource;
				mConfigSelectBox.DisplayMember = "ConfigName";
				mConfigSelectBox.ValueMember = "ConfigName";
				mConfigSelectBox.Visible = true;
				mConfigSelectBox.SelectedValue = mConfigList.First(); // Set the value first before setting the callback.
				mConfigSelectBox.SelectedIndexChanged += new EventHandler(ConfigSelectBox_SelectedIndexChanged);
				mMainForm.Controls.Add(mConfigSelectBox);

				mModuleSearchLabel = new Label();
				mModuleSearchLabel.Visible = true;
				mModuleSearchLabel.Text = "Search for Module:";
				mMainForm.Controls.Add(mModuleSearchLabel);

				mModuleSearchTextBox = new TextBox();
				mModuleSearchTextBox.Multiline = false;
				mModuleSearchTextBox.ReadOnly = false;
				mModuleSearchTextBox.KeyDown += new KeyEventHandler(ModuleSearchBox_KeyDown);
				mMainForm.Controls.Add(mModuleSearchTextBox);

				mGetModulePropertiesButton = new Button();
				mGetModulePropertiesButton.Text = "Module's Properties";
				mGetModulePropertiesButton.Click += new EventHandler(GetModulePropertiesButton_Click);
				mMainForm.Controls.Add(mGetModulePropertiesButton);

				mInspectOptionSetsButton = new Button();
				mInspectOptionSetsButton.Text = "Inspect OptionSets";
				mInspectOptionSetsButton.Click += new EventHandler(InspectOptionSetsButton_Click);
				mMainForm.Controls.Add(mInspectOptionSetsButton);

				mParentLabel = new Label();
				mParentLabel.Text = "Parent Modules:";
				mParentLabel.Visible = true;
				mMainForm.Controls.Add(mParentLabel);

				mCurrentPkgLabel = new Label();
				mCurrentPkgLabel.Text = "Current Package:";
				mCurrentPkgLabel.Visible = true;
				mMainForm.Controls.Add(mCurrentPkgLabel);

				mDependentLabel = new Label();
				mDependentLabel.Text = "Dependent Modules:";
				mDependentLabel.Visible = true;
				mMainForm.Controls.Add(mDependentLabel);

				mParentPackageModules = new ListBox();
				BindingSource parentBoxBindingSource = new BindingSource();
				parentBoxBindingSource.DataSource = mParentBoxModuleList;
				mParentPackageModules.DataSource = parentBoxBindingSource;
				mParentPackageModules.DisplayMember = "DisplayMember";
				mParentPackageModules.ValueMember = "ValueMember";
				mParentPackageModules.MouseClick += new MouseEventHandler(ParentListBox_MouseClick);
				mParentPackageModules.Visible = true;
				mMainForm.Controls.Add(mParentPackageModules);

				mCurrentPackageModules = new ListBox();
				BindingSource currentBoxBindingSource = new BindingSource();
				currentBoxBindingSource.DataSource = mCurrentBoxModuleList;
				mCurrentPackageModules.DataSource = currentBoxBindingSource;
				mCurrentPackageModules.DisplayMember = "DisplayMember";
				mCurrentPackageModules.ValueMember = "ValueMember";
				mCurrentPackageModules.MouseClick += new MouseEventHandler(CurrentListBox_MouseClick);
				mCurrentPackageModules.Visible = true;
				mMainForm.Controls.Add(mCurrentPackageModules);

				mDependentPackageModules = new ListBox();
				mDependentPackageModules.SelectionMode = SelectionMode.One;
				BindingSource dependentBoxBindingSource = new BindingSource();
				dependentBoxBindingSource.DataSource = mDependentBoxModuleList;
				mDependentPackageModules.DataSource = dependentBoxBindingSource;
				mDependentPackageModules.DisplayMember = "DisplayMember";
				mDependentPackageModules.ValueMember = "ValueMember";
				mDependentPackageModules.MouseClick += new MouseEventHandler(DependentListBox_MouseClick);
				mDependentPackageModules.Visible = true;
				mMainForm.Controls.Add(mDependentPackageModules);

				ResizeControls();

				mConfigSelectBox.SelectedItem = mConfigList[mCurrConfigIndex];
				RepopulateAllControlData();
			}
			catch (Exception e)
			{
				throw new BuildException("Error setting up the GUI dialog box: ", e);
			}
			try
			{
				Log.Minimal.WriteLine(LogPrefix + "Showing buildinfo dialog box ...");
				mMainForm.ShowDialog();
			}
			catch (Exception e)
			{
				throw new BuildException("Error executing the dialog box: ", e);
			}
		}

		// Some private data initialized in the ExecuteTask function and will be used later in even callbacks.
		BuildGraph mCurrBuildGraph = null;
		private int mCurrConfigIndex = 0;
		private List<ConfigWrapper> mConfigList = new List<ConfigWrapper>();
		private List<ListBoxItem> mCurrentBoxModuleList = new List<ListBoxItem>();
		private List<ListBoxItem> mParentBoxModuleList = new List<ListBoxItem>();
		private List<ListBoxItem> mDependentBoxModuleList = new List<ListBoxItem>();
		private List<IModule> mAllModules = new List<IModule>();
		private IPackage mCurrSelectedPackage = null;
		private IModule mCurrSelectedModule = null;

		// The important control items in the form that will need to be updated when things are changed.
		private Form mMainForm;
		private Label mCurrConfigLabel;
		private ComboBox mConfigSelectBox;
		private Label mModuleSearchLabel;
		private TextBox mModuleSearchTextBox;
		private Button mGetModulePropertiesButton;
		private Button mInspectOptionSetsButton;
		private Label mParentLabel;
		private ListBox mParentPackageModules;
		private Label mCurrentPkgLabel;
		private ListBox mCurrentPackageModules;
		private Label mDependentLabel;
		private ListBox mDependentPackageModules;

		private void MainForm_Resize(object sender, EventArgs e)
		{
			ResizeControls();
			mMainForm.Invalidate();
		}

		private void ConfigSelectBox_SelectedIndexChanged(object sender, EventArgs e)
		{
			mCurrConfigIndex = (sender as ComboBox).SelectedIndex;
			mCurrSelectedModule = null;

			// Find the current package under selected config
			IEnumerable<IPackage> pkgs = null;
			if (mCurrSelectedPackage != null)
			{
				string currentPackageName = mCurrSelectedPackage.Name;
				pkgs = mAllModules.Where(m => m.Configuration.Name == mConfigList[mCurrConfigIndex].Config.Name && m.Package.Name == currentPackageName).Select(m => m.Package);
			}
			if (pkgs == null || !pkgs.Any())
			{
				mCurrSelectedPackage = mCurrBuildGraph.TopModules.Where(m => m.Configuration.Name == mConfigList[mCurrConfigIndex].Config.Name).First().Package;
			}
			else
			{
				mCurrSelectedPackage = pkgs.First();
			}

			RepopulateAllControlData();
			mMainForm.Invalidate();
		}

		private void ModuleSearchBox_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Return)
			{
				string searchModuleName = mModuleSearchTextBox.Text;
				IEnumerable<IModule> foundModules = mAllModules.Where(m => (m.Name == searchModuleName || (m.Package.Name == searchModuleName && m.Name == Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME)) && m.Configuration.Name == mConfigList[mCurrConfigIndex].ConfigName);
				if (foundModules.Any())
				{
					// There should be only one for a given config.
					mCurrSelectedModule = foundModules.First();
					mCurrSelectedPackage = mCurrSelectedModule.Package;
					mModuleSearchTextBox.Text = "";
					RepopulateAllControlData();
					mMainForm.Invalidate();
				}
				else
				{
					MessageBox.Show(String.Format("Unable to find module {0}",searchModuleName));
					mModuleSearchTextBox.Text = "";
				}
			}
		}

		private void GetModulePropertiesButton_Click(object sender, EventArgs e)
		{
			if (mCurrSelectedModule == null && mCurrSelectedPackage == null)
			{
				MessageBox.Show("Please select a module first to see the module's Properties set!");
				return;
			}
			PropertyDictionary properties = mCurrSelectedModule == null 
				? mCurrSelectedPackage.Project.Properties 
				: mCurrSelectedModule.Package.Project.Properties;

			PropertyViewForm propDialog = new PropertyViewForm(properties);
			propDialog.Text = "Properties of " + (mCurrSelectedModule == null ? "Package \"" + mCurrSelectedPackage.Name + "\"" : "Module \"" + (mCurrSelectedModule.Name == Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME ? mCurrSelectedModule.Package.Name : mCurrSelectedModule.Name) + "\"")
					+ " (" + mCurrSelectedModule.Configuration.Name + ")";

			propDialog.Show();
		}

		private void InspectOptionSetsButton_Click(object sender, EventArgs e)
		{
			if (mCurrSelectedModule == null && mCurrSelectedPackage == null)
			{
				MessageBox.Show("Please select a module first to see the module's Properties set!");
				return;
			}
			OptionSetDictionary optionSetDictionary = mCurrSelectedModule == null 
				? mCurrSelectedPackage.Project.NamedOptionSets
				: mCurrSelectedModule.Package.Project.NamedOptionSets;

			OptionSetsInspectForm optionSetInspectDialog = new OptionSetsInspectForm(optionSetDictionary);
			optionSetInspectDialog.Text = "Named OptionSets for " + (mCurrSelectedModule == null ? "Package \"" + mCurrSelectedPackage.Name + "\"" : "Module \"" + (mCurrSelectedModule.Name == Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME ? mCurrSelectedModule.Package.Name : mCurrSelectedModule.Name) + "\"")
					+ " (" + mCurrSelectedModule.Configuration.Name + ")";

			optionSetInspectDialog.Show();
		}

		private void CurrentListBox_MouseClick(object sender, MouseEventArgs e)
		{
			// Get the new module info.
			ListBox lb = sender as ListBox;
			ListBoxItem lbSelect = lb.SelectedItem as ListBoxItem;
			if (lbSelect != null)
			{
				if (lbSelect.Module != null && mCurrSelectedModule != lbSelect.Module)
				{
					mCurrSelectedModule = lbSelect.Module;

					// Re-populate the other two list box
					RepopulateParentModulesColumn();
					RepopulateDependentModulesColumn();
				}
				else if (lbSelect.Module == null)
				{
					if (mCurrSelectedModule != null)
						RepopulateCurrentPackageColumn();
					else
						lb.ClearSelected();
				}
			}
		}

		private void ParentListBox_MouseClick(object sender, MouseEventArgs e)
		{
			// Get the new module info.
			ListBox lb = sender as ListBox;
			ListBoxItem lbSelect = lb.SelectedItem as ListBoxItem;
			if (lbSelect != null)
			{
				if (lbSelect.Module != null && mCurrSelectedModule != lbSelect.Module)
				{
					mCurrSelectedModule = lbSelect.Module;
					mCurrSelectedPackage = mCurrSelectedModule.Package;

					RepopulateAllControlData();
				}
				else if (lbSelect.Module == null)
				{
					lb.ClearSelected();
				}
			}
		}

		private void DependentListBox_MouseClick(object sender, MouseEventArgs e)
		{
			// Get the new module info.
			ListBox lb = sender as ListBox;
			ListBoxItem lbSelect = lb.SelectedItem as ListBoxItem;
			if (lbSelect != null)
			{
				if (lbSelect.Module != null && mCurrSelectedModule != lbSelect.Module)
				{
					mCurrSelectedModule = lbSelect.Module;
					mCurrSelectedPackage = mCurrSelectedModule.Package;

					RepopulateAllControlData();
				}
				else if (lbSelect.Module == null)
				{
					lb.ClearSelected();
				}
			}
		}

		private void AddModulesToList(IEnumerable<IModule> modules, ref List<ListBoxItem> listboxData)
		{
			if (modules == null || !modules.Any())
				return;

			IPackage pkg = modules.First().Package;
			listboxData.Add(new ListBoxItem(null, pkg, null, null));

			foreach (BuildGroups grp in Enum.GetValues(typeof(BuildGroups)))
			{
				IEnumerable<IModule> grpModules = modules.Where(m => m.BuildGroup == grp);
				if (grpModules.Any())
				{
					string currGroup = grp.ToString();
					listboxData.Add(new ListBoxItem(null, null, currGroup, null));
					foreach (IModule module in grpModules)
					{
						listboxData.Add(new ListBoxItem(module, pkg, currGroup, null));
					}
				}
			}
		}

		private void AddPackageDependencyToList(IEnumerable<Dependency<IModule>> dependents, ref List<ListBoxItem> listboxData)
		{
			if (dependents == null || !dependents.Any())
				return;

			IPackage pkg = dependents.First().Dependent.Package;
			listboxData.Add(new ListBoxItem(null, pkg, null, null));

			foreach (BuildGroups grp in Enum.GetValues(typeof(BuildGroups)))
			{
				IEnumerable<Dependency<IModule>> grpDependents = dependents.Where(d => d.Dependent.BuildGroup == grp);
				if (grpDependents.Any())
				{
					string currGroup = grp.ToString();
					listboxData.Add(new ListBoxItem(null, null, currGroup, null));
					foreach (Dependency<IModule> dependent in grpDependents)
					{
						listboxData.Add(new ListBoxItem(dependent.Dependent, pkg, currGroup, ResolveDependentTypeString(dependent)));
					}
				}
			}
		}

		private void AddParentOfModuleToList(IModule currentModule, IEnumerable<IModule> parentPkgModules, ref List<ListBoxItem> listboxData)
		{
			if (currentModule == null || parentPkgModules == null || !parentPkgModules.Any())
				return;

			IPackage parentPkg = parentPkgModules.First().Package;
			listboxData.Add(new ListBoxItem(null, parentPkg, null, null));

			foreach (BuildGroups grp in Enum.GetValues(typeof(BuildGroups)))
			{
				IEnumerable<IModule> grpModules = parentPkgModules.Where(m => m.BuildGroup == grp);
				if (grpModules.Any())
				{
					string currGroup = grp.ToString();
					listboxData.Add(new ListBoxItem(null, null, currGroup, null));
					foreach (IModule module in grpModules)
					{
						string depType = ResolveDependentTypeString(module.Dependents.Where(d => d.Dependent == currentModule).First());
						listboxData.Add(new ListBoxItem(module, parentPkg, currGroup, depType));
					}
				}
			}
		}

		private void ResizeControls()
		{
			int horizontalSpacing = 10;
			int verticalSpacing = 7;
			int columnWidth = (mMainForm.ClientSize.Width - (horizontalSpacing * 4)) / 3;

			mCurrConfigLabel.Top = verticalSpacing*2;
			mCurrConfigLabel.Left = horizontalSpacing;
			mCurrConfigLabel.Width = horizontalSpacing * 9;

			mConfigSelectBox.Top = mCurrConfigLabel.Top-3;
			mConfigSelectBox.Left = mCurrConfigLabel.Left + mCurrConfigLabel.Width + horizontalSpacing;
			mConfigSelectBox.Width = horizontalSpacing * 15;

			mModuleSearchLabel.Top = mCurrConfigLabel.Top;
			mModuleSearchLabel.Left = mConfigSelectBox.Left + mConfigSelectBox.Width + horizontalSpacing * 3;

			mModuleSearchTextBox.Top = mModuleSearchLabel.Top-3;
			mModuleSearchTextBox.Left = mModuleSearchLabel.Left + mModuleSearchLabel.Width + horizontalSpacing;
			mModuleSearchTextBox.Width = horizontalSpacing * 20;

			mGetModulePropertiesButton.Top = mModuleSearchTextBox.Top-2;
			mGetModulePropertiesButton.Left = mModuleSearchTextBox.Left + mModuleSearchTextBox.Width + horizontalSpacing * 2;
			mGetModulePropertiesButton.Width = horizontalSpacing * 15;

			mInspectOptionSetsButton.Top = mGetModulePropertiesButton.Top;
			mInspectOptionSetsButton.Left = mGetModulePropertiesButton.Left + mGetModulePropertiesButton.Width + horizontalSpacing * 2;
			mInspectOptionSetsButton.Width = mGetModulePropertiesButton.Width;

			mParentLabel.Top = mCurrConfigLabel.Top + mCurrConfigLabel.Height + verticalSpacing;
			mParentLabel.Left = mCurrConfigLabel.Left;
			mParentLabel.Width = columnWidth;

			mCurrentPkgLabel.Top = mParentLabel.Top;
			mCurrentPkgLabel.Left = mParentLabel.Left + columnWidth + horizontalSpacing;
			mCurrentPkgLabel.Width = columnWidth;

			mDependentLabel.Top = mParentLabel.Top;
			mDependentLabel.Left = mCurrentPkgLabel.Left + columnWidth + horizontalSpacing;
			mDependentLabel.Width = columnWidth;

			int listBoxHeight = mMainForm.ClientSize.Height - verticalSpacing * 5 - mCurrentPkgLabel.Height - mParentLabel.Height;

			mParentPackageModules.Top = mParentLabel.Top + mParentLabel.Height + verticalSpacing;
			mParentPackageModules.Left = mParentLabel.Left;
			mParentPackageModules.Width = columnWidth;
			mParentPackageModules.Height = listBoxHeight;

			mCurrentPackageModules.Top = mCurrentPkgLabel.Top + mCurrentPkgLabel.Height + verticalSpacing;
			mCurrentPackageModules.Left = mCurrentPkgLabel.Left;
			mCurrentPackageModules.Width = columnWidth;
			mCurrentPackageModules.Height = listBoxHeight;

			mDependentPackageModules.Top = mDependentLabel.Top + mDependentLabel.Height + verticalSpacing;
			mDependentPackageModules.Left = mDependentLabel.Left;
			mDependentPackageModules.Width = columnWidth;
			mDependentPackageModules.Height = listBoxHeight;
		}

		private void RepopulateAllControlData()
		{
			RepopulateCurrentPackageColumn();
			RepopulateParentModulesColumn();
			RepopulateDependentModulesColumn();
		}

		string ResolveDependentTypeString(Dependency<IModule> dep)
		{
			if (dep.IsKindOf(Core.DependencyTypes.AutoBuildUse))
				return "auto";
			else if (dep.IsKindOf(Core.DependencyTypes.Build))
				return "build";
			else if (dep.IsKindOf(Core.DependencyTypes.Interface) && dep.IsKindOf(Core.DependencyTypes.Link))
				return "use";
			else if (dep.IsKindOf(Core.DependencyTypes.Interface))
				return "interface";
			else if (dep.IsKindOf(Core.DependencyTypes.Link))
				return "link";
			return "";
		}

		private void RepopulateCurrentPackageColumn()
		{
			int currentSelection = -1;
			mCurrentBoxModuleList.Clear();

			if (mCurrSelectedPackage != null)
			{
				IEnumerable<IModule> currPackageModulesInConfig = mCurrSelectedPackage.Modules.Where(m => m.Configuration.Name == mConfigList[mCurrConfigIndex].Config.Name);
				if (currPackageModulesInConfig != null && 
					!currPackageModulesInConfig.Any() && mCurrSelectedModule != null && 
					mCurrSelectedModule.Configuration.Name == mConfigList[mCurrConfigIndex].Config.Name &&
					mCurrSelectedModule.Package == mCurrSelectedPackage &&
					(mCurrSelectedModule.Name == Modules.Module_UseDependency.PACKAGE_DEPENDENCY_NAME || mCurrSelectedModule.Name == mCurrSelectedPackage.Name))
				{
					// For some reason, the package's module list don't store this dummy module in Use dependent package.
					currPackageModulesInConfig = new List<IModule>() { mCurrSelectedModule };
				}
				AddModulesToList(currPackageModulesInConfig, ref mCurrentBoxModuleList);

				if (mCurrSelectedModule != null)
				{
					foreach (ListBoxItem itm in mCurrentBoxModuleList)
					{
						++currentSelection;
						if (itm.Module == mCurrSelectedModule)
						{
							break;
						}
					}
				}
			}

			BindingSource currentBoxBindingSource = new BindingSource();
			currentBoxBindingSource.DataSource = mCurrentBoxModuleList;
			mCurrentPackageModules.DataSource = currentBoxBindingSource;
			mCurrentPackageModules.DisplayMember = "DisplayMember";
			mCurrentPackageModules.ValueMember = "ValueMember";
			if (currentSelection != -1)
			{
				mCurrentPackageModules.SetSelected(currentSelection, true);
			}
			else
			{
				mCurrentPackageModules.ClearSelected();
			}
			mCurrentPackageModules.Update();
		}

		private void RepopulateParentModulesColumn()
		{
			mParentBoxModuleList.Clear();

			if (mCurrSelectedModule != null)
			{
				IEnumerable<IModule> allParents = mAllModules.Where(m => m.Dependents.Contains(mCurrSelectedModule));
				IEnumerable<IPackage> allPackages = allParents.Select(m => m.Package).Distinct();

				foreach (IPackage pkg in allPackages)
				{
					AddParentOfModuleToList(mCurrSelectedModule, allParents.Where(m => m.Package.Name == pkg.Name).OrderBy(m => m.Package.Name), ref mParentBoxModuleList);
				}
			}

			BindingSource parentBoxBindingSource = new BindingSource();
			parentBoxBindingSource.DataSource = mParentBoxModuleList;
			mParentPackageModules.DataSource = parentBoxBindingSource;
			mParentPackageModules.DisplayMember = "DisplayMember";
			mParentPackageModules.ValueMember = "ValueMember";
			mParentPackageModules.ClearSelected();
			mParentPackageModules.Update();
		}

		private void RepopulateDependentModulesColumn()
		{
			mDependentBoxModuleList.Clear();

			if (mCurrSelectedModule != null)
			{
				IEnumerable<Dependency<IModule>> allDependents = mCurrSelectedModule.Dependents.AsEnumerable();
				IEnumerable<IPackage> allPackages = allDependents.Select(d => d.Dependent.Package).Distinct();

				foreach (IPackage pkg in allPackages.OrderBy(p => p.Name))
				{
					AddPackageDependencyToList(allDependents.Where(d => d.Dependent.Package == pkg).OrderBy(d => d.Type), ref mDependentBoxModuleList);
				}
			}

			BindingSource dependentBoxBindingSource = new BindingSource();
			dependentBoxBindingSource.DataSource = mDependentBoxModuleList;
			mDependentPackageModules.DataSource = dependentBoxBindingSource;
			mDependentPackageModules.DisplayMember = "DisplayMember";
			mDependentPackageModules.ValueMember = "ValueMember";
			mDependentPackageModules.ClearSelected();
			mDependentPackageModules.Update();
		}
	}

	public class PropertyViewForm : Form
	{
		private ListView mPropertyListView = null;

		public PropertyViewForm(PropertyDictionary properties)
		{
			mPropertyListView = new ListView();
			mPropertyListView.Columns.Add("Name", 200, HorizontalAlignment.Left);
			mPropertyListView.Columns.Add("Value", -2, HorizontalAlignment.Left);
			foreach (Property prop in properties.OrderBy(p => p.Name))
			{
				ListViewItem item = new ListViewItem(new string[] { prop.Name, prop.Value });
				mPropertyListView.Items.Add(item);
			}
			mPropertyListView.GridLines = true;
			mPropertyListView.View = View.Details;
			mPropertyListView.MultiSelect = false;
			mPropertyListView.Top = 10;
			mPropertyListView.Left = 10;
			mPropertyListView.DoubleClick += new EventHandler(PropertyListView_DoubleClick);

			Size = new Size(DefaultSize.Width + 500, DefaultSize.Height + 200);

			Controls.Add(mPropertyListView);

			ResizeControls();

			Resize += new EventHandler(ResizeThisForm);
		}

		private void ResizeControls()
		{
			mPropertyListView.Size = new Size(ClientSize.Width - 20, ClientSize.Height - 20);
			mPropertyListView.Update();
		}

		private void ResizeThisForm(object sender, EventArgs e)
		{
			ResizeControls();
		}

		private void PropertyListView_DoubleClick(object sender, EventArgs e)
		{
			if (mPropertyListView.SelectedItems != null)
			{
				TextDisplayForm dispDialog = new TextDisplayForm(mPropertyListView.SelectedItems[0].SubItems[1].Text);
				dispDialog.Text = "Content of " + mPropertyListView.SelectedItems[0].Text;
				dispDialog.Show();
			}
		}
	}

	public class OptionSetsInspectForm : Form
	{
		private Label mNamedOptionSetName = null;
		private ComboBox mOptionSetComboBox = null;
		private ListView mOptionSetListView = null;
		private OptionSet mCurrentSelectedOptionSet = null;

		public OptionSetsInspectForm(OptionSetDictionary optionSets)
		{
			mNamedOptionSetName = new Label();
			mNamedOptionSetName.Text = "OptionSet Name:";
			mNamedOptionSetName.Top = 10;
			mNamedOptionSetName.Left = 10;
			mNamedOptionSetName.Width = 100;
			mNamedOptionSetName.Visible = true;
			Controls.Add(mNamedOptionSetName);

			mOptionSetComboBox = new ComboBox();
			BindingSource optionSetListBindingSource = new BindingSource();
			optionSetListBindingSource.DataSource = optionSets.OrderBy(o => o.Key);
			mCurrentSelectedOptionSet = (optionSetListBindingSource.DataSource as IOrderedEnumerable<KeyValuePair<string, OptionSet>>).First().Value;
			mOptionSetComboBox.DataSource = optionSetListBindingSource;
			mOptionSetComboBox.DisplayMember = "Key";
			mOptionSetComboBox.ValueMember = "Key";
			mOptionSetComboBox.Top = mNamedOptionSetName.Top - 2;
			mOptionSetComboBox.Left = mNamedOptionSetName.Left + mNamedOptionSetName.Width + 10;
			mOptionSetComboBox.Width = 250;
			mOptionSetComboBox.Visible = true;
			mOptionSetComboBox.SelectedValue = mCurrentSelectedOptionSet;
			mOptionSetComboBox.SelectedIndexChanged += new EventHandler(OptionSetComboBox_SelectedIndexChanged);
			Controls.Add(mOptionSetComboBox);

			Size = new Size(DefaultSize.Width + 500, DefaultSize.Height + 200);

			mOptionSetListView = new ListView();
			mOptionSetListView.Columns.Add("Name", 200, HorizontalAlignment.Left);
			mOptionSetListView.Columns.Add("Value", -2, HorizontalAlignment.Left);
			mOptionSetListView.GridLines = true;
			mOptionSetListView.View = View.Details;
			mOptionSetListView.MultiSelect = false;
			mOptionSetListView.Top = mNamedOptionSetName.Top + mNamedOptionSetName.Height + 7;
			mOptionSetListView.Left = mNamedOptionSetName.Left;
			mOptionSetListView.DoubleClick += new EventHandler(OptionSetListView_DoubleClick);
			ResetListViewData();
			Controls.Add(mOptionSetListView);

			ResizeControls();

			Resize += new EventHandler(ResizeThisForm);
		}

		private void ResizeControls()
		{
			mOptionSetListView.Width = ClientSize.Width - 25;
			mOptionSetListView.Height = ClientSize.Height - mOptionSetComboBox.Height - 30;
		}

		private void ResetListViewData()
		{
			if (mCurrentSelectedOptionSet == null)
				return;

			mOptionSetListView.Items.Clear();

			foreach (KeyValuePair<string,string> opt in mCurrentSelectedOptionSet.Options.OrderBy(o => o.Key))
			{
				ListViewItem item = new ListViewItem(new string[] { opt.Key, opt.Value });
				mOptionSetListView.Items.Add(item);
			}
		}

		private void ResizeThisForm(object sender, EventArgs e)
		{
			ResizeControls();
		}

		private void OptionSetComboBox_SelectedIndexChanged(object sender, EventArgs e)
		{
			KeyValuePair<string, OptionSet> selectedItem = (KeyValuePair<string, OptionSet>)(sender as ComboBox).SelectedItem;
			if (selectedItem.Value != null && mCurrentSelectedOptionSet != selectedItem.Value)
			{
				mCurrentSelectedOptionSet = selectedItem.Value;
				ResetListViewData();
				mOptionSetListView.Update();
			}
		}

		private void OptionSetListView_DoubleClick(object sender, EventArgs e)
		{
			if (mOptionSetListView.SelectedItems != null)
			{
				TextDisplayForm dispDialog = new TextDisplayForm(mOptionSetListView.SelectedItems[0].SubItems[1].Text);
				dispDialog.Text = "Content of " + mOptionSetListView.SelectedItems[0].Text;
				dispDialog.Show();
			}
		}
	}

	public class TextDisplayForm : Form
	{
		private TextBox mTextBox = null;

		public TextDisplayForm(string text)
		{
			Size = new Size(DefaultSize.Width + 500, DefaultSize.Height + 200);

			mTextBox = new TextBox();
			mTextBox.Top = 0;
			mTextBox.Left = 0;
			mTextBox.Multiline = true;
			mTextBox.ReadOnly = true;
			mTextBox.ScrollBars = ScrollBars.Both;
			mTextBox.ShortcutsEnabled = false;
			mTextBox.TabStop = false;
			mTextBox.WordWrap = false;
			mTextBox.Text = text;

			Controls.Add(mTextBox);

			ResizeControls();

			Resize += new EventHandler(ResizeThisForm);
		}

		private void ResizeControls()
		{
			mTextBox.Size = new Size(ClientSize.Width, ClientSize.Height);
			mTextBox.Update();
		}

		private void ResizeThisForm(object sender, EventArgs e)
		{
			ResizeControls();
		}
	}
}