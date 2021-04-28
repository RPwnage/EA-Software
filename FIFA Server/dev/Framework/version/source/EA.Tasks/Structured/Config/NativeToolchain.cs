using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Tasks;
using System;

namespace EA.Eaconfig.Structured
{
	[TaskName("nativetoolchain", StrictAttributeCheck = true, NestedElementsCheck = true)]
	public sealed class NativeToolchain : Task
	{
		[BuildElement("compiler", Required = true)]
		public CompilerTask Compiler { get; private set; } = new CompilerTask();

		[BuildElement("assembler", Required = true)]
		public AssemblerTask Assembler { get; private set; } = new AssemblerTask();

		[BuildElement("linker", Required = true)]
		public LinkerTask Linker { get; private set; } = new LinkerTask();

		[BuildElement("librarian", Required = true)]
		public LibrarianTask Librarian { get; private set; } = new LibrarianTask();

		protected override void ExecuteTask()
		{
			// DAVE-FUTURE-REFACTOR-TODO: be good to make this implicit parented and executed, maybe as a [TaskElememt] attribute
			Compiler.Parent = this;
			Compiler.Execute();
			Assembler.Parent = this;
			Assembler.Execute();
			Linker.Parent = this;
			Linker.Execute();
			Librarian.Parent = this;
			Librarian.Execute();

			// TODO: currently this just wraps the tool tasks (but it does make sure you use all of them!) but more will be added here
			// in future to validate native toolchain config setup
		}
	}
}