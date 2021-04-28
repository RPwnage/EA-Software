// (c) Electronic Arts. All Rights Reserved.
//-----------------------------------------------------------------------------
// mergeoption.cs
//
// NAnt custom task which merges options  from a NAnt code-build script.
// This differs from the built-in Framework mergeoption in that it is using C#
// instead of nAnt tasks to do the work in order to increase performance
//
// Written by:
//		Alex Van der Star (avdstar@ea.com)
//
// (c) 2005 Electronic Arts, Inc.
//-----------------------------------------------------------------------------
using System;
using System.Text;
using System.Collections;
using System.Xml ;


using NAnt.Core;
using NAnt.Core.Attributes;


namespace EA.Eaconfig
{
	[TaskName("MergeOptionset")]
    public class MergeOptionset : Task
	{
		string _originaloptionset	=	null;
		string _fromoptionset		=	null;
		//bool _verbose				=	false;

        public static void Execute(Project project, string optionsetName, string baseOptionsetName)
        {
            MergeOptionset task = new MergeOptionset();
            task.Project = project;
            task.OriginalOptionset = optionsetName;
            task.FromOptionset = baseOptionsetName;
            task.Execute();
        }

		[TaskAttribute("OriginalOptionset", Required=true)]
		public string OriginalOptionset
		{
			get { return _originaloptionset; }
			set { _originaloptionset = value; }
		}

		[TaskAttribute("FromOptionset", Required=true)]
		public string FromOptionset
		{
			get { return _fromoptionset; }
			set { _fromoptionset = value; }
		}

		/// <summary>Execute the task.</summary>
		protected override void ExecuteTask()
		{
			OptionSet fromOS = Project.NamedOptionSets[_fromoptionset];
			OptionSet originalOS = Project.NamedOptionSets[_originaloptionset];

            if (fromOS == null)
            {
                string errorMessage = String.Format("ERROR:  OptionSet '{0}' specified as input to MergeOptionset task does not exist.", _fromoptionset);
                throw new BuildException(errorMessage);
            }

            if (originalOS == null)
            {
                string errorMessage = String.Format("ERROR:  OptionSet '{0}' specified for as output for the MergeOptionset task does not exist.", _originaloptionset);
                throw new BuildException(errorMessage);
            }

			foreach (DictionaryEntry entry in fromOS.Options)
			{
				string name  = (string) entry.Key;
				MergeOptionTask.MergeOptionValue( originalOS, name, entry.Value.ToString() ) ;
			}
		}

		public override bool ProbingSupported 
		{
			get { return true; }
		}
	}


	[TaskName("MergeOption")]
	public class MergeOptionTask : Task
	{
		string _optionset		=	null;
		string _optionName		=	null;
		string _optionValue     =   null ;

		[TaskAttribute("Optionset", Required=true)]
		public string Optionset
		{
			get { return _optionset; }
			set { _optionset = value; }
		}

		[TaskAttribute("OptionName", Required=true)]
		public string OptionName
		{
			get { return _optionName; }
			set { _optionName = value; }
		}

		//[Property("OptionValue", Required=true)]
		public string OptionValue
		{
			get { return _optionValue; }
			set { _optionValue = value; }
		}

		protected override void InitializeTask( XmlNode taskNode )
		{
			XmlElement valueElement = (XmlElement) taskNode.SelectSingleNode("OptionValue");
			if ( valueElement == null )
			{
				throw new BuildException( String.Format("Missing required <OptionValue> element.") );
			}
			OptionValue = Project.ExpandProperties( valueElement.InnerText ) ;
		}

		protected override void ExecuteTask()
		{
			OptionSet optionset = Project.NamedOptionSets[Optionset];
			if( null != optionset )
			{
				MergeOptionValue( optionset, OptionName, OptionValue ) ;
			}
		}

		static public void MergeOptionValue( OptionSet optionset, string name, string val )
		{
			string origVal ;
			origVal = optionset.Options[name] ;

			if( null != origVal && origVal.Length > 0 )
			{
				// replicate original MergeOption's behaviour, merge the option
				// only if it does not already exist. it is not entirely correct as 
				// the value may be part of substring, as in:
				//   "include/foo" "include/foobar"
				// employ simple substring approach for now...
				if( origVal.IndexOf( val ) >= 0 )
				{
					// do nothing: we already have it
					return;
				}

                // If the original value is a true/false, on/off, or custom, we'll just keep the original
                // value
                string origValCaps = origVal.ToUpper();
                if (origValCaps == "TRUE" || origValCaps == "FALSE" || origValCaps == "ON" || origValCaps == "OFF" ||
                    origValCaps == "CUSTOM")
                {
                    // do nothing.  These are mutually exclusive values and cannot be combined.  So
                    // we're just going to keep the original value.
                    return;
                }

				// append new value to the end, along with separator
                
				val = origVal + " " + val ;
            }

            optionset.Options[name] = val;
		}

		public override bool ProbingSupported 
		{
			get { return true; }
		}
	}

}
