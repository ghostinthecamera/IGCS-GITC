using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using IGCSClient.Interfaces;

namespace IGCSClient.Controls
{
    /// <summary>
    /// Float editor using a slider
    /// </summary>
    public partial class FloatInputSliderWPF : UserControl, IInputControl<float>, IFloatSettingControl
    {
        #region Members
        private bool _suppressEvents = false;
        public event EventHandler ValueChanged;
        #endregion

        public FloatInputSliderWPF()
        {
            InitializeComponent();
        }

        public void SetValueSilently(float value)
        {
            _suppressEvents = true;
            try
            {
                _sliderControl.Value = Convert.ToDouble(value);
            }
            finally
            {
                _suppressEvents = false;
            }
        }

        #region Dependency Properties
        // New Header property (will keep your implementation intact but add this for proper binding)
        public static readonly DependencyProperty HeaderProperty =
            DependencyProperty.Register("Header", typeof(string), typeof(FloatInputSliderWPF),
                new PropertyMetadata(string.Empty));

        public static readonly DependencyProperty UseCustomTemplateProperty =
            DependencyProperty.Register("UseCustomTemplate", typeof(bool), typeof(FloatInputSliderWPF),
                new PropertyMetadata(false));

        // Existing dependency properties
        public static readonly DependencyProperty LeftLabelTextProperty =
            DependencyProperty.Register("LeftLabelText", typeof(string), typeof(FloatInputSliderWPF),
                new PropertyMetadata(string.Empty));

        public static readonly DependencyProperty RightLabelTextProperty =
            DependencyProperty.Register("RightLabelText", typeof(string), typeof(FloatInputSliderWPF),
                new PropertyMetadata(string.Empty));

        public static readonly DependencyProperty LabelFontSizeProperty =
            DependencyProperty.Register("LabelFontSize", typeof(double), typeof(FloatInputSliderWPF),
                new PropertyMetadata(10.0));

        public static readonly DependencyProperty LabelFontWeightProperty =
            DependencyProperty.Register("LabelFontWeight", typeof(FontWeight), typeof(FloatInputSliderWPF),
                new PropertyMetadata(FontWeights.Regular));

        public static readonly DependencyProperty LabelColourProperty =
            DependencyProperty.Register("LabelColour", typeof(Brush), typeof(FloatInputSliderWPF),
                new PropertyMetadata(Brushes.Black));

        public static readonly DependencyProperty LabelMarginProperty =
            DependencyProperty.Register("LabelMargin", typeof(Thickness), typeof(FloatInputSliderWPF),
                // Updated default for more consistent spacing
                new PropertyMetadata(new Thickness(0, 3, 0, 0)));
        #endregion

        #region Public Methods for Settings Factory
        public void Setup(double minValue, double maxValue, int scale, double increment, double defaultValue)
        {
            _suppressEvents = true;
            _sliderControl.Maximum = maxValue;
            _sliderControl.Minimum = minValue;
            _sliderControl.AutoToolTipPrecision = scale;
            _sliderControl.SmallChange = increment;
            _sliderControl.LargeChange = increment;
            _suppressEvents = false;
        }

        public void SetValueFromString(string valueAsString, float defaultValue)
        {
            if (!Single.TryParse(valueAsString, NumberStyles.Float, CultureInfo.InvariantCulture.NumberFormat, out var valueToSet))
            {
                valueToSet = defaultValue;
            }
            this.Value = valueToSet;
        }
        #endregion

        #region Properties
        // Keep your existing implementation but access the dependency property
        public string Header
        {
            get { return (string)GetValue(HeaderProperty); }
            set
            {
                SetValue(HeaderProperty, value);
                _headerControl.Header = value; // Direct update for backward compatibility
            }
        }

        public bool UseCustomTemplate
        {
            get { return (bool)GetValue(UseCustomTemplateProperty); }
            set { SetValue(UseCustomTemplateProperty, value); }
        }

        public float Value
        {
            get { return Convert.ToSingle(_sliderControl.Value); }
            set
            {
                _suppressEvents = true;
                _sliderControl.Value = Convert.ToDouble(value);
                _suppressEvents = false;
            }
        }

        public float TickValue
        {
            get { return Convert.ToSingle(_sliderControl.TickFrequency); }
            set { _sliderControl.TickFrequency = Convert.ToDouble(value); }
        }

        public bool SnaptoTick
        {
            get { return _sliderControl.IsSnapToTickEnabled; }
            set { _sliderControl.IsSnapToTickEnabled = value; }
        }

        public System.Windows.Controls.Primitives.TickPlacement TickPlacement
        {
            get { return _sliderControl.TickPlacement; }
            set { _sliderControl.TickPlacement = value; }
        }

        public string LeftLabelText
        {
            get { return (string)GetValue(LeftLabelTextProperty); }
            set { SetValue(LeftLabelTextProperty, value); }
        }

        public string RightLabelText
        {
            get { return (string)GetValue(RightLabelTextProperty); }
            set { SetValue(RightLabelTextProperty, value); }
        }

        public double LabelFontSize
        {
            get { return (double)GetValue(LabelFontSizeProperty); }
            set { SetValue(LabelFontSizeProperty, value); }
        }

        public FontWeight LabelFontWeight
        {
            get { return (FontWeight)GetValue(LabelFontWeightProperty); }
            set { SetValue(LabelFontWeightProperty, value); }
        }

        public Brush LabelColour
        {
            get { return (Brush)GetValue(LabelColourProperty); }
            set { SetValue(LabelColourProperty, value); }
        }

        public Thickness LabelMargin
        {
            get { return (Thickness)GetValue(LabelMarginProperty); }
            set { SetValue(LabelMarginProperty, value); }
        }
        #endregion

        #region Event Handlers
        private void _sliderControl_OnValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (_suppressEvents)
                return;
            this.ValueChanged?.Invoke(this, EventArgs.Empty);
        }
        #endregion
    }
}