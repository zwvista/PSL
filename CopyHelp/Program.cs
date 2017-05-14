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
            var pslList = Directory.GetDirectories(@"D:\Programs\Games\PSL").Where(d2 =>
            {
                var f = Path.GetFileName(d2);
                return f.StartsWith(puz) && f.Length - puz.Length == 1;
            }).Select(d2 => d2 + "/Puzzles").ToList();
            var cppList = pslList.SelectMany(d2 => Directory.GetFiles(d2, "*.cpp")).ToList();
            var stateList = Directory.GetFiles(@"D:\Programs\Games\LogicPuzzlesAndroid\app\src\main\java\com\zwstudio\logicpuzzlesandroid\puzzles", "*GameState.java", SearchOption.AllDirectories)
                .ToList();
            var cpp2state = (from c in cppList
                           from x in stateList
                           where Path.GetFileNameWithoutExtension(x).StartsWith(Path.GetFileNameWithoutExtension(c))
                           select new { Cpp = c, State = x }).ToList();

            foreach (var kv in cpp2state)
            {
                var lines = File.ReadAllLines(kv.Cpp).ToList();
                var a = lines.FindIndex(s2 => s2.Trim() == "/*");
                var b = lines.FindIndex(s2 => s2.Trim() == "*/");
                if (a == -1 || b == -1) continue;
                lines = lines.Skip(a).Take(b - a + 1).ToList();
                var lines2 = File.ReadAllLines(kv.State).ToList();
                a = lines2.FindIndex(s2 => s2.Trim() == "private void updateIsSolved() {");
                if (a == -1) continue;
                int i = a;
                foreach (var s2 in lines)
                    lines2.Insert(i++, s2.Trim() == "" ? "" : "    " + s2);
                File.WriteAllLines(kv.State, lines2);
            }
        }
    }
}
