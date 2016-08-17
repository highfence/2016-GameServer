using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MobileServer_withDB
{
	public static class Main
	{
		public static bool EnableRequestHealthCheck { get; private set; }

		public static ERROR_CODE Init()
		{
			var error = ERROR_CODE.NONE;

			error = InitDB();
			if(error != ERROR_CODE.NONE)
			{
				Console.WriteLine(string.Format("Starting. Fail DB:{0}", error));
				return error;
			}
			EnableRequestHealthCheck = true;
			return ERROR_CODE.NONE;
		}

		static ERROR_CODE InitDB()
		{
			var redisList = "localhost:6379";
			var error = DB.Redis.Init(redisList);
			if (error != ERROR_CODE.NONE)
				return error;
			return error;
		}
	}
}
