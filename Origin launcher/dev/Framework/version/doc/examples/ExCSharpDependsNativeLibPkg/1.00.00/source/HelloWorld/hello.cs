using System;


namespace HelloWorldTest
{

    class Program
    {
        [System.Runtime.InteropServices.DllImport("NativeLib.dll")]
        private static extern int GetInt();

        [System.Runtime.InteropServices.DllImport("NativeLib.dll")]
        private static extern void Hello();

        static void Main(string[] args)
        {
            Console.WriteLine("Hello world from C# Console Program.");

            // Now call the native DLL functions.
            int val = GetInt();
            Console.WriteLine("GetInt() returns: {0}",val);
            Hello();
        }
    }
}


