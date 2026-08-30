using System;
using System.Runtime.InteropServices;

using Microsoft.UI.Xaml.Controls;

namespace FaceStudio.Interop;

/// <summary>
/// Hands a native swap chain to a XAML <see cref="SwapChainPanel"/>.
///
/// The panel exposes this through ISwapChainPanelNative, a COM interface with no projection,
/// so it is called through its vtable directly. That is a little more code than declaring a
/// ComImport interface, and in exchange it does not depend on how the runtime happens to
/// marshal one - which is the sort of thing that works until a runtime upgrade.
/// </summary>
internal static class SwapChainBinder
{
    /// <summary>ISwapChainPanelNative, as declared in microsoft.ui.xaml.media.dxinterop.h.</summary>
    private static readonly Guid SwapChainPanelNativeIid = new("63aad0b8-7c24-40ff-85a8-640d944cc325");

    /// <summary>
    /// Binds <paramref name="iSwapChain"/> to <paramref name="iPanel"/>, or unbinds when it
    /// is <see cref="IntPtr.Zero"/>. Must be called on the UI thread the panel lives on.
    /// </summary>
    public static unsafe void Attach(SwapChainPanel iPanel, IntPtr iSwapChain)
    {
        ArgumentNullException.ThrowIfNull(iPanel);

        IntPtr inspectable = WinRT.MarshalInspectable<object>.FromManaged(iPanel);

        if (inspectable == IntPtr.Zero)
        {
            throw new InvalidOperationException("The panel has no native object behind it.");
        }

        try
        {
            Guid iid = SwapChainPanelNativeIid;

            int hr = Marshal.QueryInterface(inspectable, in iid, out IntPtr native);
            Marshal.ThrowExceptionForHR(hr);

            try
            {
                // IUnknown takes the first three slots, so SetSwapChain is the fourth
                void** vtable = *(void***)native;
                var setSwapChain = (delegate* unmanaged[Stdcall]<IntPtr, IntPtr, int>)vtable[3];

                Marshal.ThrowExceptionForHR(setSwapChain(native, iSwapChain));
            }
            finally
            {
                Marshal.Release(native);
            }
        }
        finally
        {
            Marshal.Release(inspectable);
        }
    }
}
