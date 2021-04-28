#define USE_ACTIVATOR_CREATE_INSTANCE

using System;
using System.Reflection;

using NAnt.Core.Attributes;

#if !USE_ACTIVATOR_CREATE_INSTANCE
using LinqExp = System.Linq.Expressions;
#endif

namespace NAnt.Core.Reflection
{
    delegate Task ObjectActivator();

    internal class TaskBuilder
    {

#if !USE_ACTIVATOR_CREATE_INSTANCE
        ObjectActivator _activator;
#endif
        private readonly Type _taskType;
        internal readonly string TaskName;
        internal string AssemblyFileName
        {
            get
            {
                return _taskType.Assembly.Location;
            }
        }

        internal TaskBuilder(Type taskType, string taskName)
        {
#if !USE_ACTIVATOR_CREATE_INSTANCE
            _activator = null;
#endif
            _taskType = taskType;
            TaskName = taskName;
        }

        internal Task CreateTask()
        {
#if USE_ACTIVATOR_CREATE_INSTANCE
            Task task = Activator.CreateInstance(_taskType) as Task;
#else
            if (_activator == null)
            {
                _activator = GetActivator(_taskType);
            }
            Task task = _activator();
#endif
            task._elementName = TaskName;
            return task;
        }

#if !USE_ACTIVATOR_CREATE_INSTANCE


        private static ObjectActivator GetActivator(Type type)
        {
            var newExp = LinqExp.Expression.New(type);

            var lambda = LinqExp.Expression.Lambda(typeof(ObjectActivator), newExp);

            var compiled = lambda.Compile() as ObjectActivator;
            return compiled;
        }
#endif
    }
}
