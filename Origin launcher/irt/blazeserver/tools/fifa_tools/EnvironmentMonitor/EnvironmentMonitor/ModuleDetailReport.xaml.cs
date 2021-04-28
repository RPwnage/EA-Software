using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace EnvironmentMonitor
{
    /// <summary>
    /// Interaction logic for ModuleDetailReport.xaml
    /// </summary>
    public partial class ModuleDetailReport : Window
    {
        private MonitorModule mModule;
        public ModuleDetailReport()
        {
            InitializeComponent();            
        }

        public void setModule(MonitorModule module)
        {
            mModule = module;
            RulesBox.DataContext = mModule.getRules();
            ParamBox.DataContext = mModule;
            ModuleLogBox.DataContext = mModule;
        }

        public MonitorModule getModule() { return mModule; }

        private void ShowRuleExecutionDetail(object sender, RoutedEventArgs e)
        {
            Button buttonClick = (Button)sender;
            Rule rule = (Rule)buttonClick.DataContext;

            RuleLogBox.DataContext = rule;
        }

        protected override void OnActivated(EventArgs e)
        {
            base.OnActivated(e);
        }
    }
}
