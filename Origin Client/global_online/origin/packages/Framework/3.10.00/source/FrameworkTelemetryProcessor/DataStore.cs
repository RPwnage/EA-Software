//#define DEBUG_VERBOSE
using System;
using System.Collections.Generic;
using System.Text;
using System.Data;
using System.Data.SqlClient;
using System.Configuration;
using System.Diagnostics;
using EATech.Common;
using EATech.Common.SQL;

namespace EA.MetricsProcessor
{
    internal class DataStore
    {
        private readonly string CONNECTION_STRING;
        private const int TIMEOUT_SEC = 2;

        internal DataStore()
        {
            SqlConnectionStringBuilder sqlConnectionString = new SqlConnectionStringBuilder("Server=sp7sql2.eatechnet.ea.com,2884;Database=EATechFrameworkPackageTracking;User=FrameworkAppWriter;Password=FrameworkAppWriter;Application Name=Framework;Connection Timeout=2");
            // SqlConnectionStringBuilder sqlConnectionString = new SqlConnectionStringBuilder(ConfigurationManager.ConnectionStrings["Framework"].ConnectionString.ToString());

            if (!EATech.Common.SQL.SqlTools.IsValidSqlConnectionString(sqlConnectionString))
            {
                CONNECTION_STRING = null;
                throw new FrameworkSqlException(String.Format("Invalid SQL connection string; {0}", sqlConnectionString));
            }

            CONNECTION_STRING = sqlConnectionString.ConnectionString;
        }

        internal int BuildID
        {
            get { return buildId; }
        }

        internal int GetNTLoginDetailsLevel(string inLogin)
        {
            int outDetailsLevel = 0;

            String storedProcedureName = "dbo.uspGet_NTLoginDetailsLevel";
            int timeout = GetTimeout();

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inNTLogin", SqlDbType.VarChar, 101).Value = inLogin;

                    SqlParameter sqlDetailsLevelParameter = sqlCommand.Parameters.Add("@outDetailsLevel", SqlDbType.Int);
                    sqlDetailsLevelParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] GetNTLoginDetailsLevel()");
                    sqlCommand.ExecuteNonQuery();

                    if (!Convert.IsDBNull(sqlReturnParameter.Value) && Convert.ToInt32(sqlReturnParameter.Value) == 0 && !Convert.IsDBNull(sqlDetailsLevelParameter.Value))
                    {
                        outDetailsLevel = Convert.ToInt32(sqlDetailsLevelParameter.Value);
                    }

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            return outDetailsLevel;
        }

        internal int GetPackageAndVersionIDs(string inPackageName, string inPackageVersion, out int outPackageID )
        {
            int outPackageVersionID = -1;

            String storedProcedureName = "dbo.uspInsGet_PackageAndVersionIDs";
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout; 
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inPackageName", SqlDbType.VarChar, 100).Value = inPackageName;
                    sqlCommand.Parameters.Add("@inPackageVersion", SqlDbType.VarChar, 50).Value = inPackageVersion;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlPackageVersionIdParameter = sqlCommand.Parameters.Add("@outPackageVersionID", SqlDbType.Int);
                    sqlPackageVersionIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] GetPackageAndVersionIDs()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for PackageID", storedProcedureName));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for PackageID", storedProcedureName));

                    outPackageID = Convert.ToInt32(sqlPackageIdParameter.Value);
                    outPackageVersionID = Convert.ToInt32(sqlPackageVersionIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif


            return outPackageVersionID;
        }

        internal int GetConfigID(string inConfigName, string inConfigSystem, string inConfigCompiler)
        {
            int outConfigID = -1;

            String storedProcedureName = "dbo.uspInsGet_ConfigurationID";
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout; 
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inConfigName", SqlDbType.VarChar, 50).Value = inConfigName;
                    sqlCommand.Parameters.Add("@inConfigSystem", SqlDbType.VarChar, 10).Value = inConfigSystem;
                    sqlCommand.Parameters.Add("@inConfigCompiler", SqlDbType.VarChar, 10).Value = inConfigCompiler;

                    SqlParameter sqlConfigIdParameter = sqlCommand.Parameters.Add("@outConfigID", SqlDbType.Int);
                    sqlConfigIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] GetConfigID()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlConfigIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for ConfigID", storedProcedureName));


                    outConfigID = Convert.ToInt32(sqlConfigIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

            return outConfigID;
        }

        internal void BuildStarted(string inNantVersion, string inNTLogIn, int inMachineID, bool inIsServiceAccount, string inBuildFile, string inBuildCmdLine)
        {
            string storedProcedureName = "dbo.uspInsUpdGet_BuildID_BuildStarted";
            int timeout = GetTimeout();

            // @inNantVersion       VARCHAR(10),
            // @inNTLogIn           VARCHAR(101),
            // @inMachineID         INT,
            // @inIsServiceAccount  BIT,
            // @inBuildFile         VARCHAR(260),
            // @inBuildCmdLine      VARCHAR(1280),
            // @inBuildCmdLineHash      VARCHAR(34),
            // @outBuildID          INT = NULL OUTPUT	

            string inBuildCmdLineHash = SqlTools.ComputeHash(inBuildCmdLine);

            if (inBuildFile.Length > 259)
                inBuildFile = "~"+ inBuildFile.Substring(inBuildFile.Length-258);

            if (inBuildCmdLine.Length > 1279)
                inBuildCmdLine = inBuildCmdLine.Substring(0, 1278) + "~";

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout; 
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inNantVersion", SqlDbType.VarChar, 10).Value = inNantVersion;
                    sqlCommand.Parameters.Add("@inNTLogIn", SqlDbType.VarChar, 101).Value = inNTLogIn;
                    sqlCommand.Parameters.Add("@inMachineID", SqlDbType.Int).Value = inMachineID;
                    sqlCommand.Parameters.Add("@inIsServiceAccount", SqlDbType.Bit).Value = inIsServiceAccount;
                    sqlCommand.Parameters.Add("@inBuildFile", SqlDbType.VarChar, 260).Value = inBuildFile;
                    sqlCommand.Parameters.Add("@inBuildCmdLine", SqlDbType.VarChar, 1280).Value = inBuildCmdLine;
                    sqlCommand.Parameters.Add("@inBuildCmdLineHash", SqlDbType.VarChar, 34).Value = inBuildCmdLineHash;

                    SqlParameter sqlBuildIdParameter = sqlCommand.Parameters.Add("@outBuildID", SqlDbType.Int);
                    sqlBuildIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] BuildStarted()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlBuildIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildID", storedProcedureName));

                    buildId = Convert.ToInt32(sqlBuildIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))
        }

        internal void BuildFinished(int inBuildTimeMillisec, int inBuildResult)
        {
            string storedProcedureName = "dbo.uspUpd_BuildFinished";
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            // @inBuildID           INT,
            // @inBuildTimeMillisec INT,
            // @inBuildResult       TINYINT

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inBuildTimeMillisec", SqlDbType.Int).Value = inBuildTimeMillisec;
                    sqlCommand.Parameters.Add("@inBuildResult", SqlDbType.TinyInt).Value = inBuildResult;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] BuildFinished()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif
        }

        internal int GetTargetID(string inTargetName, int? parentTargetID, int inConfigID, string inBuildGroup)
        {
            string storedProcedureName = "dbo.uspInsGet_TargetID";
            int targetID = -1;
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            // @inBuildID           INT,
            // @inTargetName        VARCHAR(50),
            // @inParentTargetID    INT,
            // @inConfigName        VARCHAR(50),
            // @inConfigSystem      VARCHAR(10),
            // @inConfigCompiler    VARCHAR(10),
            // @inBuildGroupID      TINYINT,
            // @outTargetID         INT = NULL OUTPUT	

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                if (inTargetName.Length > 49)
                    inTargetName = inTargetName.Substring(0, 48) + "~";

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inTargetName", SqlDbType.VarChar, 50).Value = inTargetName;
                    SqlParameter sqlParentTargetIdParameter = sqlCommand.Parameters.Add("@inParentTargetID", SqlDbType.Int);
                    sqlParentTargetIdParameter.IsNullable = true;
                    sqlParentTargetIdParameter.Value = parentTargetID;
                    sqlCommand.Parameters.Add("@inConfigId", SqlDbType.Int).Value = inConfigID;
                    sqlCommand.Parameters.Add("@inBuildGroupID", SqlDbType.TinyInt).Value = GetBuildGroupID(inBuildGroup);
                    SqlParameter sqlTargetIdParameter = sqlCommand.Parameters.Add("@outTargetID", SqlDbType.Int);
                    sqlTargetIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] GetTargetID()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlTargetIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for TargetID", storedProcedureName));

                    targetID = Convert.ToInt32(sqlTargetIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif


            return targetID;
        }

        internal void UpdateTargetInfo(int targetID, int inResult, int inBuildTimeMillisec)
        {
            string storedProcedureName = "dbo.uspUpd_BuildTarget";
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            // @inTargetID           INT,
            // @inResult            TINYINT,
            // @inBuildTimeMillisec INT,
            // @outTargetID         INT = NULL OUTPUT	

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inTargetID", SqlDbType.Int).Value = targetID;
                    sqlCommand.Parameters.Add("@inResult", SqlDbType.TinyInt).Value = inResult;
                    sqlCommand.Parameters.Add("@inBuildTimeMillisec", SqlDbType.Int).Value = inBuildTimeMillisec;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] UpdateTargetInfo()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

        }

        internal int InsertUpdatePackageInfoUseDepend(int inPackageVersionID, int inFrameworkVersion)
        {
            int packageID = -1;
            string storedProcedureName = "uspInsUpd_BuildPackageVersions_UseDepend";
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inPackageVersionID", SqlDbType.Int).Value = inPackageVersionID;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfoUseDepend()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);


                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

            return packageID;
        }

        internal int InsertUpdatePackageInfoBuildDepend(int inPackageVersionID, int inConfigID, string inBuildGroup, int? buildTargetID,
                    bool inIsAutobuild, int inFrameworkVersion, int inResult, int inBuildTimeMillisec)
        {
            string storedProcedureName = "uspInsUpd_BuildPackageVersions_BuildDepend";
            int packageID = -1;
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            // @inBuildID           INT,
            // @inPackageVersionID  INT,
            // @inConfigId          INT,
            // @inBuildGroupID      TINYINT,
            // @inBuildTargetId     INT=NULL, - removed
            // @inIsBuildDependency BIT,
            // @inIsAutobuild       BIT,
            // @inFrameworkVersion  TINYINT,
            // @inResult            TINYINT,
            // @inBuildTimeMillisec INT,	
            // @outBuildPackageID   INT = NULL OUTPUT	

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;

                    sqlCommand.Parameters.Add("@inPackageVersionID", SqlDbType.Int).Value = inPackageVersionID;
                    sqlCommand.Parameters.Add("@inConfigID", SqlDbType.Int).Value = inConfigID;
                    sqlCommand.Parameters.Add("@inBuildGroupID", SqlDbType.TinyInt).Value = GetBuildGroupID(inBuildGroup);
                    //SqlParameter sqlTargetIdParameter = sqlCommand.Parameters.Add("@inBuildTargetId", SqlDbType.Int);
                    //sqlTargetIdParameter.IsNullable = true;
                    //sqlTargetIdParameter.Value = buildTargetID;
                    sqlCommand.Parameters.Add("@inIsAutobuild", SqlDbType.Bit).Value = inIsAutobuild;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;
                    sqlCommand.Parameters.Add("@inResult", SqlDbType.TinyInt).Value = inResult;
                    sqlCommand.Parameters.Add("@inBuildTimeMillisec", SqlDbType.Int).Value = inBuildTimeMillisec;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfoBuildDepend()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

            return packageID;
        }

        internal int InsertUpdatePackageInfo(int inPackageVersionID, int inConfigID,
                        string inBuildGroup, int? buildTargetID, bool inIsBuildDependency,
                        bool inIsAutobuild, int inFrameworkVersion, int inResult, int inBuildTimeMillisec)
        {
            string storedProcedureName = "uspInsUpd_BuildPackageDetailedMetrics";
            int packageID = -1;
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif
            
            // @inBuildID           INT,
            // @inPackageVersionID  INT,
            // @inConfigID          INT,
            // @inBuildGroupID      TINYINT,
            // @inBuildTargetId     INT=NULL,
            // @inIsBuildDependency BIT,
            // @inIsAutobuild       BIT,
            // @inFrameworkVersion  TINYINT,
            // @inResult            TINYINT,
            // @inBuildTimeMillisec INT,	
            // @outBuildPackageID   INT = NULL OUTPUT	

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inPackageVersionID", SqlDbType.Int).Value = inPackageVersionID;
                    sqlCommand.Parameters.Add("@inConfigID", SqlDbType.Int).Value = inConfigID;
                    sqlCommand.Parameters.Add("@inBuildGroupID", SqlDbType.TinyInt).Value = GetBuildGroupID(inBuildGroup);
                    SqlParameter sqlTargetIdParameter = sqlCommand.Parameters.Add("@inBuildTargetId", SqlDbType.Int);
                    sqlTargetIdParameter.IsNullable = true;
                    sqlTargetIdParameter.Value = buildTargetID;
                    sqlCommand.Parameters.Add("@inIsBuildDependency", SqlDbType.Bit).Value = inIsBuildDependency;
                    sqlCommand.Parameters.Add("@inIsAutobuild", SqlDbType.Bit).Value = inIsAutobuild;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;
                    sqlCommand.Parameters.Add("@inResult", SqlDbType.TinyInt).Value = inResult;
                    sqlCommand.Parameters.Add("@inBuildTimeMillisec", SqlDbType.Int).Value = inBuildTimeMillisec;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfo()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
               Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

            return packageID;
        }


        internal int InsertUpdatePackageInfoUseDepend_GetPackageVersionID(string inPackageName, string inPackageVersion, int inFrameworkVersion, out int outPackageVersionID)
        {
            int packageID = -1;
            string storedProcedureName = "uspInsUpd_BuildPackageVersions_UseDepend_GetPackageVersionID";
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inPackageName", SqlDbType.VarChar, 100).Value = inPackageName;
                    sqlCommand.Parameters.Add("@inPackageVersion", SqlDbType.VarChar, 50).Value = inPackageVersion;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlPackageVersionIdParameter = sqlCommand.Parameters.Add("@outPackageVersionID", SqlDbType.Int);
                    sqlPackageVersionIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfoUseDepend_GetPackageVersionID()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);

                    if (Convert.IsDBNull(sqlPackageVersionIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for PackageVersionID", storedProcedureName));

                    outPackageVersionID = Convert.ToInt32(sqlPackageVersionIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

            return packageID;
        }

        internal int InsertUpdatePackageInfoBuildDepend_GetPackageVersionID(string inPackageName, string inPackageVersion, int inConfigID, string inBuildGroup, int? buildTargetID,
                    bool inIsAutobuild, int inFrameworkVersion, int inResult, int inBuildTimeMillisec, out int outPackageVersionID)
        {
            string storedProcedureName = "uspInsUpd_BuildPackageVersions_BuildDepend_GetPackageVersionID";
            int packageID = -1;
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            // @inBuildID           INT,
            // @inPackageVersionID  INT,
            // @inConfigId          INT,
            // @inBuildGroupID      TINYINT,
            // @inBuildTargetId     INT=NULL, - removed
            // @inIsBuildDependency BIT,
            // @inIsAutobuild       BIT,
            // @inFrameworkVersion  TINYINT,
            // @inResult            TINYINT,
            // @inBuildTimeMillisec INT,	
            // @outBuildPackageID   INT = NULL OUTPUT	

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inPackageName", SqlDbType.VarChar, 100).Value = inPackageName;
                    sqlCommand.Parameters.Add("@inPackageVersion", SqlDbType.VarChar, 50).Value = inPackageVersion;
                    sqlCommand.Parameters.Add("@inConfigID", SqlDbType.Int).Value = inConfigID;
                    sqlCommand.Parameters.Add("@inBuildGroupID", SqlDbType.TinyInt).Value = GetBuildGroupID(inBuildGroup);
                    //SqlParameter sqlTargetIdParameter = sqlCommand.Parameters.Add("@inBuildTargetId", SqlDbType.Int);
                    //sqlTargetIdParameter.IsNullable = true;
                    //sqlTargetIdParameter.Value = buildTargetID;
                    sqlCommand.Parameters.Add("@inIsAutobuild", SqlDbType.Bit).Value = inIsAutobuild;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;
                    sqlCommand.Parameters.Add("@inResult", SqlDbType.TinyInt).Value = inResult;
                    sqlCommand.Parameters.Add("@inBuildTimeMillisec", SqlDbType.Int).Value = inBuildTimeMillisec;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlPackageVersionIdParameter = sqlCommand.Parameters.Add("@outPackageVersionID", SqlDbType.Int);
                    sqlPackageVersionIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfoBuildDepend_GetPackageVersionID()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);

                    if (Convert.IsDBNull(sqlPackageVersionIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for PackageVersionID", storedProcedureName));

                    outPackageVersionID = Convert.ToInt32(sqlPackageVersionIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

            return packageID;
        }

        internal int InsertUpdatePackageInfo_GetPackageVersionID(string inPackageName, string inPackageVersion, int inConfigID,
                        string inBuildGroup, int? buildTargetID, bool inIsBuildDependency,
                        bool inIsAutobuild, int inFrameworkVersion, int inResult, int inBuildTimeMillisec, out int outPackageVersionID)
        {
            string storedProcedureName = "uspInsUpd_BuildPackageDetailedMetrics_GetPackageVersionID";
            int packageID = -1;
            int timeout = GetTimeout();

            #if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
            #endif

            // @inBuildID           INT,
            // @inPackageVersionID  INT,
            // @inConfigID          INT,
            // @inBuildGroupID      TINYINT,
            // @inBuildTargetId     INT=NULL,
            // @inIsBuildDependency BIT,
            // @inIsAutobuild       BIT,
            // @inFrameworkVersion  TINYINT,
            // @inResult            TINYINT,
            // @inBuildTimeMillisec INT,	
            // @outBuildPackageID   INT = NULL OUTPUT	

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
                #if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
                #endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inPackageName", SqlDbType.VarChar, 100).Value = inPackageName;
                    sqlCommand.Parameters.Add("@inPackageVersion", SqlDbType.VarChar, 50).Value = inPackageVersion;
                    sqlCommand.Parameters.Add("@inConfigID", SqlDbType.Int).Value = inConfigID;
                    sqlCommand.Parameters.Add("@inBuildGroupID", SqlDbType.TinyInt).Value = GetBuildGroupID(inBuildGroup);
                    SqlParameter sqlTargetIdParameter = sqlCommand.Parameters.Add("@inBuildTargetId", SqlDbType.Int);
                    sqlTargetIdParameter.IsNullable = true;
                    sqlTargetIdParameter.Value = buildTargetID;
                    sqlCommand.Parameters.Add("@inIsBuildDependency", SqlDbType.Bit).Value = inIsBuildDependency;
                    sqlCommand.Parameters.Add("@inIsAutobuild", SqlDbType.Bit).Value = inIsAutobuild;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;
                    sqlCommand.Parameters.Add("@inResult", SqlDbType.TinyInt).Value = inResult;
                    sqlCommand.Parameters.Add("@inBuildTimeMillisec", SqlDbType.Int).Value = inBuildTimeMillisec;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlPackageVersionIdParameter = sqlCommand.Parameters.Add("@outPackageVersionID", SqlDbType.Int);
                    sqlPackageVersionIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfo_GetPackageVersionID()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);

                    if (Convert.IsDBNull(sqlPackageVersionIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for PackageVersionID", storedProcedureName));

                    outPackageVersionID = Convert.ToInt32(sqlPackageVersionIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

            #if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
            #endif

            return packageID;
        }

        internal int InsertUpdatePackageInfoBase(int inPackageVersionID, int inFrameworkVersion)
        {
            int packageID = -1;
            string storedProcedureName = "uspInsUpd_BuildPackageVersionsBase";
            int timeout = GetTimeout();

#if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
#endif

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
#if DEBUG
                sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
#endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inPackageVersionID", SqlDbType.Int).Value = inPackageVersionID;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfoBase()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);


                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

#if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
#endif

            return packageID;
        }

        internal int InsertUpdatePackageInfoBase_GetPackageVersionID(string inPackageName, string inPackageVersion, int inFrameworkVersion, out int outPackageVersionID)
        {
            int packageID = -1;
            string storedProcedureName = "uspInsUpd_BuildPackageVersionsBase_GetPackageVersionID";
            int timeout = GetTimeout();

#if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
#endif

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
#if DEBUG
                sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
#endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inBuildID", SqlDbType.Int).Value = BuildID;
                    sqlCommand.Parameters.Add("@inPackageName", SqlDbType.VarChar, 100).Value = inPackageName;
                    sqlCommand.Parameters.Add("@inPackageVersion", SqlDbType.VarChar, 50).Value = inPackageVersion;
                    sqlCommand.Parameters.Add("@inFrameworkVersion", SqlDbType.TinyInt).Value = inFrameworkVersion;

                    SqlParameter sqlPackageIdParameter = sqlCommand.Parameters.Add("@outBuildPackageID", SqlDbType.Int);
                    sqlPackageIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlPackageVersionIdParameter = sqlCommand.Parameters.Add("@outPackageVersionID", SqlDbType.Int);
                    sqlPackageVersionIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdatePackageInfoBase_GetPackageVersionID()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlPackageIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    packageID = Convert.ToInt32(sqlPackageIdParameter.Value);

                    if (Convert.IsDBNull(sqlPackageVersionIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for PackageVersionID", storedProcedureName));

                    outPackageVersionID = Convert.ToInt32(sqlPackageVersionIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

#if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
#endif

            return packageID;
        }


        internal int InsertUpdateMachineInfo(string inMachineName, int inOSType, string inOSVersion, int inProcessorCount, int inMemoryMB)
        {
            string storedProcedureName = "uspInsGet_MachineID";
            int machineID = -1;
            int timeout = GetTimeout();

#if DEBUG_VERBOSE
                DateTime start = DateTime.Now;
#endif

            // @inMachineName VARCHAR(101),
            // @inOSType	   INT,
            // @inOSVersion   VARCHAR(101),
            // @inProcessorCount  SMALLINT,
            // @inMemoryMB  INT,
            // @outMachineID INT = NULL OUTPUT

            if (inMachineName.Length > 100)
                inMachineName = inMachineName.Substring(0, 99) + "~";

            if (inOSVersion.Length > 100)
                inOSVersion = inOSVersion.Substring(0, 99) + "~";

            using (SqlConnection sqlConnection = new SqlConnection(CONNECTION_STRING))
            {
#if DEBUG
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnSqlInfoMessage);
#endif

                using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))
                {
                    sqlCommand.CommandTimeout = timeout;
                    sqlCommand.CommandType = CommandType.StoredProcedure;

                    sqlCommand.Parameters.Add("@inMachineName", SqlDbType.VarChar, 101).Value = inMachineName;
                    sqlCommand.Parameters.Add("@inOSType", SqlDbType.Int).Value = inOSType;
                    sqlCommand.Parameters.Add("@inOSVersion", SqlDbType.VarChar, 101).Value = inOSVersion;
                    sqlCommand.Parameters.Add("@inProcessorCount", SqlDbType.SmallInt).Value = inProcessorCount;
                    sqlCommand.Parameters.Add("@inMemoryMB", SqlDbType.Int).Value = inMemoryMB;

                    SqlParameter sqlMachineIdParameter = sqlCommand.Parameters.Add("@outMachineID", SqlDbType.Int);
                    sqlMachineIdParameter.Direction = ParameterDirection.Output;

                    SqlParameter sqlReturnParameter = SqlTools.AddSqlReturnValueParameter(sqlCommand);

                    sqlConnection.Open();

                    // NAnt.Core.Logging.Log.WriteLine("        [DB] InsertUpdateMachineInfo()");
                    sqlCommand.ExecuteNonQuery();

                    if (Convert.IsDBNull(sqlReturnParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL", storedProcedureName));

                    if (Convert.ToInt32(sqlReturnParameter.Value) != 0)
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned {1}", storedProcedureName, Convert.ToInt32(sqlReturnParameter.Value)));

                    if (Convert.IsDBNull(sqlMachineIdParameter.Value))
                        throw new FrameworkSqlException(String.Format("Stored procedure {0} returned NULL for BuildPackageID", storedProcedureName));

                    machineID = Convert.ToInt32(sqlMachineIdParameter.Value);

                } // using (SqlCommand sqlCommand = new SqlCommand(storedProcedureName, sqlConnection))

            } // using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ConnectionString))

#if DEBUG_VERBOSE
                Console.WriteLine("    Stored Proc {0} time = {1}", storedProcedureName, DateTime.Now - start);
#endif

            return machineID;
        }


        private static int GetBuildGroupID(string buildGroup)
        {
            if (string.IsNullOrEmpty(buildGroup))
            {
                return 2;
            }
            else if (buildGroup == "runtime")
            {
                return 1;
            }
            else if (buildGroup == "test")
            {
                return 3;
            }
            else if (buildGroup == "example")
            {
                return 4;
            }
            else if (buildGroup == "tool")
            {
                return 5;
            }
            return 2; // Undefined
        }

        internal void Cleanup()
        {
            #if DEBUG
                Console.WriteLine(" -- ClearAllPools");
            #endif
            SqlConnection.ClearAllPools();
        }

        private static void OnSqlInfoMessage(object sender, SqlInfoMessageEventArgs args)
        {
            EATech.Common.Tools.WriteToEventLog("EATech.FrameworkMetrics.DAL.Packages", String.Format("SqlError: {0}", args.Errors.ToString()), EventLogEntryType.Error);
        }

        private int GetTimeout()
        {
            return 20;
            /*
            int timeout = TIMEOUT_SEC;
            lock (this)
            {

                if (stoppingTimeout > 0)
                {
                    TimeSpan stopSpan = (DateTime.Now - stoppingTime);
                    int stopIn = ((int)stopSpan.Milliseconds - stoppingTimeout);
                    if (stopIn > -60)
                    {
                        #if DEBUG_VERBOSE
                        Console.WriteLine(" -- STopping = {0}", stoppingTime);
                        Console.WriteLine(" -- Now      = {0}", DateTime.Now);
                        Console.WriteLine(" -- stopIn   = {0}", stopIn);
                        #endif
                        // Don't bother to do anything. Quit.
                        throw new Exception("Timed out");
                    }
                    timeout = Math.Min(TIMEOUT_SEC, (-stopIn)/1000);
                    timeout = Math.Max(timeout, 1);
                    #if DEBUG_VERBOSE
                    Console.WriteLine(" -- GetTimeout - Stopping in = {0}, timeout={1}", stopIn, timeout);
                    #endif
                }
            }
            return timeout;
             * */
        }

        internal void SetStopping(int timeout, DateTime stoppingtime)
        {
            lock (this)
            {
                stoppingTime = stoppingtime;
                stoppingTimeout = timeout;
                
            }
        }

        private int buildId = -1;

        private int stoppingTimeout = -1;
        private DateTime stoppingTime;


    }

}
