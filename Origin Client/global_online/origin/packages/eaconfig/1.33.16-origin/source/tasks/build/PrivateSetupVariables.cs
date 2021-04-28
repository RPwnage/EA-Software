using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Logging;
using NAnt.Core.Functions;


using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace EA.Eaconfig
{
    [TaskName("PrivateSetupVariables")]
    public class PrivateSetupVariables : EaconfigBuildTask
    {

        public BaseBuildData BuildData
        {
            get { return _buildData; }
        }


        public static BaseBuildData Execute(Project project, string groupName, string buildModule, BaseBuildData buildData)
        {
            PrivateSetupVariables task = new PrivateSetupVariables(project, groupName, buildModule, buildData);

            task.ExecuteTask();
            return task.BuildData;
        }

        public PrivateSetupVariables(Project project, string groupName, string buildModule, BaseBuildData buildData)
            : base(project, groupName, buildModule)
        {
            if (String.IsNullOrEmpty(GroupName))
            {
                Error.Throw(Project, Location, "PrivateSetupVariables", "'GroupName' parameter is empty.");
            }
            if (buildData == null)
            {
                Error.Throw(Project, Location, "PrivateSetupVariables", "'BaseBuildData' parameter is null.");
            }

            _buildData = buildData;
        }

        public PrivateSetupVariables()
            : base()
        {
            _buildData = new BaseBuildData();
        }

        /// <summary>Execute the task.</summary>
        protected override void ExecuteTask()
        {
            ComputeBaseBuildData computedata = new ComputeBaseBuildData(Project, ConfigSystem, ConfigCompiler, Properties[PropGroupName("buildtype")], GroupName, _buildData);

            computedata.Init();

            computedata.Compute();

            computedata.SaveNantVariables();

            _buildData = computedata.BuildData;
        }

        private BaseBuildData _buildData;

    }
}
