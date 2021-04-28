
#if UNUSED  //	TODO: Remove this UNUSED block to activate the code.
namespace NAnt.PerforceTasks	//	TODO: Replace 'PerforceTasks' with your namespace. 
{
	/// <summary>
	/// TODO: Enter the description of your task here and modify the sample below.
	/// Synchronize files between the depot and the workspace.
	/// </summary>
	/// <remarks>
	/// <para>See the 		
	/// <a href="http://www.perforce.com/perforce/doc.021/manuals/cmdref">Perforce User's Guide</a> 
	/// for more information.</para>
	/// </remarks>
	/// <example>
	/// TODO: Explain what you example does, and file in the examples below.
	/// <para>Perforce Commands:</para>
	///	<code><![CDATA[
	///	p4 task_name
	///	p4 task_name here with param1, param2, param3.]]></code>
	///	Equivalent NAnt Tasks:
	///	<code><![CDATA[
	///	<task_name />
	///	<task_name param1="value1" param2="value2" param3="value3" /> ]]></code>
	/// </example>
	[TaskName("task_name")]		//	TODO: Uncomment this line and change task_name to the name you want nant to use for your task.
	public class Task_NameTask : ExternalProgramBase 
	{
		//	TODO: Declare local storage here to match the input params expected.
		string	_files	= String.Empty;
		string	_param1 = null;
		string	_param2 = null;
		string	_param3	= null;

		// TODO: Add Properties that match the expected params, and name them appropriately.
		//	The TaskAttribute will assoscate the Property with the XML tag automagically.
		//	You can also use [Int32Validator([lo,hi])] to check for valid types
	   
		/// <summary>The files to operate on.</summary>
		[TaskAttribute("files")]
		public string Files			{ get { return _files; } set {_files = value; } }
		
		/// <summary>Describe Param1.</summary>
		[TaskAttribute("param1", Required=true)]
		public string Param1		{ get { return _param1; } set {_param1 = value; } }
		
		/// <summary>Describe Param2.</summary>
		[TaskAttribute("param2")]
		public string Param2		{ get { return _param2; } set {_param2 = value; } }
		
		/// <summary>Describe Param3.</summary>
		[TaskAttribute("param3")]
		public string Param3		{ get { return _param3; } set {_param3 = value; } }
		
		/// <summary>Gets the command line arguments for the application.</summary>
		/// TODO:	This override NOT needed if you are going to get/put a form to stdin/out.
		public override string ProgramArguments		
		{ 
			get	
			{ 
				//	TODO: Change this routine so it returns all your command line args in a single string.
				string options = GlobalOptions + " local_command_here ";
				if (Param1 != null)
					options += " -local_option1";
				if (Param2 != null)
					options += " -local_option2";
				options += " ";
				return options + Files;
			} 
		}	//	end ProgramArguments

		//	TODO: Only include this override if the perforce task deals with forms transmitted via stdin/sout.
		/// <summary>
		/// Executes the Task_Name task
		/// </summary>
		protected override void ExecuteTask() 
		{
			P4Form	form	= new P4Form(ProgramFileName, BaseDirectory, FailOnError);

			//	TODO: Build a command line to send to p4. It should probably include a "-o" to tell 
			//	p4 to output the form on stdout.  Add in any other command line options as needed.
			string	command = GlobalOptions + " Task_Name -o";

			try	
			{
				//	Execute the command and retreive the form.
				Log.WriteLineIf(Verbose, LogPrefix + "{0}>{1} {2}", form.WorkDir, form.ProgramName, command);
				form.GetForm(command);

				//	TODO: Replace the data in the form with the new data.
				form.ReplaceField("Field_name_1:", "field_value_1" );
				form.ReplaceField("Field_name_2:", "field_value_2" );

				//	Now send the modified form back to Perforce.
				command = GlobalOptions + " Task_Name -i";
				Log.WriteLineIf(Verbose, LogPrefix + "{0}>{1} {2}", form.WorkDir, form.ProgramName, command);
				form.PutForm(GlobalOptions + command);

				//	TODO: Possibly parse the resulting form (stdout from this last P4 command and 
				//	extract any useful info you might be able to locate.
				string changeProp = form.GetField("some_tag_for_the_data_you_want");  
			}
			catch (BuildException e) 
			{
				if (FailOnError)
					throw new BuildException(this.GetType().ToString() + ": Error during P4 program execution.", Location, e);
				else 
					Log.WriteLine(LogPrefix + "P4 returned an error. {0}", Location);
			}
		}	// end ExecuteTask
	}	// end class Task_NameTask
}
#endif