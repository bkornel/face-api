using System;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace FaceStudio.Controls;

/// <summary>How a meter reads its value.</summary>
public enum MeterMode
{
    /// <summary>Fills from the left. For a quantity that has a floor, such as openness.</summary>
    Fill,

    /// <summary>Grows out of the middle. For a quantity with a sign, such as an angle.</summary>
    Centered
}

/// <summary>
/// A thin horizontal bar for one live number.
///
/// Built in code rather than from a template: it is two rectangles and an arithmetic, and a
/// control template would be more of this file than the control is. A centred meter is what
/// makes a head angle readable at a glance - which way it turned is the shape of the bar,
/// before the number has been read at all.
///
/// The parts live on a Canvas, which does not measure its children. A bar whose width comes
/// from the width it was given would otherwise feed its own size back into the layout, and
/// XAML gives up on that after a few passes.
/// </summary>
public sealed class Meter : UserControl
{
    public static readonly DependencyProperty ValueProperty = DependencyProperty.Register(
        nameof(Value), typeof(double), typeof(Meter), new PropertyMetadata(0.0, OnVisualPropertyChanged));

    public static readonly DependencyProperty MinimumProperty = DependencyProperty.Register(
        nameof(Minimum), typeof(double), typeof(Meter), new PropertyMetadata(0.0, OnVisualPropertyChanged));

    public static readonly DependencyProperty MaximumProperty = DependencyProperty.Register(
        nameof(Maximum), typeof(double), typeof(Meter), new PropertyMetadata(1.0, OnVisualPropertyChanged));

    public static readonly DependencyProperty ModeProperty = DependencyProperty.Register(
        nameof(Mode), typeof(MeterMode), typeof(Meter), new PropertyMetadata(MeterMode.Fill, OnVisualPropertyChanged));

    public static readonly DependencyProperty AccentProperty = DependencyProperty.Register(
        nameof(Accent), typeof(Brush), typeof(Meter), new PropertyMetadata(null, OnAccentChanged));

    private readonly Canvas _root;
    private readonly Border _track;
    private readonly Border _fill;
    private readonly Border _centreMark;

    public Meter()
    {
        IsTabStop = false;
        Height = 6.0;
        HorizontalAlignment = HorizontalAlignment.Stretch;
        VerticalAlignment = VerticalAlignment.Center;

        _track = new Border();
        _fill = new Border();
        _centreMark = new Border { Width = 1.0, Opacity = 0.0 };

        _root = new Canvas();
        _root.Children.Add(_track);
        _root.Children.Add(_fill);
        _root.Children.Add(_centreMark);

        Content = _root;

        Loaded += (_, _) => ApplyThemeBrushes();
        SizeChanged += (_, _) => Refresh();
        ActualThemeChanged += (_, _) => ApplyThemeBrushes();
    }

    public double Value
    {
        get => (double)GetValue(ValueProperty);
        set => SetValue(ValueProperty, value);
    }

    public double Minimum
    {
        get => (double)GetValue(MinimumProperty);
        set => SetValue(MinimumProperty, value);
    }

    public double Maximum
    {
        get => (double)GetValue(MaximumProperty);
        set => SetValue(MaximumProperty, value);
    }

    public MeterMode Mode
    {
        get => (MeterMode)GetValue(ModeProperty);
        set => SetValue(ModeProperty, value);
    }

    public Brush? Accent
    {
        get => (Brush?)GetValue(AccentProperty);
        set => SetValue(AccentProperty, value);
    }

    private static void OnVisualPropertyChanged(DependencyObject iSender, DependencyPropertyChangedEventArgs iArgs)
    {
        ((Meter)iSender).Refresh();
    }

    private static void OnAccentChanged(DependencyObject iSender, DependencyPropertyChangedEventArgs iArgs)
    {
        ((Meter)iSender).ApplyThemeBrushes();
    }

    private void ApplyThemeBrushes()
    {
        if (Application.Current.Resources.TryGetValue("MeterTrackBrush", out object? track) && track is Brush trackBrush)
        {
            _track.Background = trackBrush;
            _centreMark.Background = trackBrush;
        }

        _fill.Background = Accent
                           ?? (Application.Current.Resources.TryGetValue("AccentBrush", out object? accent) && accent is Brush accentBrush
                               ? accentBrush
                               : null);

        Refresh();
    }

    private void Refresh()
    {
        double width = _root.ActualWidth;
        double height = _root.ActualHeight;

        if (width <= 0.0 || height <= 0.0) return;

        double radius = height / 2.0;

        _track.Width = width;
        _track.Height = height;
        _track.CornerRadius = new CornerRadius(radius);

        _fill.Height = height;
        _fill.CornerRadius = new CornerRadius(radius);

        _centreMark.Height = height;
        Canvas.SetLeft(_centreMark, width / 2.0);

        double span = Maximum - Minimum;
        double normalised = span > 1e-9 ? Math.Clamp((Value - Minimum) / span, 0.0, 1.0) : 0.0;

        if (Mode == MeterMode.Fill)
        {
            _centreMark.Opacity = 0.0;

            Canvas.SetLeft(_fill, 0.0);
            _fill.Width = Math.Max(0.0, normalised * width);

            return;
        }

        // Centred: the middle of the range sits in the middle of the track, and the bar
        // grows towards whichever side the value is on
        _centreMark.Opacity = 0.6;

        double half = width / 2.0;
        double offset = (normalised - 0.5) * width;

        Canvas.SetLeft(_fill, offset >= 0.0 ? half : half + offset);
        _fill.Width = Math.Max(1.0, Math.Abs(offset));
    }
}
