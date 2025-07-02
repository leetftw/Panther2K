using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Windows
{
    internal static class Registry
    {
        [DllImport("advapi32.dll", EntryPoint = "RegLoadAppKey", SetLastError = true)]
        public static extern int LoadAppKey(
        string lpFile,
        out IntPtr phkResult,
        uint samDesired,
        uint dwOptions,
        uint Reserved);

        [DllImport("advapi32.dll", EntryPoint = "RegFlushKey", SetLastError = true)]
        public static extern int FlushKey(IntPtr hKey);

        [DllImport("advapi32.dll", EntryPoint = "RegCloseKey", SetLastError = true)]
        public static extern int CloseKey(IntPtr hKey);
    }
}
