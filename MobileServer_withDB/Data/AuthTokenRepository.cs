using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MobileServer_withDB.Data
{
	public static class AuthTokenRepository
	{
		public static async Task Add(string userID, string authToken)
		{
			await DB.Redis.SetStringAsync<DBUserSession>(userID, new DBUserSession() { AuthToken = authToken, CV = 1, CDV = 1 });
		}

		public static async Task<bool> Check(string userID, string authToken)
		{
			var sessionInfo = await DB.Redis.GetStringAsync<DBUserSession>(userID);
			if (sessionInfo.Item1 == false || sessionInfo.Item2.AuthToken != authToken)
				return false;
			return true;
		}
	}
	public struct DBUserSession
	{
		public string AuthToken;
		public short CV;
		public short CDV;
	}
}
