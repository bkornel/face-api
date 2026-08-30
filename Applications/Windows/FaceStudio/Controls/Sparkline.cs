using System;
using System.Collections.Generic;

using FaceStudio.Models;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;

using Windows.Foundation;

namespace FaceStudio.Controls;

/// <summary>
/// The recent history of one measurement, as a line with a soft fill under it.
///
/// It is redrawn from the history rather than bound to it: the points change together on
/// every tick, and rebuilding one geometry is cheaper and steadier than notifying a
/// collection of two hundred and forty items twenty times a second.
///
/// The shapes live on a Canvas, which does not measure its children. A polyline whose
/// points are computed from the width it was given would otherwise report that width back
/// as its own desired size, and XAML gives up after a few passes of that.
/// </summary>
public sealed class Sparkline : UserControl
{
    public static readonly DependencyProperty StrokeProperty = DependencyProperty.Register(
        nameof(Stroke), typeof(Brush), typeof(Sparkline), new PropertyMetadata(null, OnStrokeChanged));

    /// <summary>
    /// Pins the bottom of the chart to zero instead of to the smallest sample. A frame rate
    /// hovering around 30 should read as a steady line near the top, not as a mountain range
    /// made of a tenth of a frame.
    /// </summary>
    public static readonly DependencyProperty BaselineAtZeroProperty = DependencyProperty.Register(
        nameof(BaselineAtZero), typeof(bool), typeof(Sparkline), new PropertyMetadata(true));

    /// <summary>
    /// Pins the top of the chart, so that two sparklines drawn over one another can be
    /// compared. Zero lets the chart scale itself to what it holds.
    /// </summary>
    public static readonly DependencyProperty FixedMaximumProperty = DependencyProperty.Register(
        nameof(FixedMaximum), typeof(double), typeof(Sparkline), new PropertyMetadata(0.0));

    private readonly Path _fill;
    private readonly Polyline _line;
    private readonly Canvas _root;

    private readonly List<double> _values = new();

    public Sparkline()
    {
        IsTabStop = false;
        MinHeight = 40.0;
        HorizontalAlignment = HorizontalAlignment.Stretch;

        _fill = new Path { Opacity = 0.16 };

        _line = new Polyline
        {
            StrokeThickness = 1.6,
            StrokeLineJoin = PenLineJoin.Round,
            StrokeStartLineCap = PenLineCap.Round,
            StrokeEndLineCap = PenLineCap.Round
        };

        _root = new Canvas();
        _root.Children.Add(_fill);
        _root.Children.Add(_line);

        Content = _root;

        SizeChanged += (_, _) => Redraw();
        Loaded += (_, _) => ApplyStroke();
    }

    public Brush? Stroke
    {
        get => (Brush?)GetValue(StrokeProperty);
        set => SetValue(StrokeProperty, value);
    }

    public bool BaselineAtZero
    {
        get => (bool)GetValue(BaselineAtZeroProperty);
        set => SetValue(BaselineAtZeroProperty, value);
    }

    public double FixedMaximum
    {
        get => (double)GetValue(FixedMaximumProperty);
        set => SetValue(FixedMaximumProperty, value);
    }

    /// <summary>Takes a copy of the history and redraws. Called from the page's update tick.</summary>
    public void Update(MetricHistory iHistory)
    {
        ArgumentNullException.ThrowIfNull(iHistory);

        _values.Clear();

        var enumerator = iHistory.GetEnumerator();

        while (enumerator.MoveNext()) _values.Add(enumerator.Current);

        Redraw();
    }

    private static void OnStrokeChanged(DependencyObject iSender, DependencyPropertyChangedEventArgs iArgs)
    {
        ((Sparkline)iSender).ApplyStroke();
    }

    private void ApplyStroke()
    {
        Brush? brush = Stroke
                       ?? (Application.Current.Resources.TryGetValue("AccentBrush", out object? accent) && accent is Brush accentBrush
                           ? accentBrush
                           : null);

        _line.Stroke = brush;
        _fill.Fill = brush;
    }

    private void Redraw()
    {
        double width = _root.ActualWidth;
        double height = _root.ActualHeight;

        _line.Points.Clear();

        if (_values.Count < 2 || width <= 1.0 || height <= 1.0)
        {
            _fill.Data = null;
            return;
        }

        double minimum = BaselineAtZero ? 0.0 : double.MaxValue;
        double maximum = double.MinValue;

        foreach (double value in _values)
        {
            if (!BaselineAtZero) minimum = Math.Min(minimum, value);
            maximum = Math.Max(maximum, value);
        }

        // Two charts that scale themselves cannot be read against one another, however
        // neatly they overlap
        if (FixedMaximum > 0.0) maximum = FixedMaximum;

        // A flat history would divide by zero; drawing it through the middle is what a flat
        // line should look like anyway
        double span = maximum - minimum;
        if (span < 1e-9) span = 1.0;

        // A little headroom, so the peak is not clipped by the top edge
        const double Inset = 2.0;

        double usable = Math.Max(1.0, height - 2.0 * Inset);

        var figure = new PathFigure { StartPoint = new Point(0.0, height), IsClosed = true, IsFilled = true };

        for (int i = 0; i < _values.Count; i++)
        {
            double x = width * i / (_values.Count - 1);
            double y = Inset + usable * (1.0 - Math.Clamp((_values[i] - minimum) / span, 0.0, 1.0));

            _line.Points.Add(new Point(x, y));
            figure.Segments.Add(new LineSegment { Point = new Point(x, y) });
        }

        figure.Segments.Add(new LineSegment { Point = new Point(width, height) });

        var geometry = new PathGeometry();
        geometry.Figures.Add(figure);

        _fill.Data = geometry;
    }
}
