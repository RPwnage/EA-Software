// Originally based on NAnt - A .NET build tool
// Copyright (C) 2003-2018 Electronic Arts Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// 
// As a special exception, the copyright holders of this software give you 
// permission to link the assemblies with independent modules to produce 
// new assemblies, regardless of the license terms of these independent 
// modules, and to copy and distribute the resulting assemblies under terms 
// of your choice, provided that you also meet, for each linked independent 
// module, the terms and conditions of the license of that module. An 
// independent module is a module which is not derived from or based 
// on these assemblies. If you modify this software, you may extend 
// this exception to your version of the software, but you are not 
// obligated to do so. If you do not wish to do so, delete this exception 
// statement from your version. 
// 
// Electronic Arts (Frostbite.Team.CM@ea.com)

using System;
using System.Collections.Generic;
using System.Linq;

using NAnt.Perforce.Internal;


namespace NAnt.Perforce
{
    public class P4User
    {
        public enum UserType
        {
            standardtype,
            servicetype,
            operatortype
        }

        private P4Record _UsersRecord = null;

        public string User 
        { 
            get { return _UsersRecord.GetField("user"); }
            private set { _UsersRecord.SetField("user", value); }
        }

        public DateTime Access { get { return _UsersRecord.GetDateField("access"); } }
        public DateTime Update { get { return _UsersRecord.GetDateField("update"); } }
        public P4Server Server { get; private set; }

        public string Email 
        { 
            get { return _UsersRecord.GetField("email"); }
            set { _UsersRecord.SetField("email", value); }
        }

        public string Fullname 
        { 
            get { return _UsersRecord.GetField("fullname"); }
            set { _UsersRecord.SetField("fullname", value); }
        }

        public UserType Type 
        { 
            get { return (UserType)Enum.Parse(typeof(UserType), _UsersRecord.GetField("type") + "type"); } 
            set { _UsersRecord.SetField("type", value.ToString().Substring(0, value.ToString().Length - 4)); } 
        }

        internal P4User(P4Server server, P4Record record)
        {
            Server = server;
            _UsersRecord = record;
        }

        internal P4User(P4Server server, string userName, P4User.UserType type, string email, string fullName)
        {
            Server = server;
            _UsersRecord = new P4Record();

            User = userName;
            Type = type;
            Email = email;
            Fullname = fullName;
        }

        public IEnumerable<string> GetReviews()
        {
            if (!_UsersRecord.HasArrayField("reviews"))
            {
                return Enumerable.Empty<string>();
            }
            return _UsersRecord.GetArrayField("reviews");
        }

        public void SetReviews(params string[] reviews)
        {
            _UsersRecord.SetArrayField("reviews", reviews);
        }

        public bool UpdateReviews(params string[] reviews)
        {
            IEnumerable<string> originalReviews = GetReviews();
            IEnumerable<string> newReviews = originalReviews.Concat(reviews.Except(originalReviews).ToArray());
            if (originalReviews.Count() < newReviews.Count())
            {
                SetReviews( newReviews.ToArray());
                return true;
            }
            return false;
        }

        public bool RemoveReviews(params string[] reviews)
        {
            IEnumerable<string> originalReviews = GetReviews();
            IEnumerable<string> newReviews = originalReviews.Except(reviews);
            if (originalReviews.Count() > newReviews.Count())
            {
                SetReviews(newReviews.ToArray());
                return true;
            }
            return false;
        }

        public void Save()
        {
            P4Caller.Run(Server, "user", new string[] { "-i" }, input: _UsersRecord.AsForm(), timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs);
            _UsersRecord = P4Caller.Run(Server, "user", new string[] { "-o", User }, timeoutMillisec: P4GlobalOptions.DefaultTimeoutMs).Records.First(); // read back client to get modified dates, etc
        }
    }
}