using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Microsoft.Owin.Hosting;

namespace MobileServer_withDB
{
	class Program
	{
		static void Main(string[] args)
		{
			string baseAddress = "http://127.0.0.1:19000/";

			using (WebApp.Start<StartUp>(url: baseAddress))
			{
				MobileServer_withDB.Main.Init();

				Console.WriteLine("Web API 실행 중: " + baseAddress);
				Console.ReadLine();
			}
		}
	}
}
