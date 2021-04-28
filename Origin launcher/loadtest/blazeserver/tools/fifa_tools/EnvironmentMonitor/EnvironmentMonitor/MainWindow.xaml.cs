using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Reflection;
using System.ComponentModel;

namespace EnvironmentMonitor
{
    public class RuleSelection
    {
        public RuleSelection() { Enable = true; }
        public bool Enable{get;set;}
        public String RuleName {get;set;}
        public String Modulename {get;set;}
        public Rule mRule;
    }

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    /// 

    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        private List<MonitorModule> mModules = new List<MonitorModule>();

        private List<RuleSelection> mRules = new List<RuleSelection>();
        private RuleCallbackImpl mRuleCallbackObj;
        private int mCurrentRuleIndex;

        private List<ModuleDetailReport> ModuleDetailRptWnds = new List<ModuleDetailReport>();

        private string CurrentRuleName;
        public string RuleName
        {
            get { return CurrentRuleName; }
            set {
                CurrentRuleName = value;
                OnPropertyChanged("RuleName");
            }
        }

        private string mReport;
        public string MainReport
        {
            get { return mReport; }
            set
            {
                mReport = value;
                OnPropertyChanged("MainReport");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChangedEventHandler handler = this.PropertyChanged;
            if (handler != null)
            {
                var e = new PropertyChangedEventArgs(propertyName);
                handler(this, e);
            }
        }

        

        /*
         * RuleCallbackImpl
         * RuleCallbackInterface Implementation Object
         * This impl object will be called when each rule completes its evaluation
         */
        public class RuleCallbackImpl : RuleCallbackInterface
        {
            MainWindow mMainWindow;
            public RuleCallbackImpl(MainWindow mainWindow)
            {
                mMainWindow = mainWindow;
            }

            public void HandleRuleEvaluationComplete(bool bCriticalError = false)
            {
                mMainWindow.EvaluationProgressBar.Value += 1;

                if (bCriticalError)
                {
                    mMainWindow.mCurrentRuleIndex = mMainWindow.mRules.Count;
                }
                mMainWindow.PerformRuleEvaluation();                
            }

            public void HandleRecommendationUpdate(object sender, PropertyChangedEventArgs e)
            {
                mMainWindow.HandleRecommendationUpdate(sender, e);
            }
        }

        public MainWindow()
        {
            InitializeComponent();

            mRuleCallbackObj = new RuleCallbackImpl(this);

            //Create the ModuleParams
            BlazeModuleParam blazeParam = EnvironmentConfigParam.getInstance().mBlazeParam;

            //Create the Monitor Modules
            EnvironmentConfigParam.getInstance().mBlazeParam.mGOSEnv = "stest";
            EnvironmentConfigParam.getInstance().mBlazeParam.mRedirevtorURL = "https://winter15.gosredirector.stest.ea.com:42230/";           
            blazeParam.mGameTitle = "FIFA";                       
                        
            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mBlazeParam,
                ((ServerModuleBlock)(((StackPanel)BlazeServer).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)BlazeServer).Children[0])).Operation,
                "BlazeServer"));
            ((ServerModuleBlock)(((StackPanel)BlazeServer).Children[0])).MainModule.Content = "Blaze";

            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mNucleusParam,
                ((ServerModuleBlock)(((StackPanel)NucleusServer).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)NucleusServer).Children[0])).Operation, 
                "NucleusServer"));
            ((ServerModuleBlock)(((StackPanel)NucleusServer).Children[0])).MainModule.Content = "Nucleus";

            mModules.Add(new FIFAContentModule(EnvironmentConfigParam.getInstance().mCMSParam,
                ((ServerModuleBlock)(((StackPanel)FIFAContentCDN).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)FIFAContentCDN).Children[0])).Operation, 
                "FIFAContentCDN"));
            ((ServerModuleBlock)(((StackPanel)FIFAContentCDN).Children[0])).MainModule.Content = "FIFA Content";

            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mEASFCParam,
                ((ServerModuleBlock)(((StackPanel)EASFCServer).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)EASFCServer).Children[0])).Operation, 
                "EASFCServer"));
            ((ServerModuleBlock)(((StackPanel)EASFCServer).Children[0])).MainModule.Content = "EASFC (PAS)";

            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mCMSParam, 
                ((ServerModuleBlock)(((StackPanel)EASFCContentCDN).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)EASFCContentCDN).Children[0])).Operation, 
                "EASFCContentCDN"));
            ((ServerModuleBlock)(((StackPanel)EASFCContentCDN).Children[0])).MainModule.Content = "EASFC Content";

            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mFUTParam,
                ((ServerModuleBlock)(((StackPanel)FUTServer).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)FUTServer).Children[0])).Operation,
                "FUTServer"));
            ((ServerModuleBlock)(((StackPanel)FUTServer).Children[0])).MainModule.Content = "FUT (UTAS)";

            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mCMSParam,
                ((ServerModuleBlock)(((StackPanel)FUTContentCDN).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)FUTContentCDN).Children[0])).Operation, 
                "FUTContentCDN"));
            ((ServerModuleBlock)(((StackPanel)FUTContentCDN).Children[0])).MainModule.Content = "FUT Content";

            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mCMSParam,
                ((ServerModuleBlock)(((StackPanel)CTSServer).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)CTSServer).Children[0])).Operation,
                "CTSServer"));
            ((ServerModuleBlock)(((StackPanel)CTSServer).Children[0])).MainModule.Content = "CTS Server";

            mModules.Add(new MonitorModule(EnvironmentConfigParam.getInstance().mCMSParam,
                ((ServerModuleBlock)(((StackPanel)CTSContentCDN).Children[0])).Status,
                ((ServerModuleBlock)(((StackPanel)CTSContentCDN).Children[0])).Operation, 
                "CTSContentCDN"));
            ((ServerModuleBlock)(((StackPanel)CTSContentCDN).Children[0])).MainModule.Content = "CTS Content";            
           
            // Selection of rules are initialized here, the actual rules instance are created dynamically
            InitializeRuleSelections();
            RuleSets.DataContext = mRules;
            CurrentRule.DataContext = this;
            MainLogBox.DataContext = this;
            
            //BlazeServerName.DataContext = EnvironmentConfigParam.getInstance().mBlazeParam;            
        }

        /*
         * lookupMonitorModuleByName
         * lookupMonitorModuleByName is a helper function to lookup server module         
        */
        private MonitorModule lookupMonitorModuleByName(String moduleName)
        {
            // search for existing server modules and return the object
            foreach (MonitorModule module in mModules)
            {
                if (String.Compare(module.mName, moduleName) == 0)
                {
                    return module;
                }
            }
            
            return null;
        }

        /*
         * lookupModuleDetialReportWndByModuleName
         * lookupModuleDetialReportWndByModuleName is a helper function to lookup ModuleDetailReport Wnd instance         
        */
        private ModuleDetailReport lookupModuleDetialReportWndByModuleName(String moduleName)
        {
            ModuleDetailReport retWnd = null;

            foreach (ModuleDetailReport rptWnd in ModuleDetailRptWnds)
            {
                if (String.Compare(rptWnd.getModule().mName, moduleName) == 0)
                {
                    retWnd = rptWnd;                    
                    break;
                }
            }

            return retWnd;
        }

        /*
         * PerformRuleEvaluation
         * PerformRuleEvaluation is the entry point to trigger all the rules evaluation
         * This function also has logic to skip over rules that is not enabled.
        */
        private void PerformRuleEvaluation()
        {
            bool bRuleSkipped = false;

            if (mCurrentRuleIndex >= 0 && mCurrentRuleIndex < mRules.Count)
            {
                if (mRules[mCurrentRuleIndex].Enable)
                {
                    RuleName = mRules[mCurrentRuleIndex].mRule.mRuleName;
                    mRules[mCurrentRuleIndex].mRule.PerformEvaluation();
                }
                else 
                {
                    bRuleSkipped = true;
                }

                mCurrentRuleIndex++;

                // Skip over to next rule
                if (bRuleSkipped)
                {
                    PerformRuleEvaluation();
                }
            }
            else
            {
                StartTestBtn.Content = "Start Test";
                StartTestBtn.IsEnabled = true;
                CurrentRule.Text = "All rules evalauted";
                mCurrentRuleIndex = -1;
            }
        }


        /*
         * InitializeRuleSelections
         * InitializeRuleSelections fetch the Rules and Modules Pair from XAML Resource "RulesModulesPair
         * Populate the mRules with the rule names, module names, and rule enable flag.
        */
        private void InitializeRuleSelections()
        {
            ListDictionary RuleModuleNamePairs = (ListDictionary) FindResource("RulesModulesPair");

            DictionaryEntry[] pairEntries = new DictionaryEntry[RuleModuleNamePairs.Count];
            RuleModuleNamePairs.CopyTo(pairEntries, 0);

            for (int i = 0; i < pairEntries.Length; i++)
            {
                RuleSelection ruleSelection = new RuleSelection();
                ruleSelection.RuleName = (String) pairEntries[i].Key;
                ruleSelection.Modulename = (String) pairEntries[i].Value;
                mRules.Add(ruleSelection);
            }
        }

        /*
         * CreateRules
         * CreateRules goes through the GUI rule selection list and for each select rule, this function will create the actual rule objects         
        */
        private void CreateRules()
        {
            EvaluationProgressBar.Maximum = 0;
            EvaluationProgressBar.Minimum = 0;
            EvaluationProgressBar.Value = 0;

            // Go through the rule selection, and
            foreach (RuleSelection rule in mRules)
            {
                if (rule.Enable)
                {
                    MonitorModule module = lookupMonitorModuleByName(rule.Modulename);                                      

                    if (module != null)
                    {                       
                        Assembly asm = Assembly.GetEntryAssembly();
                        string path = asm.GetName().ToString();
                        path = path.Substring(0, path.IndexOf(","));
                        string ruleNamespace = path + "." + rule.RuleName;
                        Type ruleType = asm.GetType(ruleNamespace);
                        rule.mRule = (Rule)Activator.CreateInstance(ruleType, module, mRuleCallbackObj);
                        //rule.mRule.PropertyChanged += new PropertyChangedEventHandler(this.HandleRecommendationUpdate);
                    }

                    EvaluationProgressBar.Maximum += 1;
                }
            }
        }

        /*
         * StartTest
         * StartTest Kick off the environment testing
        */
        private void StartTest(object sender, RoutedEventArgs e)
        {
            //Reset modules status

            foreach (MonitorModule module in mModules)
            {
                module.resetStatus();
            }

            mCurrentRuleIndex = 0;

            MainReport = "";
            CreateRules();
            PerformRuleEvaluation();
            StartTestBtn.Content = "TESTING";
            StartTestBtn.IsEnabled = false;
        }

        public void HandleRecommendationUpdate(object sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Recommendation")
            {
                RuleReportUpdatedEvent reportUpdateEvent = (RuleReportUpdatedEvent)e;
                MainReport += reportUpdateEvent.report;
            }
        }

        public void ModuleDetailReportClosedHandler(object sender, EventArgs e)
        {
            ModuleDetailRptWnds.Remove((ModuleDetailReport)sender);
        }

        private void MouseDownInModule(object sender, MouseButtonEventArgs e)
        {
            
            // Search list of ModuleDetailRptWnds and see if the matching report window for the module has been created
            // if so, don't create the window again, just refresh

            StackPanel module = (StackPanel)(((ServerModuleBlock)sender).Parent);


            ModuleDetailReport rptWnd = lookupModuleDetialReportWndByModuleName(module.Name);

            if (rptWnd == null)
            {
                ModuleDetailReport reportWnd = new ModuleDetailReport();          
                reportWnd.setModule(lookupMonitorModuleByName(module.Name));
                reportWnd.Show();

                ModuleDetailRptWnds.Add(reportWnd);
                reportWnd.Closed += new EventHandler(ModuleDetailReportClosedHandler);
            }            
        }

        protected override void OnContentRendered(EventArgs e)
        {
            base.OnContentRendered(e);
            updateBlazeServerConfig();
        }

        protected override void OnClosed(EventArgs e)
        {
            
            // Close existing ModuleDetailReport windows
            foreach (ModuleDetailReport reportWnd in ModuleDetailRptWnds)
            {
                reportWnd.Close();
            }
            
            base.OnClosed(e);
        }

        private void ServerModuleBlock_Loaded(object sender, RoutedEventArgs e){}

        private void EnvSelectors_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if(IsLoaded){
                updateBlazeServerConfig();
            }
        }

        private void EnvTestCertSelectors_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (IsLoaded)
            {
                if (((System.String)EnvTestCertSelector.SelectedValue) == "1stPartyCert" &&
                ((System.String)EnvPlatformSelector.SelectedValue) != "PC")
                {
                    EnvironmentConfigParam.getInstance().mBlazeParam.mGOSEnv = "scert";
                    EnvironmentConfigParam.getInstance().mBlazeParam.mRedirevtorURL = "https://winter15.gosredirector.scert.ea.com";

                    Binding certEnvBinding = new Binding();
                    if ((System.String)EnvPlatformSelector.SelectedValue == "XONE") { certEnvBinding.Source = FindResource("XOneCertEnv"); }
                    else if ((System.String)EnvPlatformSelector.SelectedValue == "PS4") { certEnvBinding.Source = FindResource("PS4CertEnv"); }
                    else if ((System.String)EnvPlatformSelector.SelectedValue == "PS3") { certEnvBinding.Source = FindResource("PS3CertEnv"); }
                    else if ((System.String)EnvPlatformSelector.SelectedValue == "XBL2") { certEnvBinding.Source = FindResource("X360CertEnv"); }

                    EnvEnvSelector.SetBinding(ComboBox.ItemsSourceProperty, certEnvBinding);
                    EnvEnvSelector.SelectedIndex = 0;
                }
                else if (((System.String)EnvTestCertSelector.SelectedValue) == "Dev")
                {
                    if (((System.String)EnvPlatformSelector.SelectedValue) == "PC")
                    {
                        EnvTestCertSelector.SelectedValue = 0;
                    }

                    EnvironmentConfigParam.getInstance().mBlazeParam.mGOSEnv = "sdev";
                    EnvironmentConfigParam.getInstance().mBlazeParam.mRedirevtorURL = "https://winter15.gosredirector.sdev.ea.com:42230/";
                    Binding testEnvBinding = new Binding();
                    testEnvBinding.Source = FindResource("TestEnv");
                    EnvEnvSelector.SetBinding(ComboBox.ItemsSourceProperty, testEnvBinding);
                    EnvEnvSelector.SelectedIndex = 0;
                }
                else
                {
                    if (((System.String)EnvPlatformSelector.SelectedValue) == "PC")
                    {
                        EnvTestCertSelector.SelectedValue = 0;
                    }

                    EnvironmentConfigParam.getInstance().mBlazeParam.mGOSEnv = "stest";
                    EnvironmentConfigParam.getInstance().mBlazeParam.mRedirevtorURL = "https://winter15.gosredirector.stest.ea.com:42230/";
                    Binding testEnvBinding = new Binding();
                    testEnvBinding.Source = FindResource("TestEnv");
                    EnvEnvSelector.SetBinding(ComboBox.ItemsSourceProperty, testEnvBinding);
                    EnvEnvSelector.SelectedIndex = 0;
                }

                updateBlazeServerConfig();
            }
        }


        private void updateBlazeServerConfig()
        {
            // Updating the server name
            BlazeServerName.Text = "fifa-20" + (System.String)EnvYearSelector.SelectedValue + "-" + ((System.String)EnvPlatformSelector.SelectedValue).ToLower();

            if ((System.String)EnvTestCertSelector.SelectedValue != "1stPartyCert")
            {
                if (EnvEnvSelector.SelectedIndex > 0)
                {
                    BlazeServerName.Text += "-" + (System.String)EnvEnvSelector.SelectedValue;
                }
            }
            EnvironmentConfigParam.getInstance().mBlazeParam.mGameYear = (System.String)EnvYearSelector.SelectedValue;
            EnvironmentConfigParam.getInstance().mBlazeParam.mPlatform = (System.String)EnvPlatformSelector.SelectedValue;
        }

        private void BlazeServerName_TextChanged(object sender, TextChangedEventArgs e)
        {
            EnvironmentConfigParam.getInstance().mBlazeParam.mBlazeServerName = BlazeServerName.Text;
        }
    }
}
