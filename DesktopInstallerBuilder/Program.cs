using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;

namespace DesktopInstallerBuilder
{
    class Program
    {
        private static string WalkParent(string path, int count)
        {
            var dir = new DirectoryInfo(path);

            foreach (int _ in Enumerable.Range(0, count))
            {
                dir = dir.Parent ?? throw new Exception("Tried to get parent of folder structure root.");
            }

            return dir.FullName;
        }

        private static string GetInPath(string fileName)
        {
            if (File.Exists(fileName))
            {
                return Path.GetFullPath(fileName);
            }

            foreach (var path in Environment.GetEnvironmentVariable("PATH").Split(';'))
            {
                var fullPath = Path.Combine(path, fileName);
                if (File.Exists(fullPath))
                {
                    return fullPath;
                }
            }
            return null;
        }

        public static int Main()
        {
            var p = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = GetInPath("ISCC.exe") ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Inno Setup 6", "ISCC.exe"),
                    Arguments = Path.Combine(WalkParent(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), 4), "DesktopInstaller", "main.iss"),
                    UseShellExecute = false,
                    RedirectStandardOutput = true
                }
            };

            try
            {
                p.OutputDataReceived += (s, e) => Console.WriteLine(e.Data);
                p.Start();
                p.BeginOutputReadLine();
                p.WaitForExit();

                return p.ExitCode;
            }
            catch (Exception ex)
            {
                var originalColor = Console.ForegroundColor;
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine(ex);
                Console.ForegroundColor = originalColor;

                return 1;
            }
            finally
            {
                p.Dispose();
            }
        }
    }
}
