using FaceStudio.Services;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace FaceStudio.Views;

/// <summary>
/// Picks the editor a pipeline parameter deserves: a switch for a flag, a slider for a
/// bounded number, a spin box for an unbounded one, a text box for the rest.
///
/// The choice comes from the schema rather than from the value, so a parameter reads the
/// same whether the file happens to hold "0.7" or "TRUE" in it today.
/// </summary>
public sealed class ParameterTemplateSelector : DataTemplateSelector
{
    public DataTemplate? BooleanTemplate { get; set; }

    public DataTemplate? SliderTemplate { get; set; }

    public DataTemplate? NumberTemplate { get; set; }

    public DataTemplate? TextTemplate { get; set; }

    protected override DataTemplate? SelectTemplateCore(object iItem, DependencyObject iContainer)
    {
        return SelectTemplateCore(iItem);
    }

    protected override DataTemplate? SelectTemplateCore(object iItem)
    {
        if (iItem is not ParameterSetting parameter) return TextTemplate;

        return parameter.Kind switch
        {
            ParameterKind.Boolean => BooleanTemplate,

            // A range is what makes a slider meaningful; without one it is a guess dressed
            // up as a control
            ParameterKind.Integer or ParameterKind.Number when parameter.Maximum > parameter.Minimum => SliderTemplate,

            ParameterKind.Integer or ParameterKind.Number => NumberTemplate,

            _ => TextTemplate
        };
    }
}
