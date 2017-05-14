using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Text.RegularExpressions;

namespace CopyHelp
{
    class Program
    {
        class CppXmlComparer : IEqualityComparer<String>
        {
            public bool Equals(String s1, String s2)
            {
                var len = Math.Min(s1.Length, s2.Length);
                return s1.Substring(0, len) == s2.Substring(0, len);
            }

            public int GetHashCode(String s)
            {
                return s.GetHashCode();
            }
        }
        static void Main(string[] args)
        {
            var puz = "Puzzles";
            var d = Directory.GetCurrentDirectory() + "/../../..";
            var dirList = Directory.GetDirectories(d).Where(d2 =>
            {
                var f = Path.GetFileName(d2);
                return f.StartsWith(puz) && f.Length - puz.Length == 1;
            }).Select(d2 => d2 + "/Puzzles").ToList();
            var xmlList = dirList.SelectMany(d2 => Directory.GetFiles(d2, "*.xml")).ToList();
            var cppList = dirList.SelectMany(d2 => Directory.GetFiles(d2, "*.cpp")).ToList();
            var cpp2xml = (from c in cppList
                           from x in xmlList
                           where Path.GetFileNameWithoutExtension(x).StartsWith(Path.GetFileNameWithoutExtension(c))
                           select new { Cpp = c, Xml = x }).ToList();

            var reg = new Regex(@"^\s+\d+\. ");
            foreach (var kv in cpp2xml)
            {
                var lines = File.ReadAllLines(kv.Cpp).ToList();
                var a = lines.FindIndex(s2 => s2.Trim() == "/*");
                var b = lines.FindIndex(s2 => s2.Trim() == "*/");
                if (a == -1 || b == -1) continue;
                lines = lines.Skip(a).Take(b - a).ToList();
                a = lines.FindIndex(s2 => s2.Trim() == "Description");
                if (a == -1) continue;
                lines = lines.Skip(a + 1).ToList();
                var lines2 = new List<String>();
                var s = "";
                foreach (var s2 in lines)
                {
                    if (reg.IsMatch(s2))
                    {
                        if (s != "") lines2.Add(s);
                        s = s2.Trim();
                    }
                    else
                        s += " " + s2.Trim();
                }
                if (s != "") lines2.Add(s);

                lines = File.ReadAllLines(kv.Xml).ToList();
                if (lines[2] != "<help>") continue;
                int i = 4;
                foreach (var s2 in lines2)
                    lines.Insert(i++, s2);
                File.WriteAllLines(kv.Xml, lines);
            }
        }
    }
}
