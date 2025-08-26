using IGCSClient.Interfaces;
using ModernWpf.Controls;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace IGCSClient.Controls
{
    /// <summary>
    /// Interaction logic for ToggleGroupInputWPF.xaml
    /// This control creates a group of toggle switches based on provided options.
    /// Only one toggle can be active at a time.
    /// Optionally, a header and horizontal rule can be displayed above the toggles.
    /// It now also lets you optionally specify the integer value each toggle sends.
    /// </summary>
    public partial class ToggleGroupInputWPF : UserControl, IInputControl<int>
    {
        /// <summary>
        /// Raised when the selected toggle changes.
        /// </summary>
        public event EventHandler ValueChanged;

        // Holds references to all created toggles.
        private readonly List<ToggleSwitch> _toggleSwitches = new();

        // The currently selected value.
        private int _currentValue = -1;

        public ToggleGroupInputWPF()
        {
            InitializeComponent();
            UpdateHeaderVisibility();

            // Design-time sample data
            if (DesignerProperties.GetIsInDesignMode(this))
            {
                Header = "Sample Header";
                var sampleToggles = new List<string>
                {
                    "Option 1",
                    "Option 2",
                    "Option 3"
                };
                // Optional: sample toggle values different than indices.
                var sampleValues = new List<int> { 10, 20, 30 };
                Setup(sampleToggles, sampleValues);
                Value = 10;
            }
        }

        #region Dependency Properties
        // [Dependency properties for margins, font sizes, etc. remain unchanged]
        // Header properties
        public Thickness HeaderMargin
        {
            get { return (Thickness)GetValue(HeaderMarginProperty); }
            set { SetValue(HeaderMarginProperty, value); }
        }
        public static readonly DependencyProperty HeaderMarginProperty =
            DependencyProperty.Register("HeaderMargin", typeof(Thickness), typeof(ToggleGroupInputWPF), new PropertyMetadata(new Thickness(0, 0, 0, 2)));

        public double HeaderFontSize
        {
            get { return (double)GetValue(HeaderFontSizeProperty); }
            set { SetValue(HeaderFontSizeProperty, value); }
        }
        public static readonly DependencyProperty HeaderFontSizeProperty =
            DependencyProperty.Register("HeaderFontSize", typeof(double), typeof(ToggleGroupInputWPF), new PropertyMetadata(11.0));

        public FontWeight HeaderFontWeight
        {
            get { return (FontWeight)GetValue(HeaderFontWeightProperty); }
            set { SetValue(HeaderFontWeightProperty, value); }
        }
        public static readonly DependencyProperty HeaderFontWeightProperty =
            DependencyProperty.Register("HeaderFontWeight", typeof(FontWeight), typeof(ToggleGroupInputWPF), new PropertyMetadata(FontWeights.Bold));

        // Border properties
        public Thickness BorderMargin
        {
            get { return (Thickness)GetValue(BorderMarginProperty); }
            set { SetValue(BorderMarginProperty, value); }
        }
        public static readonly DependencyProperty BorderMarginProperty =
            DependencyProperty.Register("BorderMargin", typeof(Thickness), typeof(ToggleGroupInputWPF), new PropertyMetadata(new Thickness(0, 0, 0, 2)));

        // Toggle panel properties
        public Thickness TogglePanelMargin
        {
            get { return (Thickness)GetValue(TogglePanelMarginProperty); }
            set { SetValue(TogglePanelMarginProperty, value); }
        }
        public static readonly DependencyProperty TogglePanelMarginProperty =
            DependencyProperty.Register("TogglePanelMargin", typeof(Thickness), typeof(ToggleGroupInputWPF), new PropertyMetadata(new Thickness(10, 0, 5, 0)));

        // Toggle font properties
        public double ToggleFontSize
        {
            get { return (double)GetValue(ToggleFontSizeProperty); }
            set { SetValue(ToggleFontSizeProperty, value); }
        }
        public static readonly DependencyProperty ToggleFontSizeProperty =
            DependencyProperty.Register("ToggleFontSize", typeof(double), typeof(ToggleGroupInputWPF), new PropertyMetadata(17.0));

        public FontWeight ToggleFontWeight
        {
            get { return (FontWeight)GetValue(ToggleFontWeightProperty); }
            set { SetValue(ToggleFontWeightProperty, value); }
        }
        public static readonly DependencyProperty ToggleFontWeightProperty =
            DependencyProperty.Register("ToggleFontWeight", typeof(FontWeight), typeof(ToggleGroupInputWPF), new PropertyMetadata(FontWeights.Normal));

        // Header text property
        public string Header
        {
            get { return (string)GetValue(HeaderProperty); }
            set { SetValue(HeaderProperty, value); }
        }
        public static readonly DependencyProperty HeaderProperty =
            DependencyProperty.Register("Header", typeof(string), typeof(ToggleGroupInputWPF),
                new PropertyMetadata(string.Empty, OnHeaderChanged));

        private static void OnHeaderChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as ToggleGroupInputWPF;
            control?.UpdateHeaderVisibility();
        }
        #endregion

        private void UpdateHeaderVisibility()
        {
            HeaderPanel.Visibility = string.IsNullOrWhiteSpace(Header) ? Visibility.Collapsed : Visibility.Visible;
        }

        /// <summary>
        /// Gets or sets the current selected value.
        /// Setting this value programmatically will update the UI.
        /// </summary>
        public int Value
        {
            get => _currentValue;
            set
            {
                if (value != _currentValue)
                {
                    SetValueInternal(value);
                }
            }
        }

        /// <summary>
        /// Sets up the control with a list of toggle options.
        /// Optionally, you can supply a list of integer values (toggleValues) that correspond to each toggle.
        /// If toggleValues is null, the control uses the zero-based index.
        /// </summary>
        /// <param name="toggleOptions">A list of strings representing each toggle’s label.</param>
        /// <param name="toggleValues">Optional: a list of int values for each toggle.</param>
        public void Setup(List<string> toggleOptions, List<int> toggleValues = null)
        {
            if (toggleOptions == null || toggleOptions.Count == 0)
                throw new ArgumentException("Toggle options must contain at least one option.", nameof(toggleOptions));

            if (toggleValues != null && toggleValues.Count != toggleOptions.Count)
                throw new ArgumentException("toggleValues must have the same number of items as toggleOptions.", nameof(toggleValues));

            // Clear any existing toggles.
            TogglePanel.Children.Clear();
            _toggleSwitches.Clear();

            // Dynamically create a toggle for each option.
            for (int i = 0; i < toggleOptions.Count; i++)
            {
                // Use the provided value if available; otherwise, use the index.
                int valueToUse = toggleValues != null ? toggleValues[i] : i;

                var toggle = new ToggleSwitch
                {
                    OffContent = toggleOptions[i],
                    OnContent = toggleOptions[i],
                    Tag = valueToUse,
                    IsOn = false,
                    FontSize = ToggleFontSize,
                    FontWeight = ToggleFontWeight
                };

                // Attach the Toggled event handler.
                toggle.Toggled += Toggle_Toggled;

                _toggleSwitches.Add(toggle);
                TogglePanel.Children.Add(toggle);
            }
        }

        /// <summary>
        /// Sets the value from a string representation.
        /// </summary>
        /// <param name="valueAsString">The value as a string.</param>
        /// <param name="defaultValue">The default value if parsing fails.</param>
        public void SetValueFromString(string valueAsString, int defaultValue)
        {
            if (!int.TryParse(valueAsString, out int result))
            {
                result = defaultValue;
            }
            Value = result;
        }

        /// <summary>
        /// Handles the toggled event for each toggle switch.
        /// When one toggle is turned on, this method turns off all others and updates the current value.
        /// </summary>
        private void Toggle_Toggled(object sender, RoutedEventArgs e)
        {
            if (sender is ToggleSwitch toggledSwitch)
            {
                if (toggledSwitch.IsOn)
                {
                    int newValue = (int)toggledSwitch.Tag;
                    if (_currentValue != newValue)
                    {
                        _currentValue = newValue;
                        ValueChanged?.Invoke(this, EventArgs.Empty);
                    }

                    // Ensure only one toggle is active.
                    foreach (var toggle in _toggleSwitches)
                    {
                        if (!toggle.Equals(toggledSwitch))
                        {
                            toggle.IsOn = false;
                        }
                    }
                }
                else
                {
                    // Prevent deselecting the only active toggle.
                    bool anyOn = false;
                    foreach (var toggle in _toggleSwitches)
                    {
                        if (toggle.IsOn)
                        {
                            anyOn = true;
                            break;
                        }
                    }
                    if (!anyOn)
                    {
                        toggledSwitch.IsOn = true;
                    }
                }
            }
        }

        /// <summary>
        /// Programmatically sets the toggle corresponding to the specified value to be active.
        /// If a toggle with a matching Tag is found, it is set to on and all others are turned off.
        /// </summary>
        /// <param name="newValue">The value to select.</param>
        private void SetValueInternal(int newValue)
        {
            bool found = false;
            foreach (var toggle in _toggleSwitches)
            {
                int toggleValue = (int)toggle.Tag;
                if (toggleValue == newValue)
                {
                    toggle.IsOn = true;
                    found = true;
                }
                else
                {
                    toggle.IsOn = false;
                }
            }

            if (found && _currentValue != newValue)
            {
                _currentValue = newValue;
                ValueChanged?.Invoke(this, EventArgs.Empty);
            }
        }

        #region Helper Methods for Individual Toggle Access

        /// <summary>
        /// Retrieves the ToggleSwitch control associated with the specified custom value.
        /// Returns null if no matching toggle is found.
        /// </summary>
        /// <param name="value">The custom integer value assigned to the toggle.</param>
        public ToggleSwitch GetToggleByValue(int value)
        {
            foreach (var toggle in _toggleSwitches)
            {
                if ((int)toggle.Tag == value)
                {
                    return toggle;
                }
            }
            return null;
        }

        /// <summary>
        /// Retrieves the ToggleSwitch control associated with the specified label.
        /// Returns null if no matching toggle is found.
        /// </summary>
        /// <param name="label">The label of the toggle (assumed to match OffContent).</param>
        public ToggleSwitch GetToggleByLabel(string label)
        {
            if (string.IsNullOrEmpty(label))
                return null;

            foreach (var toggle in _toggleSwitches)
            {
                // Assuming OffContent is a string; adjust if necessary.
                if (toggle.OffContent != null && toggle.OffContent.ToString().Equals(label, StringComparison.InvariantCulture))
                {
                    return toggle;
                }
            }
            return null;
        }

        /// <summary>
        /// Enables or disables the toggle corresponding to the specified custom value.
        /// </summary>
        /// <param name="value">The custom integer value assigned to the toggle.</param>
        /// <param name="isEnabled">True to enable; false to disable.</param>
        public void SetToggleEnabled(int value, bool isEnabled)
        {
            var toggle = GetToggleByValue(value);
            if (toggle != null)
            {
                toggle.IsEnabled = isEnabled;
            }
        }

        /// <summary>
        /// Enables or disables the toggle corresponding to the specified label.
        /// </summary>
        /// <param name="label">The label of the toggle.</param>
        /// <param name="isEnabled">True to enable; false to disable.</param>
        public void SetToggleEnabledByLabel(string label, bool isEnabled)
        {
            var toggle = GetToggleByLabel(label);
            if (toggle != null)
            {
                toggle.IsEnabled = isEnabled;
            }
        }

        /// <summary>
        /// Checks whether the toggle corresponding to the specified custom value is enabled.
        /// Returns false if no matching toggle is found.
        /// </summary>
        /// <param name="value">The custom integer value assigned to the toggle.</param>
        public bool IsToggleEnabled(int value)
        {
            var toggle = GetToggleByValue(value);
            return toggle != null && toggle.IsEnabled;
        }

        /// <summary>
        /// Checks whether the toggle corresponding to the specified label is enabled.
        /// Returns false if no matching toggle is found.
        /// </summary>
        /// <param name="label">The label of the toggle.</param>
        public bool IsToggleEnabledByLabel(string label)
        {
            var toggle = GetToggleByLabel(label);
            return toggle != null && toggle.IsEnabled;
        }

        #endregion

    }
}