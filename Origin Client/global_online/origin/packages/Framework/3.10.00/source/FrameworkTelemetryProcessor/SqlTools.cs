using System;
using System.Collections.Generic;
using System.Web;
using System.Text;
using System.Security.Cryptography;

using System.Data;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Runtime.Serialization;

namespace EATech.Common.SQL
{
    [DebuggerStepThrough]
    public class FrameworkSqlException : Exception
    {
        public FrameworkSqlException(String inMessage) : base(inMessage) { }
        public FrameworkSqlException(SerializationInfo inInfo, StreamingContext inContext) : base(inInfo, inContext) { }
        public FrameworkSqlException(String inMessage, Exception inException) : base(inMessage, inException) { }
    }

    public sealed class SqlTools
    {
        public static SqlParameter AddSqlReturnValueParameter(SqlCommand inSqlCommand)
        {
            if (inSqlCommand.CommandType != CommandType.StoredProcedure)
                throw new FrameworkSqlException(String.Format("SqlCommand {0} mismatch! Must be stored procedure.", inSqlCommand.CommandText));

            SqlParameter sqlParameter = new SqlParameter("@RETURN_VALUE", SqlDbType.Int);
            sqlParameter.Direction = ParameterDirection.ReturnValue;
            inSqlCommand.Parameters.Add(sqlParameter);

            return sqlParameter;
        }

        #region " Validate SQL Connection Strings "
        public static bool IsValidSqlConnectionString(SqlConnectionStringBuilder inSqlConnectionString)
        {
            return _IsValidSqlConnectionString(inSqlConnectionString.ToString());
        }
        private static bool _IsValidSqlConnectionString(string inSqlConnectionString)
        {
            bool bValidSqlConnectionString = false;

            try
            {
                using (SqlConnection sqlConnection = new SqlConnection(inSqlConnectionString.ToString()))
                {
                    sqlConnection.InfoMessage += new SqlInfoMessageEventHandler(OnInfoMessage);

                    // Method #1
                    /*using (SqlCommand sqlCommand = new SqlCommand("SELECT * FROM dbo.udfThisFunctionIsFake()", sqlConnection))
                    {
                        sqlCommand.Connection.Open();
                        sqlCommand.CommandType = CommandType.Text;
                        sqlCommand.ExecuteNonQuery();
                    }*/
                    // Method #2
                    using (SqlCommand sqlCommand = new SqlCommand("dbo.spThisFunctionIsFake", sqlConnection))
                    {
                        sqlCommand.Connection.Open();
                        sqlCommand.CommandType = CommandType.StoredProcedure;
                        sqlCommand.ExecuteNonQuery();
                    }
                }

                bValidSqlConnectionString = true;
            }
            catch (SqlException exception)
            {
                // exception Numbers 208 & 2812 mean the SqlConnectionString was valid, but the dummy T-SQL failed
                bValidSqlConnectionString = ((exception.Number == 208) || (exception.Number == 2812));
            }
            catch
            {
                bValidSqlConnectionString = false;
            }

            return bValidSqlConnectionString;
        }
        #endregion

        internal static void OnInfoMessage(object sender, SqlInfoMessageEventArgs args)
        {
            foreach (SqlError err in args.Errors)
                EATech.Common.Tools.WriteToEventLog(
                        "SqlTools.OnInfoMessage",
                        String.Format(
                            "The {0} has received a severity {1}, state {2} error number {3}\non line {4} of procedure {5} on server {6}:\n{7}",
                            err.Source, err.Class, err.State, err.Number, err.LineNumber, err.Procedure, err.Server, err.Message),
                        EventLogEntryType.Warning
                    );
        }

        internal static string ComputeHash(string text)
        {
            if (text == null)
                text = string.Empty;

            byte[] bytes = Encoding.UTF8.GetBytes(text);
            if (bytes.Length > 20)
            {
                SHA1 sha = new SHA1CryptoServiceProvider();
                bytes = sha.ComputeHash(bytes);
            }
            string hash = String.Format("{0:0000}_{1}", text.Length, Convert.ToBase64String(bytes));
            return hash;
        }

    }
}

