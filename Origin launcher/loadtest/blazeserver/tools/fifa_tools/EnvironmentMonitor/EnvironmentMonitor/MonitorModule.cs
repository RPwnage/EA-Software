using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Shapes;
using System.Windows.Media;

using System.ComponentModel;
using System.Threading;
using System.Reflection;

namespace EnvironmentMonitor
{
    public class ModuleParam
    {
        public ModuleParam(){}

        public String AllParams
        {
            get
            {
                // Instantiate a String Builder
                // Get the Type of the ModuleParam instance
                // Get the list of Fields from the Type
                // Enumerate through the Fields and append the value to String Builder

                StringBuilder paramDump = new StringBuilder();

                Type paramObjectType = this.GetType();
                FieldInfo[] paramFieldsInfo = paramObjectType.GetFields();

                foreach (FieldInfo field in paramFieldsInfo)
                {
                    paramDump.AppendLine(String.Format("{0}:   {1}", field.Name, field.GetValue(this)));
                }

                return paramDump.ToString();
            }
            set{}
        }
    }

    public interface RuleCallbackInterface
    {
        void HandleRuleEvaluationComplete(bool bCriticalError = false);
        void HandleRecommendationUpdate(object sender, PropertyChangedEventArgs e);
    }

    public class RuleReportUpdatedEvent : PropertyChangedEventArgs
    {
        public string report;
        public RuleReportUpdatedEvent(string propertyName) : base(propertyName) { }
    }

    public class Rule : INotifyPropertyChanged
    {
        /*
         * mModule is the associated server module of this rule
         */
        public MonitorModule mModule;

        /*
         * mCallbackObj is the callback interface instance when this rule completed evaluation
         */
        protected RuleCallbackInterface mCallbackObj;

        /*
         * uiContext is the UI Thread Context, the rule use this context to send message to the UI Thread to update GUI
         */
        protected SynchronizationContext uiContext;

        /*
         * workerThread is the workerthread that handles the actual evaluation on a separate worker thread
         */
        protected BackgroundWorker workerThread;

        /*
         * mRuleName - name property of the rule
         */
        public String mRuleName { get; set; }

        /*
         * mRuleName - name property of the rule
         */
        public String RuleStatus 
        {
            get
            {
                if (mbSatisfied)
                    return "Pass";
                else
                    return "Failed";
            } 
            set
            {
                RuleStatus = value;
            }
        }
        /*
         * mbSatisfied is a boolean property representing whether the rule is satisfied
         */
        public bool mbSatisfied
        {
            get;
            set;
        }

        /*
         * mbGiveRecommendation is a boolean property representing whether the rule is satisfied
         */
        public bool mbGiveRecommendation;

        /*
         * mReport is the string pipe to output TTY during rule evaluation
        */
        protected StringBuilder mReport;

        /*
         * RuleReport is the property that reflects the TTY from mReport to the GUI
        */
        public String RuleReport
        {
            get
            {
                return mReport.ToString();
            }
            set
            {
                var e = new RuleReportUpdatedEvent("RuleReport");
                e.report = mReport.ToString();
                if (PropertyChanged != null)
                {
                    PropertyChanged(this, e);
                }
            }
        }

        /*
         * mReport is the string pipe to output TTY during rule evaluation
        */
        protected StringBuilder mRecommend;

        /*
         * Recommendation is the property that reflects the TTY from mReport to the GUI
        */
        public String Recommendation
        {
            get
            {
                return mRecommend.ToString();
            }
            set
            {
                var e = new RuleReportUpdatedEvent("Recommendation");
                e.report = mRecommend.ToString();
                if (PropertyChanged != null)
                {
                    PropertyChanged(this, e);
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        /*
         * Rule constructor
         * During construction of a rule, it will do the below
         * 1. Associate the rule to the given server module
         * 2. Add this rule to the server module
         * 3. Associate the rule evaluation complete callback interface object
         * 4. Allocate and setup the mReport TTY
         * 5. initialized the rule name and mbSatisfied flag
        */
        public Rule(MonitorModule module, RuleCallbackInterface callbackObj, String name=null)
        {
            mModule = module;
            mModule.addRule(this);
            mCallbackObj = callbackObj;
            mReport = new StringBuilder();
            mRecommend = new StringBuilder();
            mbSatisfied = false;
            mbGiveRecommendation = true;

            if (name != null)
            {
                mRuleName = name;
            }
            else{
                mRuleName = this.GetType().Name;
            }

            PropertyChanged += new PropertyChangedEventHandler(mCallbackObj.HandleRecommendationUpdate);
        }

        /*
         * PerformEvaluation
         * PerformEvaluation will setup and start the workerThread to perform evaluation
        */
        public virtual void PerformEvaluation()
        {
            uiContext = SynchronizationContext.Current;
            
            mbSatisfied = false;
            mbGiveRecommendation = true;

            mReport.AppendLine(String.Format("Performing - {0}", mRuleName));
            workerThread = new BackgroundWorker();
            workerThread.DoWork += new DoWorkEventHandler(evaluate);
            workerThread.RunWorkerAsync(uiContext);            
        }

        /*
         * onEvaluationComplete
         * onEvaluationComplete is the post workerThread completion handler
         * This needs to be called by the UI Thread, and it will update the GUI and call into other post evaluation handler
        */
        protected virtual void onEvaluationComplete(object state)
        {
            mReport.AppendLine(String.Format("{0} completed\n", mRuleName));
            mModule.updateStatus();
            mCallbackObj.HandleRuleEvaluationComplete();            
        }

        /*
         * evaluate
         * evaluate should contain the actual evaluation logic of each rule
        */
        protected virtual void evaluate(object sender, DoWorkEventArgs e) 
        {
            // Override this function to put in the actual evaluation logic

            // Once Evaluation is completed, make a call as below to flag completion to UI Thread to update the GUI
            uiContext.Post(onEvaluationComplete, this);
        }
    }

    public class MonitorModule : INotifyPropertyChanged
    {
        protected List<Rule> mRules = new List<Rule>();

        protected Ellipse mStatusControl;
        protected TextBox mOperationText;
        protected int mRulesEvaluated;
        public ModuleParam Param { get; set; } 

        public String mName = "NoName";
        protected ToolTip mTooltip;

        private String mReport;
        public String ModuleReport
        {
            get
            {
                return mReport;
            }
            set
            {
                mReport = value;
                var e = new PropertyChangedEventArgs("ModuleReport");
                if (PropertyChanged != null)
                {
                    PropertyChanged(this, e);
                }
            }
        } 
        
        // Setting up a Property Changed Event to publish ModuleReport change event
        // this will update the ModuleDetailReport window to update the ModuleReport panel as
        // rules of the modules are being evaluated
        public event PropertyChangedEventHandler PropertyChanged;

        public MonitorModule(ModuleParam param, Ellipse statusControl, TextBox operationText, String name = null)
        {            
            mStatusControl = statusControl;
            mOperationText = operationText;
            Param = param;

            if (name != null)
            {
                mName = name;
            }

            mTooltip = new ToolTip();
            ToolTipService.SetToolTip(mStatusControl, mTooltip);

            ModuleReport = " ";
        }

        public void resetStatus()
        {
            Color rectColor = new Color();
            rectColor.R = 255;
            rectColor.G = 255;
            rectColor.B = 255;
            rectColor.A = 200;

            SolidColorBrush solidBrush = new SolidColorBrush(rectColor);
            mStatusControl.Fill = solidBrush;
            mOperationText.Text = "";
            mRulesEvaluated = 0;

            mRules.Clear();
        }

        public void updateStatus()
        {
            int interval = (510 / mRules.Count);
            byte residual = (byte)(255 % interval);

            Color rectColor = new Color();
            rectColor.R = 255;
            rectColor.G = 0;
            rectColor.B = 0;
            rectColor.A = 200;

            String statusDes = "Double-Click for detail - \n";

            ModuleReport = "";
            foreach (Rule rule in mRules)
            {
                if (rule.mbSatisfied)
                {
                    if (rectColor.G < 255)
                    {
                        byte leftOver = (byte)(255 - rectColor.G);
                        if (leftOver <= interval)
                        {
                            rectColor.G = 255;
                            rectColor.R -= (byte)(interval - leftOver);
                        }
                        else
                        {
                            rectColor.G += (byte)interval;
                        }
                    }
                    else
                    {
                        rectColor.R -= (byte)interval;
                    }
                }
                else
                {
                    if (rule.mbGiveRecommendation)
                    {
                        // Trigger Recommendation listener to show the error log
                        rule.Recommendation = rule.Recommendation;
                        rule.mbGiveRecommendation = false;
                    }
                }

                statusDes += rule.mRuleName + "-" + (rule.mbSatisfied ? "Pass\n" : "Failed\n");
                
                ModuleReport += rule.RuleReport;
            }            

            SolidColorBrush solidBrush = new SolidColorBrush(rectColor);            
            mStatusControl.Fill = solidBrush;
            mTooltip.Content = statusDes;

            mRulesEvaluated++;

            if (mRulesEvaluated > 0)
            {        
                if(mRulesEvaluated < mRules.Count)
                {
                    mOperationText.Text = "Evaluating";
                }
                else if (mRulesEvaluated == mRules.Count)
                {
                    mOperationText.Text = "Module Tested";
                }

            }
        }

        
        public void addRule(Rule rule) { 
            mRules.Add(rule); 
        }

        public void removeRule(Rule rule) { mRules.Remove(rule); }

        public List<Rule> getRules() { return mRules;}
    }
}
