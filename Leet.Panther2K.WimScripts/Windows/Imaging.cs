using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace Windows
{
    internal static class Imaging
    {
        public enum CreationDisposition : uint
        {
            CreateNew = 1,
            CreateAlways = 2,
            OpenExisting = 3,
            OpenAlways = 4,
        }

        public enum DesiredAccess : uint
        {
            GenericRead = 0x80000000,
            GenericWrite = 0x40000000,
            GenericExecute = 0x20000000,
        }

        public enum CompressionType : uint
        {
            /// <summary>
            /// Does not compress the WIM file.
            /// </summary>
            None = 0,
            /// <summary>
            /// Compresses the WIM file using 'fast' (Xpress) compression.
            /// </summary>
            Xpress = 1,
            /// <summary>
            /// Compresses the WIM file using 'fast' (Xpress) compression.
            /// </summary>
            Fast = 1,
            /// <summary>
            /// Compresses the WIM file using 'maximum' (LZX) compression.
            /// </summary>
            Lzx = 2,
            /// <summary>
            /// Compresses the WIM file using 'maximum' (LZX) compression.
            /// </summary>
            Maximum = 2,
            /// <summary>
            /// Compresses the WIM file using 'recovery' (LZMS) compression.
            /// </summary>
            /// <remarks>
            /// This on its own does not generate an ESD file. 
            /// To do that, you need to specify the solid compression flag and use this compression type while creating the WIM file.
            /// </remarks>
            Lzms = 3,
            /// <summary>
            /// Compresses the WIM file using 'recovery' (LZMS) compression.
            /// </summary>
            /// <remarks>
            /// This on its own does not generate an ESD file. 
            /// To do that, you need to specify the solid compression flag and use this compression type while creating the WIM file.
            /// </remarks>
            Recovery = 3,
        }

        /*
#define WIM_FLAG_APPLY_CI_EA               0x00001000
#define WIM_FLAG_WIM_BOOT                  0x00002000
#define WIM_FLAG_APPLY_COMPACT             0x00004000
#define WIM_FLAG_SUPPORT_EA                0x00008000 // It can be used in mount also.
        */
        public enum FlagsAndAttributes : uint
        { 
            None = 0,
            Reserved = 0x1,
            /// <summary>
            /// Generates data integrity information for new files. Verifies and updates existing files
            /// </summary>
            /// <remarks>
            /// Applicable to: WIMApplyImage, WIMCaptureImage, WIMCommitImageHandle, WIMCopyFile, WIMCreateFile, WIMMountImageHandle, WIMSetReferenceFile
            /// </remarks>
            Verify = 0x2,
            /// <summary>
            /// Specifies that the image is to be sequentially read for caching or performance purposes.
            /// </summary>
            Index = 0x4,
            /// <summary>
            /// Applies the image without physically creating directories or files. Useful for obtaining a list of files and directories in the image.
            /// </summary>
            NoApply = 0x8,
            /// <summary>
            /// Disables restoring security information for directories.
            /// </summary>
            NoDirAcl = 0x10,
            /// <summary>
            /// Disables restoring security information for files.
            /// </summary>
            NoFileAcl = 0x20,
            /// <summary>
            /// Opens the .wim file in a mode that enables simultaneous reading and writing.
            /// </summary>
            ShareWrite = 0x40,
            /// <summary>
            /// Sends a WIM_MSG_FILEINFO message during the apply operation.
            /// </summary>
            FileInfo = 0x80,
            /// <summary>
            /// Disables automatic path fixups for junctions and symbolic links.
            /// </summary>
            NoRpFix = 0x100,
            /// <summary>
            /// Represents a flag indicating that solid compression is enabled.
            /// </summary>
            /// <remarks>
            /// Solid compression processes files as a single continuous stream, which can
            /// improve compression ratios for similar files but may increase memory usage and reduce random access
            /// performance.
            /// </remarks>
            SolidCompression = 0x20000000,
            MountReadOnly = 0x00000200,
            MountFast = 0x00000400,
            MountLegacy = 0x00000800,
        }

        public enum ExportFlags : uint
        {
            None = 0x0,
            AllowDuplicates = 0x1,
            OnlyResources = 0x2,
            OnlyMetadata = 0x4,
            VerifySource = 0x8,
            VerifyDestination = 0x10,
        }

        public enum MessageId : uint
        {
            Base = 0x8000 + 0x1476, // WM_APP + 0x1476
            Text,
            Progress,
            Process,
            Scanning,
            SetRange,
            SetPos,
            StepIt,
            Compress,
            Error,
            Alignment,
            Retry,
            Split,
            FileInfo,
            Info,
            Warning,
            ChkProcess,
            WarningObjectId,
            StaleMountDir,
            StaleMountFile,
            MountCleanupProgress,
            CleanupScanningDrive,
            ImageAlreadyMounted,
            CleanupUnmountingImage,
            QueryAbort,
            IoRangeStartRequestLoop,
            IoRangeEndRequestLoop,
            IoRangeRequest,
            IoRangeRelease,
            VerifyProgress,
            CopyBuffer,
            MetadataExclude,
            GetApplyRoot,
            MdPad,
            StepName,
            PerfileCompress,
            CheckCiEaPrerequisiteNotMet,
            JournalingEnabled,
        }

        public enum CallbackResult : uint
        {
            Success = 0x0, // ERROR_SUCCESS
            Done = 0xFFFFFFF0, // WIM_MSG_DONE
            SkipError = 0xFFFFFFFE, // WIM_MSG_SKIP_ERROR
            AbortImage = 0xFFFFFFFF, // WIM_MSG_ABORT_IMAGE
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public delegate CallbackResult MessageCallback(MessageId dwMessageId, IntPtr wParam, IntPtr lParam, IntPtr pvUserData);

        [DllImport("wimgapi.dll", EntryPoint = "WIMCreateFile", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr WimCreateFile(string pszWimPath, DesiredAccess dwDesiredAccess, CreationDisposition dwCreationDisposition, FlagsAndAttributes dwFlagsAndAttributes, CompressionType dwCompressionType, ref uint creationResult);

        [DllImport("wimgapi.dll", EntryPoint = "WIMLoadImage", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern IntPtr WimLoadImage(IntPtr hWim, uint dwImageIndex);

        [DllImport("wimgapi.dll", EntryPoint = "WIMApplyImage", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimApplyImage(IntPtr hImage, string pszPath, FlagsAndAttributes dwApplyFlags);

        [DllImport("wimgapi.dll", EntryPoint = "WIMSetTemporaryPath", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimSetTemporaryPath(IntPtr hWim, string pszPath);

        [DllImport("wimgapi.dll", EntryPoint = "WIMExportImage", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimExportImage(IntPtr hImage, IntPtr hWim, ExportFlags dwFlags);

        [DllImport("wimgapi.dll", EntryPoint = "WIMCloseHandle", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimCloseHandle(IntPtr hObject);

        [DllImport("wimgapi.dll", EntryPoint = "WIMRegisterMessageCallback", SetLastError = true)]
        public static extern uint WimRegisterMessageCallback(IntPtr hWim, MessageCallback fpMessageProc, IntPtr pvUserData);

        [DllImport("wimgapi.dll", EntryPoint = "WIMMountImageHandle", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimMountImageHandle(IntPtr hImage, string pszMountPath, FlagsAndAttributes dwMountFlags);

        [DllImport("wimgapi.dll", EntryPoint = "WIMCommitImageHandle", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimCommitImageHandle(IntPtr hImage, FlagsAndAttributes dwCommitFlags, ref IntPtr phNewImageHandle);

        [DllImport("wimgapi.dll", EntryPoint = "WIMUnmountImageHandle", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimUnmountImageHandle(IntPtr hImage, uint dwUnmountFlags = 0);

        [DllImport("wimgapi.dll", EntryPoint = "WIMGetImageCount", SetLastError = true)]
        public static extern uint WimGetImageCount(IntPtr hWim);

        [DllImport("wimgapi.dll", EntryPoint = "WIMSetBootImage", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimSetBootImage(IntPtr hWim, uint dwImageIndex);

        //        BOOL
        //WINAPI
        //WIMRegisterLogFile(
        //PCWSTR pszLogFile,
        //DWORD dwFlags
        //);
        [DllImport("wimgapi.dll", EntryPoint = "WIMRegisterLogFile", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool WimRegisterLogFile(string pszLogFile, uint dwFlags = 0);
    }
}
