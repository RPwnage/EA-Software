using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Reflection;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Linq;
using System.Text;

using NAnt.Core;
using NAnt.Core.Attributes;
using NAnt.Core.Threading;
using NAnt.Core.Util;

namespace NAnt.Core.Reflection
{
    public sealed class FunctionFactory
    {
        static FunctionFactory()
        {
            FUNCTION_ASSEMBLY_PATTERNS = new string[] { "Tasks.dll", "Tests.dll" };
        }

        internal static void GlobalInit()
        {
            _nantFunctions = new FunctionFactory();

            // initialize builtin NAnt.Core tasks
            _nantFunctions.AddFunctions(Assembly.GetExecutingAssembly());

            // scan the directory where NAnt.Core.dll lives (this assembly) and a tasks subfolder
            string nantDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            _nantFunctions.ScanDirectories(nantDir, Path.Combine(nantDir, "tasks"));
        }

        internal FunctionFactory()
        {
            if (_nantFunctions != null)
            {
                _methodCollection = new ConcurrentDictionary<MethodKey, MethodInfo>(_nantFunctions._methodCollection);
            }
            else
            {
                _methodCollection = new ConcurrentDictionary<MethodKey, MethodInfo>();
            }
        }

        internal FunctionFactory(FunctionFactory other)
        {
            _methodCollection = new ConcurrentDictionary<MethodKey, MethodInfo>(other._methodCollection);
        }


        /// <summary>
        /// Run a function given the nant function name. 
        /// This corresponds to the name defined in the FunctionAttribute by the static method.
        /// </summary>
        public string Run(string functionName, List<string> parameterList, Project project)
        {
            MethodInfo methodInfo = null;
            int numParam = parameterList.Count + 1; // add one for project

            bool retval = _methodCollection.TryGetValue(new MethodKey(functionName, numParam), out methodInfo);

            // check for existence of function
            if (retval == false || methodInfo == null)
            {
                string msg = String.Format("Function '{0}' with {1} parameters is undeclared.", functionName, numParam - 1);

                string found = _methodCollection.Where(kvp => kvp.Key.Name == functionName).Select(kvp => kvp.Value)
                    .ToString(Environment.NewLine + "\t", mi => mi.ReturnType.Name + " " + mi.Name + "(" + mi.GetParameters().ToString(", ", par => par.Name) + ")");

                if (!String.IsNullOrWhiteSpace(found))
                {
                    msg += " Found methods:" + Environment.NewLine + "\t" + found;
                }
                throw new BuildException(msg);
            }

            // set up the parameters
            object[] paramObj = new object[numParam];
            ParameterInfo[] parameterInfo = methodInfo.GetParameters();
            try
            {
                // the first param is the project
                paramObj[0] = project;

                // convert each string parameter into its requested type
#if NANT_CONCURRENT_DISABLE
                Parallel.For(0, parameterList.Count, i =>
#else
                for(int i = 0; i < parameterList.Count; i++) 
#endif
                {
                    try
                    {
                        Type type = parameterInfo[i + 1].ParameterType;
                        paramObj[i + 1] = PropertyConverter.Convert(type, parameterList[i]);
                    }
                    catch (Exception e)
                    {
                        string msg = String.Format("Failed to change parameter '{0}' with value '{1}' to type '{2}' in function '{3}'.",
                            i, parameterList[i], parameterInfo[i + 1].ParameterType.ToString(), functionName);
                        throw new BuildException(msg, e);
                    }
                }
#if NANT_CONCURRENT_DISABLE
);
#endif
            }
            catch (System.Exception e)
            {
                ThreadUtil.RethrowThreadingException("Error in FunctionFactory", Location.UnknownLocation, e);
            }

            // invoke the function to evalues the string
            string eval = null;

            try
            {
                eval = (string)methodInfo.Invoke(this, paramObj);
            }
            catch (System.Exception e)
            {
                string msg = String.Format("Function '{0}' failed to execute.", functionName);
                throw new BuildException(msg, e.InnerException);
            }
            return eval;
        }

        /// <summary>Scans directory list for any task assemblies and adds them to list of known functions.</summary>
        /// <param name="paths">The full path to the directory to scan.</param>
        internal void ScanDirectories(params string[] paths)
        {
#if NANT_CONCURRENT
            // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
            bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else
                bool parallel = false;
#endif
            if (parallel)
            {
                Parallel.ForEach(paths, path =>
                {
                    // Don't do anything if we don't have a valid directory path
                    if (!String.IsNullOrWhiteSpace(path) && Directory.Exists(path))
                    {
                        var assemblies = ReflectionUtil.GetAssemblyFiles(path, FUNCTION_ASSEMBLY_PATTERNS);

                        Parallel.ForEach(assemblies, assemblyFile =>
                        {
                            AddFunctions(AssemblyLoader.Get(assemblyFile));
                        });
                    }
                });
            }
            else
            {
                foreach (var path in paths)
                {
                    // Don't do anything if we don't have a valid directory path
                    if (!String.IsNullOrWhiteSpace(path) && Directory.Exists(path))
                    {
                        var assemblies = ReflectionUtil.GetAssemblyFiles(path, FUNCTION_ASSEMBLY_PATTERNS);

                        foreach (var assemblyFile in assemblies)
                        {
                            AddFunctions(AssemblyLoader.Get(assemblyFile));
                        }
                    }
                }
            }
        }

        /// <summary>Scans the given assembly for any classes derived from Function.</summary>
        /// <param name="assembly">The Assembly containing the new functions to be loaded.</param>
        /// <returns>The count of tasks found in the assembly.</returns>
        internal int AddFunctions(Assembly assembly, bool overrideExistingFunc, bool failOnError)
        {
            int retVal = 0;
            try
            {
#if NANT_CONCURRENT
                // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
                bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else
                bool parallel = false;
#endif
                if (parallel)
                {
                    Parallel.ForEach(assembly.GetTypes(), type =>
                    {
                        if (type.IsSubclassOf(typeof(FunctionClassBase)) && !type.IsAbstract)
                        {
                            Interlocked.Add(ref retVal, AddMethods(assembly, type, overrideExistingFunc, failOnError));
                        }
                    });
                }
                else
                {
                    foreach (Type type in assembly.GetTypes())
                    {
                        if (type.IsSubclassOf(typeof(FunctionClassBase)) && !type.IsAbstract)
                        {
                            Interlocked.Add(ref retVal, AddMethods(assembly, type, overrideExistingFunc, failOnError));
                        }
                    }
                }
            }
            catch (Exception e)
            {
                string msg = String.Format("Could not load functions from '{0}'.", assembly.Location);
                throw new BuildException(msg, e);
            }
            return retVal;
        }

        internal int AddFunctions(Assembly assembly)
        {
            return AddFunctions(assembly, false, false);
        }

        private int AddMethods(Assembly assembly, Type type, bool overrideExistingFunc, bool failOnError)
        {
            int retVal = 0;

            // check if the class has defined it self as a function class attribute
            FunctionClassAttribute functionClassAttribute = Attribute.GetCustomAttribute(type, typeof(FunctionClassAttribute)) as FunctionClassAttribute;
            if (functionClassAttribute == null)
                return retVal;

#if NANT_CONCURRENT
            // Under mono most parallel execution is slower than consequtive. Untill this is resolved use consequtive execution 
            bool parallel = (PlatformUtil.IsMonoRuntime == false);
#else
            bool parallel = false;
#endif
            // find each method of the class which is defined as a function attribute
            if (parallel)
            {
                Parallel.ForEach(type.GetMethods(), methodInfo =>
                {
                    FunctionAttribute attribute = GetFunctionAttribute(methodInfo);
                    if (attribute != null)
                    {
                        Interlocked.Add(ref retVal, AddMethod(methodInfo, overrideExistingFunc, failOnError));
                    }
                });
            }
            else
            {
                var  methods = type.GetMethods();

                for(int i = 0; i < methods.Length; i++)
                {
                    MethodInfo methodInfo = methods[i];

                    FunctionAttribute attribute = GetFunctionAttribute(methodInfo);
                    if (attribute != null)
                    {
                        Interlocked.Add(ref retVal, AddMethod(methodInfo, overrideExistingFunc, failOnError));
                    }
                }
            }

            return retVal;
        }


        private int AddMethod(MethodInfo methodInfo, bool overrideExistingMethod, bool failOnError)
        {
            // check for static method
            if (methodInfo.IsStatic == false)
            {
                string msg = String.Format("Function '{0}' must be declared static.", methodInfo.Name);
                throw new BuildException(msg);
            }

            // check return type
            if (methodInfo.ReturnType != typeof(String))
            {
                string msg = String.Format("Function '{0}' must return a string.", methodInfo.Name);
                throw new BuildException(msg);
            }

            // get parameters
            ParameterInfo[] parameterInfo = methodInfo.GetParameters();

            // check first param type
            if ((parameterInfo.Length < 1 || parameterInfo[0].ParameterType != typeof(Project)))
            {
                string msg = String.Format("Function '{0}' must have the first parameter set to type {1}.", methodInfo.Name, typeof(Project).ToString());
                throw new BuildException(msg);
            }

            MethodKey methodKey = new MethodKey(methodInfo);

            int ret = 0;

            // check if function exists, functions are distinguished by name and number of params (not type)
            if (overrideExistingMethod)
            {
                _methodCollection[methodKey] = methodInfo;
                ret = 1;
            }
            else
            {
                if (_methodCollection.TryAdd(methodKey, methodInfo))
                {
                    ret = 1;
                }
                else if (failOnError)
                {
                    throw new BuildException("ERROR: Function '" + methodInfo.Name + "' already exists!");
                }
                else
                {
                    ret = 0;
                }
            }

            return ret;
        }

        private static FunctionAttribute GetFunctionAttribute(MethodInfo method)
        {
            object[] functionAttributes = method.GetCustomAttributes(typeof(FunctionAttribute), false);
            if (functionAttributes.Length > 0)
            {
                return (FunctionAttribute)functionAttributes[0];
            }
            return null;
        }


        private class MethodKey
        {
            readonly internal string Name;
            readonly internal int NumParams;

            internal MethodKey(string name, int numParams)
            {
                Name = name;
                NumParams = numParams;
            }

            internal MethodKey(MethodInfo info)
                : this(info.Name, info.GetParameters().Length)
            {
            }

            public override int GetHashCode()
            {
                return (Name + NumParams.ToString()).GetHashCode();
            }

            public override bool Equals(object obj)
            {
                MethodKey other = obj as MethodKey;
                if (Name != other.Name)
                {
                    return (false);
                }
                return (NumParams == other.NumParams);
            }
        };

        private readonly ConcurrentDictionary<MethodKey, MethodInfo> _methodCollection;
        private static FunctionFactory _nantFunctions;
        private static readonly string[] FUNCTION_ASSEMBLY_PATTERNS;
    }
}
