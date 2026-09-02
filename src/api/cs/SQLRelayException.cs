// Copyright (c) David Muse
// See the file COPYING for more information

using System;
using System.Runtime.Serialization;

namespace SQLRClient
{
    [Serializable]
    public sealed class SQLRelayException : SystemException
    {
        #region member variables
        Int64 number = 0;
        String sqlState = "";
        #endregion

        #region constructors and destructors
        internal SQLRelayException(Int64 number, String message, String sqlState)
            : base(message)
        {
            this.number = number;
            this.sqlState = sqlState;
        }

        private SQLRelayException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
            number = info.GetInt64("number");
            sqlState = info.GetString("sqlState");
        }
        #endregion

        #region properties
        public Int64 Number
        {
            get
            {
                return number;
            }
        }

        // matches System.Data.Odbc.OdbcException.SQLState; empty when the
        // backend supplied no SQLSTATE
        public String SQLState
        {
            get
            {
                return sqlState;
            }
        }
        #endregion

        #region public methods
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue("number", number);
            info.AddValue("sqlState", sqlState);
            base.GetObjectData(info, context);
        }
        #endregion
    }
}
