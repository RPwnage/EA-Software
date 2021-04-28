using System;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Threading;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Diagnostics;


namespace EA.MetricsProcessor
{
    public class PipeServer
    {
        // map for converting the config IDs sent by the nant build process
        // to the real configIDs that are contained in the database.
        static Dictionary<int, int> mConfigIdDictionary;
        static Dictionary<int, int> mPackageVersionIdDictionary;
        static Dictionary<int, int> mTargetIdDictionary;
        static Queue<string> mEventQueue;
        static DataStore dataStore = null;
        static object queueLock;
        static bool run;

        public static void Main()
        {
            try
            {
                //IM: Hopefully temporary measure. WE have unhandled exceptions in some worker threads
                AppDomain.CurrentDomain.UnhandledException +=
                        new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);

                run = true;

                dataStore = new DataStore();

                mConfigIdDictionary = new Dictionary<int, int>();
                mPackageVersionIdDictionary = new Dictionary<int, int>();
                mTargetIdDictionary = new Dictionary<int, int>();
                queueLock = new object();
                mEventQueue = new Queue<string>();

                Thread databaseThread = new Thread(DatabaseThread);
                databaseThread.Start();

                Thread.Sleep(250);
                TimeSpan thirtySeconds = new TimeSpan(0, 0, 30);
                DateTime dtExitTime = DateTime.Now + thirtySeconds;

                while (run && (DateTime.Now < dtExitTime))
                {
                    using (NamedPipeServerStream pipeServer =
                        new NamedPipeServerStream("FrameworkTelemetryPipe", PipeDirection.In, 1, PipeTransmissionMode.Byte, PipeOptions.Asynchronous))
                    {

                        var asyncResult = pipeServer.BeginWaitForConnection(PipeConnected, null);

                        if (asyncResult.AsyncWaitHandle.WaitOne(dtExitTime - DateTime.Now))
                        {
                            pipeServer.EndWaitForConnection(asyncResult);
                        }
                        else
                        {
                            // terminate the wait
                            run = false;
                            continue;
                        }

                        StreamString ss = new StreamString(pipeServer);

                        lock (queueLock)
                        {
                            while (true)
                            {
                                string strBuildEvent;
                                try
                                {

                                    // Verify our identity to the connected client using a
                                    // string that the client anticipates.
                                    strBuildEvent = ss.ReadString();
                                }
                                // Catch the IOException that is raised if the pipe is broken
                                // or disconnected.
                                catch (IOException e)
                                {
                                    Console.WriteLine("ERROR: {0}", e.Message);
                                    strBuildEvent = string.Empty;
                                }

                                if (strBuildEvent != string.Empty)
                                {
                                    if (strBuildEvent == "TERMINATE")
                                    {
                                        run = false;
                                        pipeServer.Close();
                                        break;
                                    }
                                    dtExitTime = DateTime.Now + thirtySeconds;

                                    mEventQueue.Enqueue(strBuildEvent);
                                    Monitor.PulseAll(queueLock);
                                }
                                else
                                {
                                    pipeServer.Close();
                                    break;
                                }
                            }
                        }
                    }
                }

                databaseThread.Join(1000);
                databaseThread.Abort();
            }
            catch (Exception)
            {
            }
        }

        private static void PipeConnected(IAsyncResult e)
        {
        }

        private static void DatabaseThread(object data)
        {
            try
            {
                while (run)
                {
                    string strBuildEvent;
                    lock (queueLock)
                    {
                        while (mEventQueue.Count == 0) Monitor.Wait(queueLock);
                        strBuildEvent = mEventQueue.Dequeue();
                    }

                    if (strBuildEvent == string.Empty)
                    {
                        Console.WriteLine("Terminating Database comm thread");
                        run = false;
                        continue;
                    }

                    Console.WriteLine(strBuildEvent);

                    if (strBuildEvent.StartsWith("START BUILD"))
                    {
                        Regex re = new Regex(@"START BUILD&machineName=([^&]*)&WorkingSet=([^&]*)&nantVersion=([^&]*)&NTLogIn=([^&]*)&IsServiceAccount=([^&]*)&BuildFileName=([^&]*)&BuildCmdLine=(.*)");
                        string[] strSplit = re.Split(strBuildEvent);

                        int machineId = dataStore.InsertUpdateMachineInfo(
                            strSplit[1],            /* machineName */
                            (int)Environment.OSVersion.Platform,
                            Environment.OSVersion.VersionString,
                            Environment.ProcessorCount,
                            int.Parse(strSplit[2])  /* (Environment.WorkingSet /(1024*8)) */
                            );

                        dataStore.BuildStarted(
                            strSplit[3],            /* Assembly.GetExecutingAssembly().GetName().Version.ToString() */
                            strSplit[4],            /* GetAccountName() */
                            machineId,
                            strSplit[5] == "True",  /* System.Security.Principal.WindowsIdentity.GetCurrent().IsSystem */
                            strSplit[6],            /* buildData.BuildFileName */
                            strSplit[7]             /* commandLine.ToString() */
                            );
                    }
                    else if (strBuildEvent.StartsWith("FINISH BUILD"))
                    {
                        Regex re = new Regex(@"FINISH BUILD&timeInMS=([^&]*)&status=(.*)");
                        string[] strSplit = re.Split(strBuildEvent);

                        dataStore.BuildFinished(
                            (int)(float.Parse(strSplit[1])),    /* buildTime.TotalMilliseconds */
                            strSplit[2] == "Success" ? 1 : 2    /* data.Status */
                        );

                        mConfigIdDictionary.Clear();
                        mPackageVersionIdDictionary.Clear();
                        mTargetIdDictionary.Clear();
                    }
                    else if (strBuildEvent.StartsWith("CONFIGID"))
                    {
                        Regex re = new Regex(@"CONFIGID&configName=([^&]*)&configSystem=([^&]*)&configCompiler=(.*)");
                        string[] strSplit = re.Split(strBuildEvent);

                        int id = dataStore.GetConfigID(strSplit[1], strSplit[2], strSplit[3]);

                        int hashCode = strBuildEvent.GetHashCode();

                        if (!mConfigIdDictionary.ContainsKey(hashCode))
                            mConfigIdDictionary.Add(hashCode, id);
                    }
                    else if (strBuildEvent.StartsWith("PACKAGEUPDATE"))
                    {
                        Regex re = new Regex(@"PACKAGEUPDATE&PackageVersionId=([^&]*)&ConfigID=([^&]*)&BuildGroup=([^&]*)&buildTargetID=([^&]*)&IsBuildDependency=([^&]*)&IsAutobuild=([^&]*)&FrameworkVersion=([^&]*)&Result=([^&]*)&BuildTimeMillisec=(.*)");
                        string[] strSplit = re.Split(strBuildEvent);

                        int? targetId = null;
                        if (strSplit[4] != string.Empty)
                            targetId = mTargetIdDictionary[int.Parse(strSplit[4])];

                        dataStore.InsertUpdatePackageInfo(
                            mPackageVersionIdDictionary[int.Parse(strSplit[1])], /* cachedPackageVersion.DbVersionKey*/
                            mConfigIdDictionary[int.Parse(strSplit[2])],    /* configId */
                            strSplit[3],                                    /* buildGroup */
                            targetId,
                            strSplit[5] == "True",                          /* data.IsBuild */
                            strSplit[6] == "True",                          /* data.Autobuild */
                            int.Parse(strSplit[7]),                         /* frameworkVersion */
                            int.Parse(strSplit[8]),                         /* result */
                            int.Parse(strSplit[9])                          /* buildTime.TotalMilliseconds */
                            );
                    }
                    else if (strBuildEvent.StartsWith("PACKAGE"))
                    {
                        int packageVersionId;

                        Regex re = new Regex(@"PACKAGE&PackageName=([^&]*)&PackageVersion=([^&]*)&ConfigID=([^&]*)&BuildGroup=([^&]*)&buildTargetID=([^&]*)&IsBuildDependency=([^&]*)&IsAutobuild=([^&]*)&FrameworkVersion=([^&]*)&Result=([^&]*)&BuildTimeMillisec=(.*)");
                        string[] strSplit = re.Split(strBuildEvent);

                        int? targetId = null;
                        if (strSplit[5] != string.Empty)
                            targetId = mTargetIdDictionary[int.Parse(strSplit[5])];

                        int packageId = dataStore.InsertUpdatePackageInfo_GetPackageVersionID(
                            strSplit[1],                /* packageName */
                            strSplit[2],                /* packageVersion */
                            mConfigIdDictionary[int.Parse(strSplit[3])], /* configID */
                            strSplit[4],                /* buildGroup */
                            targetId,                   /* targetId */
                            strSplit[6] == "True",      /* data.IsBuild */
                            strSplit[7] == "True",      /* data.Autobuild */
                            int.Parse(strSplit[8]),     /* frameworkVersion */
                            int.Parse(strSplit[9]),     /* result */
                            int.Parse(strSplit[10]),    /* buildTime.TotalMilliseconds */
                            out packageVersionId);

                        int hashCode = strBuildEvent.GetHashCode();

                        if (!mPackageVersionIdDictionary.ContainsKey(hashCode))
                            mPackageVersionIdDictionary.Add(strBuildEvent.GetHashCode(), packageVersionId);
                    }
                    else if (strBuildEvent.StartsWith("START TARGET"))
                    {
                        Regex re = new Regex(@"START TARGET&targetName=([^&]*)&parentTargetId=([^&]*)&configId=([^&]*)&BuildGroup=([^&]*)");
                        string[] strSplit = re.Split(strBuildEvent);

                        int? parentTargetId = null;
                        if (strSplit[2] != string.Empty)
                            parentTargetId = mTargetIdDictionary[int.Parse(strSplit[2])];

                        int targetId = dataStore.GetTargetID(
                            strSplit[1],            /* targetData.TargetName */
                            parentTargetId,         /* parentTargetId */
                            mConfigIdDictionary[int.Parse(strSplit[3])], /* configId */
                            strSplit[4]             /* targetData.BuildGroup */);

                        int hashCode = strBuildEvent.GetHashCode();

                        if (!mTargetIdDictionary.ContainsKey(hashCode))
                            mTargetIdDictionary.Add(hashCode, targetId);
                    }
                    else if (strBuildEvent.StartsWith("UPDATETARGETINFO"))
                    {
                        Regex re = new Regex(@"UPDATETARGETINFO&targetID=([^&]*)&Result=([^&]*)&BuildTimeMillisec=(.*)");
                        string[] strSplit = re.Split(strBuildEvent);

                        dataStore.UpdateTargetInfo(mTargetIdDictionary[int.Parse(strSplit[1])],
                            int.Parse(strSplit[2]), int.Parse(strSplit[3]));
                    }
                    else
                    {
                        Console.WriteLine("Data Received from Framework Build: {0}",
                            strBuildEvent);
                    }
                }
            }
            catch (Exception)
            {
                // if an unhandled exception occurs, just quit without
                // displaying an error message.
                Process.GetCurrentProcess().Kill();
            }
        }

        static void CurrentDomain_UnhandledException(object sender, System.UnhandledExceptionEventArgs e)
        {
            // if an unhandled exception occurs, just quit without
            // displaying an error message.
            Process.GetCurrentProcess().Kill();
        }
    }

    // Defines the data protocol for reading and writing strings on our stream
    public class StreamString
    {
        private Stream ioStream;

        public StreamString(Stream ioStream)
        {
            this.ioStream = ioStream;
        }

        public string ReadString()
        {
            int len = 0;

            len = ioStream.ReadByte();
            if (len != -1)
            {
                len *= 256;
                len += ioStream.ReadByte();
                if (len > 0)
                {
                    byte[] inBuffer = new byte[len];
                    ioStream.Read(inBuffer, 0, len);
                    return UnicodeEncoding.UTF8.GetString(inBuffer);
                }
            }

            return string.Empty;
        }
    }
}
