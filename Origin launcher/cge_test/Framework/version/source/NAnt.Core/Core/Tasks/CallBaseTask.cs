// NAnt - A .NET build tool
// Copyright (C) 2003-2020 Electronic Arts Inc.


using NAnt.Core.Attributes;

namespace NAnt.Core.Tasks
{

	/// <summary>
	/// Calls a NAnt target.
	/// </summary>
	/// <include file='Examples/CallBase/Simple.example' path='example'/>
	[TaskName("call-base", NestedElementsCheck = true)]
	public class CallBaseTask : Task {

		protected override void ExecuteTask()
		{
			try
			{
				Target owningTarget = null;
				object parent = this.Parent;
				while(parent != null)
				{
					if (parent is Target)
					{
						owningTarget = (Target)parent;
						break;
					}
					else
					{
						parent = ((Element)parent).Parent;
					}
				}
				if (owningTarget != null)
				{
					if (owningTarget.BaseTarget == null)
					{
						throw new BuildException($"Error in call-base task. No base target found for target: {owningTarget}.");
					}
					owningTarget.BaseTarget.Execute(Project);					
				}
				else
				{
					throw new BuildException($"Error in call-base task. Unable to find owning target for this call."); 
				}
				
			}
			catch (ContextCarryingException e)
			{
				//Reset location
				var frame = e.PopNAntStackFrame();
				e.PushNAntStackFrame(frame.MethodName, frame.ScopeType, Location);
				//throw e instead of throw to reset the stacktrace
				throw e;
			}
		}
	}
}
