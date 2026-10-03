// ============================================================================
//  WinAudio.cs - minimal MMDevice + IPolicyConfig interop for PowerShell
//  Lets the setup scripts enumerate endpoints (with state) and set the
//  default audio device (needed to route Windows audio into VB-Cable).
// ============================================================================
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public static class WinAudio
{
    const int eRender = 0, eCapture = 1;
    const int DEVICE_STATE_ACTIVE     = 0x1;
    const int DEVICE_STATE_DISABLED   = 0x2;
    const int DEVICE_STATE_NOTPRESENT = 0x4;
    const int DEVICE_STATE_UNPLUGGED  = 0x8;
    const int DEVICE_STATEMASK_ALL    = 0xF;

    [ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")]
    class MMDeviceEnumeratorComObject { }

    [ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IMMDeviceEnumerator
    {
        int EnumAudioEndpoints(int dataFlow, int stateMask, out IMMDeviceCollection devices);
        int GetDefaultAudioEndpoint(int dataFlow, int role, out IMMDevice endpoint);
        int GetDevice([MarshalAs(UnmanagedType.LPWStr)] string id, out IMMDevice device);
        int RegisterEndpointNotificationCallback(IntPtr client);
        int UnregisterEndpointNotificationCallback(IntPtr client);
    }

    [ComImport, Guid("0BD7A1BE-7A1A-44DB-8397-CC5392387B5E"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IMMDeviceCollection
    {
        int GetCount(out int count);
        int Item(int index, out IMMDevice device);
    }

    [ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IMMDevice
    {
        int Activate([MarshalAs(UnmanagedType.LPStruct)] Guid iid, int clsCtx,
                     IntPtr activationParams, [MarshalAs(UnmanagedType.IUnknown)] out object iface);
        int OpenPropertyStore(int stgmAccess, out IntPtr props);
        int GetId([MarshalAs(UnmanagedType.LPWStr)] out string id);
        int GetState(out int state);
    }

    [ComImport, Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IPropertyStore { }

    [ComImport, Guid("F8679F50-850A-41CF-9C72-430F290290C8"),
     InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IPolicyConfig
    {
        int GetMixFormat(string a, IntPtr b);
        int GetDeviceFormat(string a, bool b, IntPtr c);
        int ResetDeviceFormat(string a);
        int SetDeviceFormat(string a, IntPtr b, IntPtr c);
        int GetProcessingPeriod(string a, bool b, IntPtr c, IntPtr d);
        int SetProcessingPeriod(string a, IntPtr b);
        int GetShareMode(string a, IntPtr b);
        int SetShareMode(string a, IntPtr b);
        int GetPropertyValue(string a, bool b, IntPtr key, IntPtr value);
        int SetPropertyValue(string a, bool b, IntPtr key, IntPtr value);
        int SetDefaultEndpoint([MarshalAs(UnmanagedType.LPWStr)] string deviceId, int role);
        int SetEndpointVisibility(string a, bool b);
    }

    static string StateText (int s)
    {
        if ((s & DEVICE_STATE_ACTIVE)     != 0) return "ACTIVE";
        if ((s & DEVICE_STATE_DISABLED)   != 0) return "DISABLED";
        if ((s & DEVICE_STATE_UNPLUGGED)  != 0) return "UNPLUGGED";
        if ((s & DEVICE_STATE_NOTPRESENT) != 0) return "NOT-PRESENT";
        return "UNKNOWN(" + s + ")";
    }

    // lines: "PLAY|<STATE>|<id>|<friendly>"   (or "REC|...")
    public static string ListDevices()
    {
        var result = new List<string>();
        var enumerator = (IMMDeviceEnumerator)new MMDeviceEnumeratorComObject();
        foreach (int flow in new[] { eRender, eCapture })
        {
            IMMDeviceCollection coll;
            int hr = enumerator.EnumAudioEndpoints(flow, DEVICE_STATEMASK_ALL, out coll);
            if (hr != 0 || coll == null) continue;
            int count;
            coll.GetCount(out count);
            for (int i = 0; i < count; i++)
            {
                IMMDevice dev;
                coll.Item(i, out dev);
                string id = "?";
                int state = 0;
                try { dev.GetId(out id); } catch { }
                try { dev.GetState(out state); } catch { }
                string friendly = GetName(dev);
                result.Add((flow == eRender ? "PLAY|" : "REC|") + StateText(state) + "|" + id + "|" + friendly);
            }
        }
        return string.Join("\n", result.ToArray());
    }

    // anyState=false -> only ACTIVE endpoints; true -> every endpoint
    public static string FindDeviceId(string nameFragment, bool playback, bool anyState)
    {
        var lines = ListDevices().Split('\n');
        string tag = playback ? "PLAY|" : "REC|";
        foreach (var line in lines)
        {
            if (!line.StartsWith(tag)) continue;
            var parts = line.Substring(tag.Length).Split(new[] { '|' }, 3);
            if (parts.Length < 3) continue;
            string state = parts[0], rest = parts[1] + "|" + parts[2];
            if (!anyState && state != "ACTIVE") continue;
            if (rest.IndexOf(nameFragment, StringComparison.OrdinalIgnoreCase) >= 0)
                return parts[1];
        }
        return "";
    }

    public static void SetDefaultDevice(string nameFragment, bool playback, int role)
    {
        string id = FindDeviceId(nameFragment, playback, false);
        if (string.IsNullOrEmpty(id))
            throw new Exception("Audio endpoint containing '" + nameFragment + "' not found (ACTIVE).");

        Type t = Type.GetTypeFromCLSID(new Guid("BCDE0395-E52F-467C-8E3D-C4579291692E"));
        var com = Activator.CreateInstance(t);
        var config = (IPolicyConfig)com;
        // role: 0 = console, 1 = multimedia, 2 = communications  (set all three)
        if (role < 0) { config.SetDefaultEndpoint(id, 0); config.SetDefaultEndpoint(id, 1); config.SetDefaultEndpoint(id, 2); }
        else          config.SetDefaultEndpoint(id, role);
        Marshal.ReleaseComObject(com);
    }

    static string GetName(IMMDevice dev)
    {
        string id;
        try { dev.GetId(out id); } catch { return "?"; }
        try
        {
            using (var key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(
                @"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render"))
            {
                if (key != null)
                {
                    foreach (var guidSub in key.GetSubKeyNames())
                    {
                        using (var devKey = key.OpenSubKey(guidSub))
                        {
                            if (devKey == null) continue;
                            var devId = devKey.GetValue("DeviceID") as string;
                            if (devId != null && id.IndexOf(devId, StringComparison.OrdinalIgnoreCase) >= 0)
                            {
                                using (var fn = devKey.OpenSubKey("Properties"))
                                {
                                    var name = fn.GetValue("{a45c254e-df1c-4efd-8020-67d146a850e0},14") as string;
                                    if (!string.IsNullOrEmpty(name)) return name;
                                }
                            }
                        }
                    }
                }
            }
        }
        catch { }
        return id;
    }
}
