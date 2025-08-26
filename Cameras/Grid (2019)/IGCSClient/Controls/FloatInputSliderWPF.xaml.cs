using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using IGCSClient.Classes;
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
        private bool _useCustomStepping = false;
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

        #region Enhanced Settings Factory Support
        /// <summary>
        /// Enhanced setup with separate tick interval control
        /// </summary>
        /// <param name="minValue">Minimum slider value</param>
        /// <param name="maxValue">Maximum slider value</param>
        /// <param name="scale">Display precision (decimal places)</param>
        /// <param name="increment">Step size for precise control</param>
        /// <param name="tickInterval">Interval between visual tick marks</param>
        /// <param name="defaultValue">Default value</param>
        public void SetupWithTickControl(double minValue, double maxValue, int scale, double increment, double tickInterval, double defaultValue)
        {
            _suppressEvents = true;

            // Basic setup (same as your existing Setup method)
            _sliderControl.Maximum = maxValue;
            _sliderControl.Minimum = minValue;
            _sliderControl.AutoToolTipPrecision = scale;
            _sliderControl.SmallChange = increment;
            _sliderControl.LargeChange = Math.Max(tickInterval, increment * 5);
            _useCustomStepping = true;  // Flag for enhanced mode

            if (increment > 0)
            {
                _sliderControl.TickFrequency = increment;  // Fine stepping
                _sliderControl.IsSnapToTickEnabled = false;

                // Create custom tick collection for visual marks
                CreateCustomTickMarks(minValue, maxValue, tickInterval);
                _sliderControl.Ticks = CustomTicks;

                // Show tick marks if not too dense
                //var range = maxValue - minValue;
                //var tickCount = range / tickInterval;
                //_sliderControl.TickPlacement = tickCount > 50 ?
                //    System.Windows.Controls.Primitives.TickPlacement.None :
                //    System.Windows.Controls.Primitives.TickPlacement.Both;
            }

            this.Value = (float)defaultValue;
            _suppressEvents = false;
        }

        public static readonly DependencyProperty CustomTicksProperty =
            DependencyProperty.Register("CustomTicks", typeof(DoubleCollection), typeof(FloatInputSliderWPF),
                new PropertyMetadata(null));

        public DoubleCollection CustomTicks
        {
            get { return (DoubleCollection)GetValue(CustomTicksProperty); }
            set { SetValue(CustomTicksProperty, value); }
        }

        private void CreateCustomTickMarks(double minValue, double maxValue, double tickInterval)
        {
            var ticks = new DoubleCollection();

            if (tickInterval > 0)
            {
                int precision = GetDecimalPlaces(tickInterval);

                for (double tick = minValue; tick <= maxValue + (tickInterval / 2); tick += tickInterval)
                {
                    if (tick >= minValue && tick <= maxValue)
                    {
                        ticks.Add(Math.Round(tick, precision));
                    }
                }
            }

            CustomTicks = ticks;
        }

        private int GetDecimalPlaces(double value)
        {
            if (value >= 1) return 0;

            string valueStr = value.ToString("G17", CultureInfo.InvariantCulture);
            int decimalIndex = valueStr.IndexOf('.');
            if (decimalIndex == -1) return 0;

            int decimalPlaces = valueStr.Length - decimalIndex - 1;

            // Clamp to Math.Round's valid range (0-15)
            return Math.Max(0, Math.Min(15, decimalPlaces));
        }
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

            if (increment > 0)
            {
                _sliderControl.TickFrequency = increment;
                _sliderControl.IsSnapToTickEnabled = true;

                // Hide tick marks if they would be too dense
                var range = maxValue - minValue;
                var tickCount = range / increment;

                // Only override if ticks would be too dense, otherwise leave XAML setting alone
                if (tickCount > 100)
                {
                    _sliderControl.TickPlacement = System.Windows.Controls.Primitives.TickPlacement.None;
                }
            }

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
            //if (_suppressEvents)
            //    return;
            //this.ValueChanged?.Invoke(this, EventArgs.Empty);
            if (_suppressEvents)
                return;

            if (_useCustomStepping)
            {
                // Enhanced mode: manual rounding
                _suppressEvents = true;
                double stepSize = _sliderControl.SmallChange;
                double roundedValue = Math.Round(e.NewValue / stepSize) * stepSize;
                _sliderControl.Value = roundedValue;
                _suppressEvents = false;
            }

            this.ValueChanged?.Invoke(this, EventArgs.Empty);
        }
        #endregion
    }
}