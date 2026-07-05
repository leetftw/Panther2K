using DiscUtils.Udf;
using Microsoft.Deployment.Compression.Cab;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Schema;
using System.Xml.Serialization;

namespace pantherScripts
{
    internal class Program
    {
        public class TaskResult
        {
            
        }

        public class TaskContext
        {
            // Keep reference of working files
            public List<string> WorkingFiles { get; set; } = new List<string>();

            // For fetching image from MS
            public MctCatalog EsdCatalog { get; set; } = null;
            public EsdFile SelectedEsd { get; set; } = null;

            // Base script
            public string ImageFile { get; set; } = null;
            public string PantherFile { get; set; } = null;
            public IntPtr WimHandle { get; set; } = IntPtr.Zero;
            public IntPtr MountedImage { get; set; } = IntPtr.Zero;
            public bool IncludeInstallImage { get; set; } = true;
            public bool ReplaceWimWithEsd { get; set; } = true;
        }

        public delegate TaskResult TaskDelegate(TaskContext context);

        private static IEnumerable<string> WordWrap(string text, int maxLineLength)
        {
            var words = text.Split(' ');
            var line = new StringBuilder();
            foreach (var word in words)
            {
                if (line.Length + word.Length + 1 > maxLineLength)
                {
                    if (line.Length > 0)
                    {
                        yield return line.ToString();
                        line.Clear();
                    }
                }
                if (line.Length > 0)
                    line.Append(' ');
                line.Append(word);
            }
            if (line.Length > 0)
                yield return line.ToString();
        }

        public static void ClearAndWriteHeader(string message)
        {
            Console.Clear();
            Console.WriteLine("==================================================");
            Console.WriteLine(" pantherScripts 2.0.0 pre-alpha");
            Console.WriteLine();
            var lines = message.Replace("\r\n", "\n")
                               .Split('\n')
                               .SelectMany(a => WordWrap(a, 50))
                               .Select(a => " " + a);
            Console.WriteLine(string.Join("\r\n", lines));
            Console.WriteLine("==================================================");
            Console.WriteLine();
        }

        public static TaskResult RetrieveLatestESDCatalog(TaskContext context)
        {
            ClearAndWriteHeader("Retrieving latest ESD catalog...");

            // win10: https://go.microsoft.com/fwlink/?LinkId=841361
            // win11: https://go.microsoft.com/fwlink/?LinkId=2156292

            string catalogUrl = "https://go.microsoft.com/fwlink/?LinkId=2156292";

            using (WebClient client = new WebClient { Proxy = null })
            using (MemoryStream cabFile = new MemoryStream())
            using (Stream cabFileNetwork = client.OpenRead(catalogUrl))
            {
                cabFileNetwork.CopyTo(cabFile);
                cabFile.Position = 0;
                using (CabEngine cabEngine = new CabEngine())
                using (Stream productsXml = cabEngine.Unpack(cabFile, "products.xml"))
                using (StreamReader streamReader = new StreamReader(productsXml, Encoding.UTF8))
                {
                    XmlSerializer serializer = new XmlSerializer(typeof(MctCatalog));
                    MctCatalog catalog = (MctCatalog)serializer.Deserialize(streamReader);
                    context.EsdCatalog = catalog;
                }
            }

            return new TaskResult();
        }

        public static TaskResult SelectESDFromCatalog(TaskContext context)
        {
            ClearAndWriteHeader("Select an ESD to download.");

            if (context.EsdCatalog == null || context.EsdCatalog.Catalogs.Count == 0)
            {
                throw new InvalidOperationException("ESD catalog is not loaded.");
            }

            string lang = "en-us";

            // First print latest entry for each architecture
            var latestBuilds = context.EsdCatalog.Catalogs
                .SelectMany(c => c.PublishedMedia)
                .SelectMany(pm => pm.Files)
                .Where(f => f.LanguageCode.ToLowerInvariant() == lang)
                .GroupBy(f => f.Architecture)
                .Select(g => g.FirstOrDefault())
                .ToList();

            // Let user select an ESD from the catalog
            Console.WriteLine("Available ESDs:");
            for (int i = 0; i < latestBuilds.Count; i++)
            {
                var file = latestBuilds[i];
                Console.WriteLine($"{i + 1}: {file.FileName} ({file.Architecture}) - {file.Size} bytes");
            }

            int index = -1;
            while (true)
            {
                Console.WriteLine("Select an ESD by number (or press Enter to skip):");
                string input = Console.ReadLine();
                if (!string.IsNullOrWhiteSpace(input) && int.TryParse(input, out index) && index >= 1 && index <= latestBuilds.Count)
                {
                    break;
                }
                Console.WriteLine("Invalid selection. Please enter a valid number.");
            }

            index--;
            Console.WriteLine("Selected: " + latestBuilds[index].FileName);
            context.SelectedEsd = latestBuilds[index];
            return new TaskResult();
        }

        public static TaskResult DownloadSelectedEsd(TaskContext context)
        {
            ClearAndWriteHeader("Downloading ESD from Microsoft...");

            // Download the ESD as fast as possible
            // Maybe even using multiple connections to speed up the download
            if (context.SelectedEsd == null)
            {
                throw new InvalidOperationException("No ESD selected.");
            }

            string esdUrl = context.SelectedEsd.FilePath;
            string esdFileName = Path.GetFileName(esdUrl);
            string esdFilePath = Path.Combine(Directory.GetCurrentDirectory(), esdFileName);
            Console.WriteLine($"Downloading {esdFileName}.");

            // 1. Get file size
            long fileSize;
            var req = (HttpWebRequest)WebRequest.Create(esdUrl);
            req.Method = "HEAD";
            using (var resp = req.GetResponse())
                fileSize = resp.ContentLength;

            int partCount = 8; // Number of parallel downloads
            long partSize = fileSize / partCount;
            object fileLock = new object();
            long totalWritten = 0;

            // 2. Create the output file with the correct size
            using (var fs = new FileStream(esdFilePath, FileMode.Create, FileAccess.Write, FileShare.Write))
            {
                fs.SetLength(fileSize);
            }

            // 3. Download parts in parallel and write directly to file
            Parallel.For(0, partCount, i =>
            {
                long start = i * partSize;
                long end = (i == partCount - 1) ? fileSize - 1 : (start + partSize - 1);

                var partReq = (HttpWebRequest)WebRequest.Create(esdUrl);
                partReq.AddRange(start, end);
                using (var partResp = partReq.GetResponse())
                using (var stream = partResp.GetResponseStream())
                {
                    byte[] buffer = new byte[1 * 1024 * 1024];
                    int bytesRead;
                    long position = start;
                    using (var fs = new FileStream(esdFilePath, FileMode.Open, FileAccess.Write, FileShare.Write))
                    {
                        fs.Position = position;
                        while ((bytesRead = stream.Read(buffer, 0, buffer.Length)) > 0)
                        {
                            lock (fileLock)
                            {
                                fs.Position = position;
                                fs.Write(buffer, 0, bytesRead);
                                position += bytesRead;
                                totalWritten += bytesRead;
                                // Progress bar
                                double percent = (double)totalWritten / fileSize;
                                int barWidth = 40;
                                int filled = (int)(percent * barWidth);
                                string bar = new string('=', filled) + new string(' ', barWidth - filled);
                                string progress = $"\r[{bar}] {percent * 100,6:##0.00}% ({totalWritten / 1024 / 1024} MB of {fileSize / 1024 / 1024} MB)".PadRight(Console.BufferWidth - 1);
                                Console.Write(progress);
                            }
                        }
                    }
                }
            });

            Console.WriteLine("\r\nDownload completed.");
            context.ImageFile = Path.GetFullPath(esdFilePath);
            context.WorkingFiles.Add(context.ImageFile);

            return new TaskResult();
        }

        public static TaskResult DisplayOverview(TaskContext context)
        {
            ClearAndWriteHeader("Overview of selected preferences.");

            Console.WriteLine("Selected files:");
            Console.WriteLine($"Panther2K ZIP: {context.PantherFile}");
            Console.WriteLine($"Source image file: {context.ImageFile}" + (context.WorkingFiles.Contains(context.ImageFile) ? " (downloaded from Windows Update)" : ""));
            Console.WriteLine($"Install image will{(context.IncludeInstallImage ? "" : " not")} be included.");
            Console.WriteLine($"Install image will{(context.ReplaceWimWithEsd ? "" : " not")} be recompressed to ESD.");
            Console.WriteLine("Press any key to continue, or Escape to cancel.");

            if (Console.ReadKey().Key == ConsoleKey.Escape)
            {
                Console.WriteLine("Exiting...");
                throw new OperationCanceledException("User cancelled the operation.");
            }
            return new TaskResult();
        }

        public static TaskResult CreateWorkingDirectories(TaskContext context)
        {
            ClearAndWriteHeader("Creating working directories...");

            Directory.CreateDirectory("./ISO");
            Directory.CreateDirectory("./WimgapiTemp");
            context.WorkingFiles.Add("./ISO");
            context.WorkingFiles.Add("./WimgapiTemp");
            return new TaskResult();
        }

        private static void ExtractISORecursive(UdfReader reader, string path, string outputDir)
        {
            // Read the UDF file system and extract files
            foreach (var file in reader.GetFiles(path))
            {
                string targetPath = Path.GetFullPath(outputDir) + file;
                Directory.CreateDirectory(Path.GetDirectoryName(targetPath));
                using (Stream outputStream = File.Create(targetPath))
                {
                    Console.WriteLine($"Extracting {file}.");
                    using (Stream udfFile = reader.GetFileInfo(file).OpenRead())
                        udfFile.CopyTo(outputStream);
                }
            }
            foreach (var directory in reader.GetDirectories(path))
            {
                // Recursively extract subdirectories
                ExtractISORecursive(reader, Path.Combine(path, directory), outputDir);
            }
        }

        public static TaskResult ISOExtract(TaskContext context)
        {
            ClearAndWriteHeader("Extracting ISO file...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }
            if (!File.Exists(context.ImageFile))
            {
                throw new FileNotFoundException("Selected image file does not exist.", context.ImageFile);
            }

            using (Stream fs = File.OpenRead(context.ImageFile))
            using (UdfReader reader = new UdfReader(fs))
            {
                ExtractISORecursive(reader, "", "./ISO");
            }

            return new TaskResult();
        }

        public static TaskResult ESDExtractSetupRoot(TaskContext context)
        {
            ClearAndWriteHeader("Extracting Windows Setup from ESD...");

            if (!File.Exists(context.ImageFile))
            {
                throw new FileNotFoundException("Selected iamge file does not exist.", context.ImageFile);
            }
            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }
            if (!Directory.Exists("./WimgapiTemp"))
            {
                throw new DirectoryNotFoundException("Temp directory for Wimgapi does not exist.");
            }

            uint result = 0;
            IntPtr hWim = Windows.Imaging.WimCreateFile(context.ImageFile, Windows.Imaging.DesiredAccess.GenericRead, Windows.Imaging.CreationDisposition.OpenExisting,
                Windows.Imaging.FlagsAndAttributes.SolidCompression, Windows.Imaging.CompressionType.Recovery, ref result);
            if (hWim == IntPtr.Zero) throw new Win32Exception();

            Windows.Imaging.MessageCallback callback = (Windows.Imaging.MessageId dwMessageId, IntPtr wParam, IntPtr lParam, IntPtr pvUserData) =>
            {
                if (dwMessageId == Windows.Imaging.MessageId.Progress)
                {
                    // Progress bar
                    double percent = (wParam.ToInt32() / 100.0);
                    int barWidth = 40;
                    int filled = (int)(percent * barWidth);
                    string bar = new string('=', filled) + new string(' ', barWidth - filled);
                    string progress = $"\r[{bar}] {percent * 100,6:##0.00}% (ETA: {new TimeSpan(0,0,0,0,lParam.ToInt32()):g})".PadRight(Console.BufferWidth - 1);
                    Console.Write(progress);
                }

                return Windows.Imaging.CallbackResult.Success;
            };
            if (Windows.Imaging.WimRegisterMessageCallback(hWim, callback, IntPtr.Zero) == 0xFFFFFFFF)
                throw new Win32Exception();

            if (!Windows.Imaging.WimSetTemporaryPath(hWim, "./WimgapiTemp"))
                throw new Win32Exception();

            IntPtr hImage = Windows.Imaging.WimLoadImage(hWim, 1);
            if (hImage == IntPtr.Zero) throw new Win32Exception();

            Console.WriteLine("Extracting ISO root.");
            if (!Windows.Imaging.WimApplyImage(hImage, "./ISO", Windows.Imaging.FlagsAndAttributes.Index))
                throw new Win32Exception();
            Console.WriteLine();

            if (!Windows.Imaging.WimCloseHandle(hImage))
                throw new Win32Exception();
            if (!Windows.Imaging.WimCloseHandle(hWim))
                throw new Win32Exception();

            return new TaskResult();
        }

        public static TaskResult CopyAutorun(TaskContext context)
        {
            ClearAndWriteHeader("Copying autorun.inf to ISO root...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }

            File.Copy("./resources/autorun.inf", "./ISO/autorun.inf", true);
            return new TaskResult();
        }

        public static TaskResult ISOTempMoveInstallWim(TaskContext context)
        {
            ClearAndWriteHeader("Moving installation image...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }
            if (!Directory.Exists("./WimgapiTemp"))
            {
                throw new DirectoryNotFoundException("Temp directory for Wimgapi does not exist.");
            }
            if (File.Exists("./ISO/sources/install.wim") && File.Exists("./ISO/sources/install.esd"))
            {
                throw new InvalidDataException("Both install.wim and install.esd exist in ISO root.");
            }
            if (!File.Exists("./ISO/sources/boot.wim"))
            {
                throw new FileNotFoundException("boot.wim does not exist in ISO root.", "./ISO/sources/boot.wim");
            }

            if (File.Exists("./ISO/sources/install.wim"))
            {
                // Move install.wim to sources folder
                if (context.ReplaceWimWithEsd)
                {
                    // Copy each image in install.wim individually
                    uint result = 0;
                    IntPtr hWim = Windows.Imaging.WimCreateFile("./ISO/sources/install.wim", Windows.Imaging.DesiredAccess.GenericRead, Windows.Imaging.CreationDisposition.OpenExisting, Windows.Imaging.FlagsAndAttributes.None, Windows.Imaging.CompressionType.Maximum, ref result);
                    if (hWim == IntPtr.Zero)
                        throw new Win32Exception();

                    if (!Windows.Imaging.WimSetTemporaryPath(hWim, "./WimgapiTemp"))
                        throw new Win32Exception();

                    Console.WriteLine("Converting install.wim to install.esd");
                    IntPtr hTargetFile = Windows.Imaging.WimCreateFile("./ISO/install.esd", Windows.Imaging.DesiredAccess.GenericWrite, Windows.Imaging.CreationDisposition.CreateAlways, Windows.Imaging.FlagsAndAttributes.SolidCompression, Windows.Imaging.CompressionType.Recovery, ref result);
                    if (hTargetFile == IntPtr.Zero)
                        throw new Win32Exception();

                    if (!Windows.Imaging.WimSetTemporaryPath(hTargetFile, "./WimgapiTemp"))
                        throw new Win32Exception();

                    Windows.Imaging.MessageCallback callback = (Windows.Imaging.MessageId dwMessageId, IntPtr wParam, IntPtr lParam, IntPtr pvUserData) =>
                    {
                        if (dwMessageId == Windows.Imaging.MessageId.Progress)
                        {
                            // Progress bar
                            double percent = (wParam.ToInt32() / 100.0);
                            int barWidth = 40;
                            int filled = (int)(percent * barWidth);
                            string bar = new string('=', filled) + new string(' ', barWidth - filled);
                            string progress = $"\r[{bar}] {percent * 100,6:##0.00}% (ETA: {new TimeSpan(0, 0, 0, 0, lParam.ToInt32()):g})".PadRight(Console.BufferWidth - 1);
                            Console.Write(progress);
                        }
                        return Windows.Imaging.CallbackResult.Success;
                    };
                    if (Windows.Imaging.WimRegisterMessageCallback(hTargetFile, callback, IntPtr.Zero) == 0xFFFFFFFF)
                        throw new Win32Exception();

                    uint imageCount = Windows.Imaging.WimGetImageCount(hWim);
                    if (imageCount == 0)
                        throw new Win32Exception();
                    
                    for (uint i = 1; i <= imageCount; i++)
                    {
                        IntPtr hImage = Windows.Imaging.WimLoadImage(hWim, i);
                        if (hImage == IntPtr.Zero) throw new Win32Exception();

                        Console.WriteLine($"Exporting image {i} to install.esd.");
                        if (!Windows.Imaging.WimExportImage(hImage, hTargetFile, Windows.Imaging.ExportFlags.None))
                            throw new Win32Exception();
                        Console.WriteLine();
                        
                        if (!Windows.Imaging.WimCloseHandle(hImage))
                            throw new Win32Exception();
                    }

                    if (!Windows.Imaging.WimCloseHandle(hTargetFile))
                        throw new Win32Exception();

                    if (!Windows.Imaging.WimCloseHandle(hWim))
                        throw new Win32Exception();
                }
                else File.Move("./ISO/sources/install.wim", "./ISO/install.wim");
            }
            else if (File.Exists("./ISO/sources/install.esd"))
            {
                // Move install.esd to sources folder
                File.Move("./ISO/sources/install.esd", "./ISO/install.esd");
            }
            else
            {
                throw new FileNotFoundException("No install.wim or install.esd found in ISO root.");
            }

            File.Move("./ISO/sources/boot.wim", "./ISO/boot.wim");

            return new TaskResult();
        }

        public static TaskResult RemoveSetupFromRoot(TaskContext context)
        {
            ClearAndWriteHeader("Removing Windows Setup from ISO root...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }

            if (Directory.Exists("./ISO/sources")) Directory.Delete("./ISO/sources", true);
            if (Directory.Exists("./ISO/support")) Directory.Delete("./ISO/support", true);
            if (Directory.Exists("./ISO/upgrade")) Directory.Delete("./ISO/upgrade", true);
            if (File.Exists("./ISO/setup.exe")) File.Delete("./ISO/setup.exe");

            return new TaskResult();
        }

        public static TaskResult AddPantherToRoot(TaskContext context)
        {
            ClearAndWriteHeader("Adding Panther2K to ISO root...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }

            Directory.CreateDirectory("./ISO/Panther2K");

            // Extract panther zip to directory
            ZipFile.ExtractToDirectory(context.PantherFile, "./ISO/Panther2K");

            return new TaskResult();
        }
        
        public static TaskResult CreateSourcesFolder(TaskContext context)
        {
            ClearAndWriteHeader("Creating sources folder...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }

            Directory.CreateDirectory("./ISO/sources");

            return new TaskResult();
        }

        public static TaskResult ISOMoveBackImagesToSources(TaskContext context)
        {
            ClearAndWriteHeader("Moving back installation images to sources...");
            
            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }
            if (!File.Exists("./ISO/install.wim") && !File.Exists("./ISO/install.esd"))
            {
                throw new FileNotFoundException("Both install.wim and install.esd do not exist in ISO root.");
            }
            if (!File.Exists("./ISO/boot.wim"))
            {
                throw new FileNotFoundException("boot.wim does not exist in ISO root.", "./ISO/boot.wim");
            }

            if (File.Exists("./ISO/install.wim"))
            {
                File.Move("./ISO/install.wim", "./ISO/sources/install.wim");
                context.ImageFile = Path.GetFullPath("./ISO/sources/install.wim");
            }
            else
            {
                File.Move("./ISO/install.esd", "./ISO/sources/install.esd");
                context.ImageFile = Path.GetFullPath("./ISO/sources/install.esd");
            }

            File.Move("./ISO/boot.wim", "./ISO/sources/boot.wim");

            return new TaskResult();
        }

        public static TaskResult ESDCreateBootWim(TaskContext context)
        {
            ClearAndWriteHeader("Creating boot.wim from ESD file...");

            if (!File.Exists(context.ImageFile))
            {
                throw new FileNotFoundException("Selected iamge file does not exist.", context.ImageFile);
            }
            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }
            if (!Directory.Exists("./ISO/sources"))
            {
                throw new DirectoryNotFoundException("ISO/sources directory does not exist.");
            }
            if (!Directory.Exists("./WimgapiTemp"))
            {
                throw new DirectoryNotFoundException("Temp directory for Wimgapi does not exist.");
            }

            uint result = 0;
            IntPtr hSrcFile = Windows.Imaging.WimCreateFile(context.ImageFile, Windows.Imaging.DesiredAccess.GenericRead, Windows.Imaging.CreationDisposition.OpenExisting,
                Windows.Imaging.FlagsAndAttributes.SolidCompression, Windows.Imaging.CompressionType.Recovery, ref result);
            if (hSrcFile == IntPtr.Zero) throw new Win32Exception();

            if (!Windows.Imaging.WimSetTemporaryPath(hSrcFile, "./WimgapiTemp"))
                throw new Win32Exception();

            IntPtr hSrcImage = Windows.Imaging.WimLoadImage(hSrcFile, 2);
            if (hSrcImage == IntPtr.Zero) throw new Win32Exception();

            IntPtr hTargetFile = Windows.Imaging.WimCreateFile("./ISO/sources/boot.wim", Windows.Imaging.DesiredAccess.GenericWrite, Windows.Imaging.CreationDisposition.CreateAlways,
                Windows.Imaging.FlagsAndAttributes.None, Windows.Imaging.CompressionType.Maximum, ref result);
            if (hTargetFile == IntPtr.Zero) throw new Win32Exception();

            Windows.Imaging.MessageCallback callback = (Windows.Imaging.MessageId dwMessageId, IntPtr wParam, IntPtr lParam, IntPtr pvUserData) =>
            {
                if (dwMessageId == Windows.Imaging.MessageId.Progress)
                {
                    // Progress bar
                    double percent = (wParam.ToInt32() / 100.0);
                    int barWidth = 40;
                    int filled = (int)(percent * barWidth);
                    string bar = new string('=', filled) + new string(' ', barWidth - filled);
                    string progress = $"\r[{bar}] {percent * 100,6:##0.00}% (ETA: {new TimeSpan(0, 0, 0, 0, lParam.ToInt32()):g})".PadRight(Console.BufferWidth - 1);
                    Console.Write(progress);
                }

                return Windows.Imaging.CallbackResult.Success;
            };
            if (Windows.Imaging.WimRegisterMessageCallback(hTargetFile, callback, IntPtr.Zero) == 0xFFFFFFFF)
                throw new Win32Exception();

            if (!Windows.Imaging.WimSetTemporaryPath(hTargetFile, "./WimgapiTemp"))
                throw new Win32Exception();

            Console.WriteLine("Copying Windows PE to boot.wim.");
            if (!Windows.Imaging.WimExportImage(hSrcImage, hTargetFile, Windows.Imaging.ExportFlags.None))
                throw new Win32Exception();
            Console.WriteLine();

            if (!Windows.Imaging.WimCloseHandle(hSrcImage))
                throw new Win32Exception();

            hSrcImage = Windows.Imaging.WimLoadImage(hSrcFile, 3);
            if (hSrcImage == IntPtr.Zero) throw new Win32Exception();

            Console.WriteLine("Copying Windows Setup to boot.wim.");
            if (!Windows.Imaging.WimExportImage(hSrcImage, hTargetFile, Windows.Imaging.ExportFlags.None))
                throw new Win32Exception();
            Console.WriteLine();

            if (!Windows.Imaging.WimCloseHandle(hSrcImage))
                throw new Win32Exception();

            if (!Windows.Imaging.WimCloseHandle(hTargetFile))
                throw new Win32Exception();

            if (!Windows.Imaging.WimCloseHandle(hSrcFile))
                throw new Win32Exception();

            return new TaskResult();
        }

        public static TaskResult CreateWimMountFolder(TaskContext context)
        {
            ClearAndWriteHeader("Creating folder to mount image...");

            Directory.CreateDirectory("./WimMount");
            context.WorkingFiles.Add("./WimMount");
            return new TaskResult();
        }

        public static TaskResult MountBootWim(TaskContext context)
        {
            ClearAndWriteHeader("Mounting boot image...");

            if (!File.Exists("./ISO/sources/boot.wim"))
            {
                throw new FileNotFoundException("boot.wim does not exist.");
            }
            if (!Directory.Exists("./WimMount"))
            {
                throw new DirectoryNotFoundException("WimMount directory does not exist.");
            }
            if (!Directory.Exists("./WimgapiTemp"))
            {
                throw new DirectoryNotFoundException("Temp directory for Wimgapi does not exist.");
            }

            uint result = 0;
            IntPtr hWim = Windows.Imaging.WimCreateFile("./ISO/sources/boot.wim", Windows.Imaging.DesiredAccess.GenericWrite | Windows.Imaging.DesiredAccess.GenericExecute, Windows.Imaging.CreationDisposition.OpenExisting,
                Windows.Imaging.FlagsAndAttributes.None, Windows.Imaging.CompressionType.Maximum, ref result);
            if (hWim == IntPtr.Zero) throw new Win32Exception();

            if (!Windows.Imaging.WimSetTemporaryPath(hWim, "./WimgapiTemp"))
                throw new Win32Exception();

            IntPtr hImage = Windows.Imaging.WimLoadImage(hWim, 1);
            if (hImage == IntPtr.Zero) throw new Win32Exception();

            Windows.Imaging.MessageCallback callback = (Windows.Imaging.MessageId dwMessageId, IntPtr wParam, IntPtr lParam, IntPtr pvUserData) =>
            {
                if (dwMessageId == Windows.Imaging.MessageId.Progress)
                {
                    // Progress bar
                    double percent = (wParam.ToInt32() / 100.0);
                    int barWidth = 40;
                    int filled = (int)(percent * barWidth);
                    string bar = new string('=', filled) + new string(' ', barWidth - filled);
                    string progress = $"\r[{bar}] {percent * 100,6:##0.00}% (ETA: {new TimeSpan(0, 0, 0, 0, lParam.ToInt32()):g})".PadRight(Console.BufferWidth - 1);
                    Console.Write(progress);
                }

                return Windows.Imaging.CallbackResult.Success;
            };
            if (Windows.Imaging.WimRegisterMessageCallback(hWim, callback, IntPtr.Zero) == 0xFFFFFFFF)
                throw new Win32Exception();

            Console.WriteLine("Mounting boot.wim to ./WimMount.");
            if (!Windows.Imaging.WimMountImageHandle(hImage, "./WimMount", Windows.Imaging.FlagsAndAttributes.MountFast))
                throw new Win32Exception();
            Console.WriteLine();

            context.WimHandle = hWim;
            context.MountedImage = hImage;

            return new TaskResult();
        }

        public static TaskResult AddPanther2KToWimMount(TaskContext context)
        {
            ClearAndWriteHeader("Configuring boot image to load Panther2K...");

            if (!Directory.Exists("./WimMount"))
            {
                throw new DirectoryNotFoundException("WimMount directory does not exist.");
            }

            Console.WriteLine("Adding Panther2K load script to boot.wim.");
            File.Copy("./resources/startnet.cmd", "./WimMount/Windows/System32/startnet.cmd", true);

            return new TaskResult();
        }

        public static TaskResult ConfigureMountedPEInstRoot(TaskContext context)
        {
            ClearAndWriteHeader("Configuring boot image target path...");

            // SOFTWARE hive is in ./WimMount/Windows/System32/config/SOFTWARE
            // HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\WinPE\InstRoot
            // Set this to X:\
            if (!Directory.Exists("./WimMount"))
            {
                throw new DirectoryNotFoundException("WimMount directory does not exist.");
            }

            // Load the SOFTWARE hive
            string softwareHivePath = Path.Combine("./WimMount/Windows/System32/config", "SOFTWARE");
            if (!File.Exists(softwareHivePath))
            {
                throw new FileNotFoundException("SOFTWARE hive does not exist.", softwareHivePath);
            }

            int returnCode = Windows.Registry.LoadAppKey(softwareHivePath, out IntPtr hKey, 0xF003F, 1, 0);
            if (returnCode != 0)
            {
                throw new Win32Exception(returnCode, "Failed to load SOFTWARE hive.");
            }

            Console.WriteLine("Setting installation root for boot.wim.");
            using (var safeHandle = new Microsoft.Win32.SafeHandles.SafeRegistryHandle(hKey, ownsHandle: false))
            using (var baseKey = RegistryKey.FromHandle(safeHandle))
            using (var winPEKey = baseKey.CreateSubKey(@"Microsoft\Windows NT\CurrentVersion\WinPE"))
            {
                winPEKey.SetValue("InstRoot", "X:\\");
            }

            returnCode = Windows.Registry.FlushKey(hKey);
            if (returnCode != 0)
            {
                throw new Win32Exception(returnCode, "Failed to flush SOFTWARE hive handle.");
            }

            returnCode = Windows.Registry.CloseKey(hKey);
            if (returnCode != 0)
            {
                throw new Win32Exception(returnCode, "Failed to close SOFTWARE hive handle.");
            }

            return new TaskResult();
        }

        public static TaskResult SaveBootWim(TaskContext context)
        {
            ClearAndWriteHeader("Saving modified boot image...");

            if (!Directory.Exists("./WimMount"))
            {
                throw new DirectoryNotFoundException("WimMount directory does not exist.");
            }

            if (context.WimHandle == IntPtr.Zero || context.MountedImage == IntPtr.Zero)
            {
                throw new InvalidOperationException("WIM handle or mounted image is not set.");
            }

            // Commit changes to the mounted WIM image
            Console.WriteLine("Committing changes to boot.wim.");
            IntPtr resultingImage = IntPtr.Zero;
            if (!Windows.Imaging.WimCommitImageHandle(context.MountedImage, Windows.Imaging.FlagsAndAttributes.None, ref resultingImage))
            {
                throw new Win32Exception();
            }
            Console.WriteLine();

            // Unmount the image after committing
            Console.WriteLine("Unmounting boot.wim.");
            if (!Windows.Imaging.WimUnmountImageHandle(context.MountedImage))
            {
                throw new Win32Exception();
            }
            Console.WriteLine();

            // Close all handles
            if (!Windows.Imaging.WimCloseHandle(context.MountedImage))
            {
                throw new Win32Exception();
            }
            if (!Windows.Imaging.WimCloseHandle(context.WimHandle))
            {
                throw new Win32Exception();
            }

            return new TaskResult();
        }

        public static TaskResult SetBootWimBootableIndex(TaskContext context)
        {
            ClearAndWriteHeader("Configuring Panther2K image as boot image...");

            if (!Directory.Exists("./ISO/sources"))
            {
                throw new DirectoryNotFoundException("ISO/sources directory does not exist.");
            }

            uint result = 0;
            IntPtr hWim = Windows.Imaging.WimCreateFile("./ISO/sources/boot.wim", Windows.Imaging.DesiredAccess.GenericWrite | Windows.Imaging.DesiredAccess.GenericExecute, Windows.Imaging.CreationDisposition.OpenExisting,
                Windows.Imaging.FlagsAndAttributes.None, Windows.Imaging.CompressionType.Maximum, ref result);
            if (hWim == IntPtr.Zero) throw new Win32Exception();

            if (!Windows.Imaging.WimSetTemporaryPath(hWim, "./WimgapiTemp"))
                throw new Win32Exception();

            Console.WriteLine("Setting bootable index for boot.wim to 1.");
            if (!Windows.Imaging.WimSetBootImage(hWim, 1))
                throw new Win32Exception();

            if (!Windows.Imaging.WimCloseHandle(hWim))
                throw new Win32Exception();

            return new TaskResult();
        }

        public static TaskResult ESDCreateInstallWim(TaskContext context)
        {
            ClearAndWriteHeader("Creating installation image from ESD...");

            if (!File.Exists(context.ImageFile))
            {
                throw new FileNotFoundException("Selected image file does not exist.", context.ImageFile);
            }
            if (!Directory.Exists("./WimgapiTemp"))
            {
                throw new DirectoryNotFoundException("Temp directory for Wimgapi does not exist.");
            }
            if (!Directory.Exists("./ISO/sources"))
            {
                throw new DirectoryNotFoundException("ISO/sources directory does not exist.");
            }

            uint result = 0;
            IntPtr hSrcFile = Windows.Imaging.WimCreateFile(context.ImageFile, Windows.Imaging.DesiredAccess.GenericRead, Windows.Imaging.CreationDisposition.OpenExisting,
                Windows.Imaging.FlagsAndAttributes.SolidCompression, Windows.Imaging.CompressionType.Recovery, ref result);
            if (hSrcFile == IntPtr.Zero) throw new Win32Exception();

            if (!Windows.Imaging.WimSetTemporaryPath(hSrcFile, "./WimgapiTemp"))
                throw new Win32Exception();

            IntPtr hTargetFile;
            if (context.ReplaceWimWithEsd) hTargetFile = Windows.Imaging.WimCreateFile("./ISO/sources/install.esd", Windows.Imaging.DesiredAccess.GenericWrite, Windows.Imaging.CreationDisposition.CreateAlways,
                Windows.Imaging.FlagsAndAttributes.SolidCompression, Windows.Imaging.CompressionType.Recovery, ref result);
            else hTargetFile = Windows.Imaging.WimCreateFile("./ISO/sources/install.wim", Windows.Imaging.DesiredAccess.GenericWrite, Windows.Imaging.CreationDisposition.CreateAlways,
                Windows.Imaging.FlagsAndAttributes.None, Windows.Imaging.CompressionType.Maximum, ref result);
            if (hTargetFile == IntPtr.Zero) throw new Win32Exception();

            Windows.Imaging.MessageCallback callback = (Windows.Imaging.MessageId dwMessageId, IntPtr wParam, IntPtr lParam, IntPtr pvUserData) =>
            {
                if (dwMessageId == Windows.Imaging.MessageId.Progress)
                {
                    // Progress bar
                    double percent = (wParam.ToInt32() / 100.0);
                    int barWidth = 40;
                    int filled = (int)(percent * barWidth);
                    string bar = new string('=', filled) + new string(' ', barWidth - filled);
                    string progress = $"\r[{bar}] {percent * 100,6:##0.00}% (ETA: {new TimeSpan(0, 0, 0, 0, lParam.ToInt32()):g})".PadRight(Console.BufferWidth - 1);
                    Console.Write(progress);
                }

                return Windows.Imaging.CallbackResult.Success;
            };
            if (Windows.Imaging.WimRegisterMessageCallback(hTargetFile, callback, IntPtr.Zero) == 0xFFFFFFFF)
                throw new Win32Exception();

            if (!Windows.Imaging.WimSetTemporaryPath(hTargetFile, "./WimgapiTemp"))
                throw new Win32Exception();

            // Index 1-3 are for setup
            // 4+ contain the install.wim files
            uint count = Windows.Imaging.WimGetImageCount(hSrcFile);
            if (count < 4)
            {
                throw new InvalidOperationException("The selected ESD does not contain an install.wim image.");
            }

            Console.WriteLine(context.ReplaceWimWithEsd ? "Copying Windows to install.esd" : "Copying Windows to install.wim.");
            
            for (uint i = 4; i <= count; i++)
            {
                IntPtr hSrcImage = Windows.Imaging.WimLoadImage(hSrcFile, i);
                if (hSrcImage == IntPtr.Zero) throw new Win32Exception();

                Console.WriteLine(context.ReplaceWimWithEsd ? $"Copying image {i} to install.esd." : $"Copying image {i} to install.wim.");
                if (!Windows.Imaging.WimExportImage(hSrcImage, hTargetFile, Windows.Imaging.ExportFlags.None))
                    throw new Win32Exception();
                Console.WriteLine();

                if (!Windows.Imaging.WimCloseHandle(hSrcImage))
                    throw new Win32Exception();
            }

            if (!Windows.Imaging.WimCloseHandle(hTargetFile))
                throw new Win32Exception();

            if (!Windows.Imaging.WimCloseHandle(hSrcFile))
                throw new Win32Exception();

            return new TaskResult();
        }


        public static TaskResult CopyBootBinaries(TaskContext context)
        {
            ClearAndWriteHeader("Copying boot sector files from ISO root...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }
            if (!File.Exists("./ISO/boot/etfsboot.com"))
            {
                throw new FileNotFoundException("etfsboot.com does not exist.", "./ISO/boot/etfsboot.com");
            }
            if (!File.Exists("./ISO/efi/microsoft/boot/efisys.bin"))
            {
                throw new FileNotFoundException("efisys.bin does not exist.", "./ISO/efi/microsoft/boot/efisys.bin");
            }

            File.Copy("./ISO/boot/etfsboot.com", "./etfsboot.com", true);
            File.Copy("./ISO/efi/microsoft/boot/efisys.bin", "./efisys.bin", true);

            context.WorkingFiles.Add("./etfsboot.com");
            context.WorkingFiles.Add("./efisys.bin");

            return new TaskResult();
        }

        public static TaskResult BuildISO(TaskContext context)
        {
            ClearAndWriteHeader("Building ISO image from generated files...");

            if (!Directory.Exists("./ISO"))
            {
                throw new DirectoryNotFoundException("ISO directory does not exist.");
            }
            if (!File.Exists("./etfsboot.com"))
            {
                throw new FileNotFoundException("etfsboot.com does not exist.", "./etfsboot.com");
            }
            if (!File.Exists("./efisys.bin"))
            {
                throw new FileNotFoundException("efisys.bin does not exist.", "./efisys.bin");
            }

            Console.WriteLine("Building ISO image...");
            //Process oscdimg = Process.Start("./resources/oscdimg.exe", "-m -o -u2 -udfver102 -bootdata:2#p0,e,betfsboot.com#pEF,e,befisys.bin ISO Panther.iso");
            Process oscdimg = Process.Start(new ProcessStartInfo()
            {
                FileName = "./resources/oscdimg.exe",
                Arguments = "-m -o -u2 -udfver102 -bootdata:2#p0,e,betfsboot.com#pEF,e,befisys.bin ISO Panther.iso",
                UseShellExecute = false,
            });
            oscdimg.WaitForExit();
            if (oscdimg.ExitCode != 0)
            {
                throw new InvalidOperationException("Failed to build ISO image. Exit code: " + oscdimg.ExitCode);
            }

            return new TaskResult();
        }

        static void Main(string[] args)
        {
            // Check if current directory is on NTFS volume
            DriveInfo currentDrive = new DriveInfo(Path.GetPathRoot(Environment.CurrentDirectory));
            if (currentDrive.DriveFormat != "NTFS")
            {
                ClearAndWriteHeader("You must run pantherScripts from on an NTFS partition. Press any key to exit.");
                Console.ReadKey();
                return;
            }

            Windows.Imaging.WimRegisterLogFile(Path.GetFullPath("./wimgapi.log"));

            // Helper: Find files by extension (case-insensitive)
            string GetFile(string ext) => args.FirstOrDefault(f => f.EndsWith(ext, StringComparison.OrdinalIgnoreCase));
            string pantherZip = GetFile(".zip");
            string esdFile = GetFile(".esd");
            string isoFile = GetFile(".iso");

            // 1. Require at least the Panther2K zip file
            if (string.IsNullOrEmpty(pantherZip) || !File.Exists(pantherZip))
            {
                ClearAndWriteHeader("Please drag and drop the Panther2K zip file (and optionally an ESD or ISO) onto this program. Press any key to exit.");
                Console.ReadKey();
                return;
            }

            // 2. Determine pipeline
            TaskDelegate[] tasks = null;
            var context = new TaskContext();
            context.PantherFile = pantherZip;

            if (!string.IsNullOrEmpty(isoFile) && File.Exists(isoFile))
            {
                // ISO pipeline
                context.ImageFile = isoFile;
                tasks = new TaskDelegate[]
                {
                    DisplayOverview,
                    CreateWorkingDirectories,
                    ISOExtract,
                    ISOTempMoveInstallWim,
                    CopyAutorun,
                    RemoveSetupFromRoot,
                    AddPantherToRoot,
                    CreateSourcesFolder,
                    ISOMoveBackImagesToSources,
                    SetBootWimBootableIndex,
                    CreateWimMountFolder,
                    MountBootWim,
                    AddPanther2KToWimMount,
                    ConfigureMountedPEInstRoot,
                    SaveBootWim,
                    CopyBootBinaries,
                    BuildISO,
                };
            }
            else if (!string.IsNullOrEmpty(esdFile) && File.Exists(esdFile))
            {
                // ESD pipeline, skip download
                context.ImageFile = esdFile;
                tasks = new TaskDelegate[]
                {
                    DisplayOverview,
                    CreateWorkingDirectories,
                    ESDExtractSetupRoot,
                    CopyAutorun,
                    RemoveSetupFromRoot,
                    AddPantherToRoot,
                    CreateSourcesFolder,
                    ESDCreateBootWim,
                    SetBootWimBootableIndex,
                    CreateWimMountFolder,
                    MountBootWim,
                    AddPanther2KToWimMount,
                    ConfigureMountedPEInstRoot,
                    SaveBootWim,
                    ESDCreateInstallWim,
                    CopyBootBinaries,
                    BuildISO,
                };
            }
            else if (args.Length == 1 && pantherZip != null)
            {
                // ESD pipeline, with download
                tasks = new TaskDelegate[]
                {
                    RetrieveLatestESDCatalog,
                    SelectESDFromCatalog,
                    DownloadSelectedEsd,
                    DisplayOverview,
                    CreateWorkingDirectories,
                    ESDExtractSetupRoot,
                    CopyAutorun,
                    RemoveSetupFromRoot,
                    AddPantherToRoot,
                    CreateSourcesFolder,
                    ESDCreateBootWim,
                    SetBootWimBootableIndex,
                    CreateWimMountFolder,
                    MountBootWim,
                    AddPanther2KToWimMount,
                    ConfigureMountedPEInstRoot,
                    SaveBootWim,
                    ESDCreateInstallWim,
                    CopyBootBinaries,
                    BuildISO,
                };
            }
            else
            {
                ClearAndWriteHeader("Invalid combination of files. Please drag and drop the Panther2K zip file and optionally an ESD or ISO file. Press any key to exit.");
                Console.ReadKey();
                return;
            }

            ClearAndWriteHeader("Gathering information.");

            Console.WriteLine("Selected files:");
            Console.WriteLine($"Panther2K ZIP: {pantherZip}");
            if (esdFile != null) Console.WriteLine($"ESD file: {esdFile}");
            else if (isoFile != null) Console.WriteLine($"ISO file: {isoFile}");
            else Console.WriteLine("No ESD or ISO file provided, an installation image will be fetched from Media Creation Tool servers.");
            Console.WriteLine();

            // 3. Query user for installation image inclusion
            Console.WriteLine("How should the installation image be included?");
            Console.WriteLine("1. Do not include the installation image");
            if (tasks.Any(t => t == (TaskDelegate)ESDCreateInstallWim))
                Console.WriteLine("2. Include install.esd (recommended for ESD pipeline)");
            if (tasks.Any(t => t == (TaskDelegate)ISOTempMoveInstallWim))
            {
                Console.WriteLine("2. Include install.wim (ISO only)");
                Console.WriteLine("3. Convert install.wim to install.esd (ISO only)");
            }

            while (true)
            {
                Console.Write(tasks.Any(t => t == (TaskDelegate)ISOTempMoveInstallWim) ?
                    "Enter your choice (1/2/3): "
                    : "Enter your choice (1/2): ");
                string choice = Console.ReadLine();

                if (choice == "1")
                {
                    context.IncludeInstallImage = false;
                    context.ReplaceWimWithEsd = false;
                    break;
                }
                else if (choice == "2")
                {
                    context.IncludeInstallImage = true;
                    context.ReplaceWimWithEsd = tasks.Any(t => t == (TaskDelegate)ESDCreateInstallWim);
                    break;
                }
                else if (choice == "3" && tasks.Any(t => t == (TaskDelegate)ISOTempMoveInstallWim))
                {
                    context.IncludeInstallImage = true;
                    context.ReplaceWimWithEsd = true; 
                    break;
                }
                else
                {
                    Console.WriteLine("Invalid choice. Please enter a valid option.");
                }
            }

            // 4. Run pipeline
            foreach (TaskDelegate func in tasks)
            {
#if DEBUG
                func(context);
#else
                try
                {
                    func(context);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Error: {ex.Message}");
                    return;
                }
#endif
            }

            foreach (string file in context.WorkingFiles)
            {
                if (Directory.Exists(file))
                    Directory.Delete(file, true);
                else if (File.Exists(file))
                    File.Delete(file);
            }
            ClearAndWriteHeader("All tasks completed successfully!\r\nOutput: Panther2K.iso");
        }
    }
}
