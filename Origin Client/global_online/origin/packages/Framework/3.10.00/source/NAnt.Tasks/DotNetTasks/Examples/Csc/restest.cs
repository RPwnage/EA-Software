using System;
using System.IO;
using System.Reflection;

class ResourceTest {
    static void Main() {
        Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("resource.txt");
        StreamReader reader = new StreamReader(stream);
        Console.WriteLine(reader.ReadToEnd());
        reader.Close();
        stream.Close();
    }
}
