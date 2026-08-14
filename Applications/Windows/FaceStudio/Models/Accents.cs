using System;

using FaceStudio.Interop;

using Microsoft.UI.Xaml.Media;

using Windows.UI;

namespace FaceStudio.Models;

/// <summary>
/// The per-user accent colours, read once from the engine.
///
/// The palette lives in the pipeline's model - the overlay, the head and these panels all
/// draw from it - so this class carries no colours of its own beyond a fallback for when
/// the engine's DLL is not there, such as in the designer. The brushes are cached: a brush
/// per binding read made the inspector allocate twenty times a second.
/// </summary>
public static class Accents
{
    private static readonly SolidColorBrush[] Brushes = Load();

    /// <summary>The accent of one user, stable frame to frame.</summary>
    public static SolidColorBrush BrushOf(int iUserId)
    {
        return Brushes[Math.Abs(iUserId % Brushes.Length)];
    }

    private static SolidColorBrush[] Load()
    {
        try
        {
            int count = NativeEngine.FeEngine_GetAccentCount();

            if (count > 0)
            {
                var brushes = new SolidColorBrush[count];

                for (int i = 0; i < count; i++)
                {
                    NativeEngine.FeEngine_GetAccentColor(i, out double r, out double g, out double b);

                    brushes[i] = new SolidColorBrush(Color.FromArgb(
                        0xFF, (byte)Math.Round(r * 255.0), (byte)Math.Round(g * 255.0), (byte)Math.Round(b * 255.0)));
                }

                return brushes;
            }
        }
        catch (DllNotFoundException)
        {
            // The designer and the tests have no engine to ask; the fallback below matches it
        }
        catch (EntryPointNotFoundException)
        {
        }

        return new[]
        {
            new SolidColorBrush(Color.FromArgb(0xFF, 0x4D, 0xC7, 0xFF)),
            new SolidColorBrush(Color.FromArgb(0xFF, 0x8C, 0xF0, 0x99)),
            new SolidColorBrush(Color.FromArgb(0xFF, 0xFF, 0xBA, 0x59)),
            new SolidColorBrush(Color.FromArgb(0xFF, 0xF0, 0x8C, 0xCC)),
            new SolidColorBrush(Color.FromArgb(0xFF, 0xFF, 0x78, 0x78))
        };
    }
}
