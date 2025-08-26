using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows;

namespace IGCSClient.Controls
{
    public class SliderTemplateSelector : DataTemplateSelector
    {
        public ControlTemplate DefaultTemplate { get; set; }
        public ControlTemplate CustomTemplate { get; set; }

        public override DataTemplate SelectTemplate(object item, DependencyObject container)
        {
            var element = container as FrameworkElement;
            if (element != null && item is FloatInputSliderWPF slider)
            {
                if (slider.UseCustomTemplate)
                {
                    return element.FindResource("CustomSliderTemplate") as DataTemplate;
                }
                else
                {
                    return element.FindResource("DefaultSliderTemplate") as DataTemplate;
                }
            }
            return base.SelectTemplate(item, container);
        }
    }
}
