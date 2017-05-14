using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;

namespace CopyHelp
{
    class Program
    {
        static void Main(string[] args)
        {
            var puz = "Puzzles";
            var d = Directory.GetCurrentDirectory() + "/../../..";
            var lst = Directory.GetDirectories(d).Where(d2 =>
            {
                var f = Path.GetFileName(d2);
                return f.StartsWith(puz) && f.Length - puz.Length == 1;
            }).SelectMany(d2 => Directory.GetFiles(d2 + "/Puzzles", "*.xml"))
            .ToList();
            foreach (var f in lst)
            {
                var lines = File.ReadAllLines(f).ToList();
                if (lines[2] != "<levels>") continue;
                int i = 2;
                lines.Insert(i++, "<help>");
                lines.Insert(i++, "  <![CDATA[");
                lines.Insert(i++, "  ]]>");
                lines.Insert(i++, "</help>");
                File.WriteAllLines(f, lines);
            }
        }
    }
}
