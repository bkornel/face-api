using System;
using System.Collections.Generic;

namespace FaceStudio.Models;

/// <summary>
/// A bounded, ordered history of one measurement, for the charts to draw.
///
/// It is a ring rather than a growing list: the charts show the recent past, and a session
/// that runs for an hour should not cost an hour's worth of memory to show the last minute
/// of it.
/// </summary>
public sealed class MetricHistory
{
    private readonly double[] _values;

    private int _next;

    public MetricHistory(int iCapacity)
    {
        Capacity = Math.Max(2, iCapacity);
        _values = new double[Capacity];
    }

    public int Capacity { get; }

    public int Count { get; private set; }

    public double Last { get; private set; }

    public double Minimum { get; private set; }

    public double Maximum { get; private set; }

    public void Push(double iValue)
    {
        if (double.IsNaN(iValue) || double.IsInfinity(iValue)) return;

        _values[_next] = iValue;
        _next = (_next + 1) % Capacity;

        if (Count < Capacity) Count++;

        Last = iValue;

        // Over the window that is actually shown, so that one stall does not keep the scale
        // of the chart stretched for the rest of the session
        double minimum = double.MaxValue;
        double maximum = double.MinValue;

        foreach (double value in this)
        {
            minimum = Math.Min(minimum, value);
            maximum = Math.Max(maximum, value);
        }

        Minimum = Count > 0 ? minimum : 0.0;
        Maximum = Count > 0 ? maximum : 0.0;
    }

    public void Clear()
    {
        Array.Clear(_values);

        _next = 0;
        Count = 0;
        Last = 0.0;
        Minimum = 0.0;
        Maximum = 0.0;
    }

    /// <summary>Oldest first, which is left to right on a chart.</summary>
    public IEnumerator<double> GetEnumerator()
    {
        int start = Count < Capacity ? 0 : _next;

        for (int i = 0; i < Count; i++)
        {
            yield return _values[(start + i) % Capacity];
        }
    }
}
