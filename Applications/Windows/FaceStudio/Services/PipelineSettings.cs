using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Text.Json.Nodes;

namespace FaceStudio.Services;

/// <summary>What kind of control a parameter deserves.</summary>
public enum ParameterKind
{
    Text,
    Integer,
    Number,
    Boolean
}

/// <summary>
/// What is known about one pipeline parameter beyond its name and its value: what to call
/// it, what it does and what a sensible value looks like.
///
/// The pipeline reads settings.json as strings and validates nothing, so a settings editor
/// that offered a bare text box per key would be a worse way to edit the file than a text
/// editor. This table is what turns it into something with ranges and explanations.
/// </summary>
public sealed record ParameterDescriptor(
    string Key,
    string Label,
    ParameterKind Kind,
    string Description,
    double Minimum = 0.0,
    double Maximum = 0.0,
    double Step = 1.0);

/// <summary>One parameter of one module, as the editor binds to it.</summary>
public sealed class ParameterSetting : INotifyPropertyChanged
{
    private string _value;

    public ParameterSetting(ParameterDescriptor iDescriptor, string iValue)
    {
        Descriptor = iDescriptor;
        _value = iValue;
        OriginalValue = iValue;
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public ParameterDescriptor Descriptor { get; }

    public string Key => Descriptor.Key;

    public string Label => Descriptor.Label;

    public string Description => Descriptor.Description;

    public ParameterKind Kind => Descriptor.Kind;

    public double Minimum => Descriptor.Minimum;

    public double Maximum => Descriptor.Maximum;

    public double Step => Descriptor.Step;

    public string OriginalValue { get; private set; }

    public bool IsModified => !string.Equals(_value, OriginalValue, StringComparison.Ordinal);

    public string Value
    {
        get => _value;
        set
        {
            if (string.Equals(_value, value, StringComparison.Ordinal)) return;

            _value = value;

            OnPropertyChanged();
            OnPropertyChanged(nameof(NumberValue));
            OnPropertyChanged(nameof(BooleanValue));
            OnPropertyChanged(nameof(IsModified));
        }
    }

    /// <summary>The value as a number, for the sliders and the spin boxes.</summary>
    public double NumberValue
    {
        get => double.TryParse(_value, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsed) ? parsed : 0.0;
        set => Value = Kind == ParameterKind.Integer
            ? ((long)Math.Round(value)).ToString(CultureInfo.InvariantCulture)
            : value.ToString("0.####", CultureInfo.InvariantCulture);
    }

    /// <summary>
    /// The value as a flag. The pipeline writes these as TRUE and FALSE, and reading back
    /// anything else as false is what its own parser does.
    /// </summary>
    public bool BooleanValue
    {
        get => _value.Equals("TRUE", StringComparison.OrdinalIgnoreCase) ||
               _value.Equals("1", StringComparison.Ordinal) ||
               _value.Equals("YES", StringComparison.OrdinalIgnoreCase);
        set => Value = value ? "TRUE" : "FALSE";
    }

    public void MarkSaved()
    {
        OriginalValue = _value;

        OnPropertyChanged(nameof(OriginalValue));
        OnPropertyChanged(nameof(IsModified));
    }

    private void OnPropertyChanged([CallerMemberName] string? iName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(iName));
    }
}

/// <summary>One module of the graph, with the parameters the editor offers for it.</summary>
public sealed class ModuleSettings
{
    public ModuleSettings(string iName, string iTitle, string iDescription)
    {
        Name = iName;
        Title = iTitle;
        Description = iDescription;
        Parameters = new ObservableCollection<ParameterSetting>();
    }

    public string Name { get; }

    public string Title { get; }

    public string Description { get; }

    /// <summary>What this module is wired to, as settings.json spells it.</summary>
    public string PortSummary { get; set; } = string.Empty;

    public ObservableCollection<ParameterSetting> Parameters { get; }

    public bool HasParameters => Parameters.Count > 0;
}

/// <summary>How the overlay reaches the screen.</summary>
public enum OverlayRenderer
{
    /// <summary>This application draws it on the GPU from the results.</summary>
    Application,

    /// <summary>The pipeline's own Visualizer draws it into the frame.</summary>
    Pipeline
}

/// <summary>
/// The deployed settings.json, read for editing and written back.
///
/// The file is parsed as a tree and written from the same tree, so everything this editor
/// does not know about - a module added by hand, a comment-like key, an ordering - survives
/// a round trip. Values are written back as strings because that is what the pipeline's
/// reader expects of every one of them.
/// </summary>
public sealed class PipelineSettings
{
    private JsonNode? _root;

    private PipelineSettings(string iPath)
    {
        Path = iPath;
        Modules = new ObservableCollection<ModuleSettings>();
        General = new ObservableCollection<ParameterSetting>();
    }

    public string Path { get; }

    public ObservableCollection<ModuleSettings> Modules { get; }

    /// <summary>The general output settings, which are not part of any module.</summary>
    public ObservableCollection<ParameterSetting> General { get; }

    public OverlayRenderer Renderer { get; set; } = OverlayRenderer.Application;

    public bool IsModified =>
        Modules.SelectMany(module => module.Parameters).Any(parameter => parameter.IsModified) ||
        General.Any(parameter => parameter.IsModified) ||
        Renderer != LoadedRenderer;

    private OverlayRenderer LoadedRenderer { get; set; } = OverlayRenderer.Application;

    public static PipelineSettings Load(string iPath)
    {
        var settings = new PipelineSettings(iPath);

        try
        {
            settings._root = JsonNode.Parse(File.ReadAllText(iPath),
                                            nodeOptions: null,
                                            documentOptions: new JsonDocumentOptions
                                            {
                                                AllowTrailingCommas = true,
                                                CommentHandling = JsonCommentHandling.Skip
                                            });
        }
        catch (Exception)
        {
            settings._root = null;
        }

        settings.Populate();

        return settings;
    }

    private JsonObject? ModulesNode => _root?["face"]?["modules"] as JsonObject;

    private JsonObject? OutputNode => _root?["face"]?["general"]?["output"] as JsonObject;

    private void Populate()
    {
        Modules.Clear();
        General.Clear();

        JsonObject? modules = ModulesNode;
        JsonObject? output = OutputNode;

        foreach (ParameterDescriptor descriptor in SettingsSchema.General)
        {
            General.Add(new ParameterSetting(descriptor, ReadValue(output, descriptor.Key)));
        }

        foreach (SettingsSchema.ModuleDescriptor descriptor in SettingsSchema.Modules)
        {
            if (modules?[descriptor.Name] is not JsonObject node) continue;

            var module = new ModuleSettings(descriptor.Name, descriptor.Title, descriptor.Description)
            {
                PortSummary = ReadPorts(node)
            };

            foreach (ParameterDescriptor parameter in descriptor.Parameters)
            {
                if (node[parameter.Key] is null) continue;

                module.Parameters.Add(new ParameterSetting(parameter, ReadValue(node, parameter.Key)));
            }

            Modules.Add(module);
        }

        // A visualizer in the graph is a visualizer whose frame comes out of the pipeline:
        // it is a sink, so the pipeline hands its frame to the host by itself
        LoadedRenderer = modules?["visualizer"] is not null
            ? OverlayRenderer.Pipeline
            : OverlayRenderer.Application;

        Renderer = LoadedRenderer;
    }

    private static string ReadValue(JsonObject? iNode, string iKey)
    {
        JsonNode? value = iNode?[iKey];

        if (value is null) return string.Empty;

        // The pipeline writes every setting as a string, but a file edited by hand may well
        // carry a bare number, and losing it here would be the editor's fault
        try
        {
            return value.GetValue<string>();
        }
        catch (Exception)
        {
            return value.ToJsonString().Trim('"');
        }
    }

    private static string ReadPorts(JsonObject iNode)
    {
        if (iNode["port"] is not JsonArray ports || ports.Count == 0) return "no inputs";

        return string.Join(", ", ports.Select(port => port?.ToJsonString().Trim('"') ?? string.Empty));
    }

    /// <summary>
    /// Writes the edits back. Everything the editor did not touch is written out as it was
    /// read, because it is the same tree.
    /// </summary>
    public bool Save(out string oError)
    {
        oError = string.Empty;

        if (_root is null)
        {
            oError = "The settings file could not be parsed, so it will not be written over.";
            return false;
        }

        try
        {
            if (OutputNode is { } output)
            {
                foreach (ParameterSetting parameter in General)
                {
                    output[parameter.Key] = parameter.Value;
                }
            }

            if (ModulesNode is { } modules)
            {
                foreach (ModuleSettings module in Modules)
                {
                    if (modules[module.Name] is not JsonObject node) continue;

                    foreach (ParameterSetting parameter in module.Parameters)
                    {
                        node[parameter.Key] = parameter.Value;
                    }
                }

                ApplyRenderer(modules);
            }

            var options = new JsonSerializerOptions { WriteIndented = true };

            File.WriteAllText(Path, _root.ToJsonString(options));

            foreach (ParameterSetting parameter in General) parameter.MarkSaved();

            foreach (ParameterSetting parameter in Modules.SelectMany(module => module.Parameters))
            {
                parameter.MarkSaved();
            }

            LoadedRenderer = Renderer;

            return true;
        }
        catch (Exception exception)
        {
            oError = exception.Message;
            return false;
        }
    }

    /// <summary>
    /// Wires the graph for whoever is drawing the overlay. The Visualizer is a sink, so
    /// adding it is the whole change - nothing downstream has to be re-pointed at it.
    /// </summary>
    private void ApplyRenderer(JsonObject iModules)
    {
        if (Renderer == OverlayRenderer.Pipeline)
        {
            if (iModules["visualizer"] is not JsonObject)
            {
                iModules["visualizer"] = new JsonObject
                {
                    ["port"] = new JsonArray("imageQueue:1", "userManager:2"),
                    ["glow"] = "TRUE",
                    ["panel"] = "TRUE",
                    ["poseBox"] = "TRUE"
                };
            }
        }
        else
        {
            // Without it the frame comes out of the image queue as it went in, and this
            // application draws its own overlay on the GPU
            iModules.Remove("visualizer");
        }
    }

    public void Revert()
    {
        Populate();
    }
}
