using System.Windows;
using System.Windows.Controls;

namespace IGCSClient.Selectors
{
    public class NodeTemplateSelector : DataTemplateSelector
    {
        // Template to use when details should be shown.
        public DataTemplate WithDetailsTemplate { get; set; }

        // Template to use when details should not be shown.
        public DataTemplate WithoutDetailsTemplate { get; set; }

        // Toggle flag – set it to true to show details; false to hide.
        public bool ShowDetails { get; set; } = true;

        public override DataTemplate SelectTemplate(object item, DependencyObject container)
        {
            // Here, item is a NodeViewModel.
            return ShowDetails ? WithDetailsTemplate : WithoutDetailsTemplate;
        }
    }
}
