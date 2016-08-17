using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Runtime.CompilerServices;

namespace MobileServer_withDB
{
	class RequestLock : IDisposable
	{
		ERROR_CODE isRequestable = ERROR_CODE.PREV_REQUEST_NOT_COMPLETE;
		string key;

		public RequestLock(string id)
		{
			key = string.Format("{0}:AT", id);
		}

		public void SetInit()
		{
			DB.Redis.SetStringNoReturn<double>(key, 0);
		}

		public ERROR_CODE isProcessingReq([CallerFilePath] string fileName = "",
			[CallerMemberName] string methodName = "",
			[CallerLineNumber] int lineNumber = 0)
		{
			double prevAllTimeSec = 0;
			var curAllTimeSec = TimeTickToSec(DateTime.Now.Ticks);

			try
			{
				var result = DB.Redis.GetString<double>(key);

				if(result.Item1)
				{
					prevAllTimeSec = result.Item2;
					if (prevAllTimeSec > 0)
						return isRequestable;
				}
				else
				{
					Console.WriteLine("[RequestLock] 처음 요청");
					DB.Redis.SetStringAsync<double>(key, 0).Wait();
				}

				var changeData = DB.Redis.Increment(key, curAllTimeSec);
				if (changeData != curAllTimeSec)
					return isRequestable;
				isRequestable = ERROR_CODE.NONE;
				return isRequestable;
			}
			catch(Exception e)
			{
				Console.Write("[RequestLock] 예외 발생 : " + e.Message);
				return ERROR_CODE.PREV_REQUEST_FAIL_REDIS;
			}
		}

		public void Dispose()
		{
			if(isRequestable == ERROR_CODE.NONE)
			{
				SetInit();
			}
		}

		Int64 TimeTickToSec(Int64 curTimeTick)
		{
			Int64 sec = (Int64)(curTimeTick / TimeSpan.TicksPerSecond);
			return sec;
		}
	}
}
