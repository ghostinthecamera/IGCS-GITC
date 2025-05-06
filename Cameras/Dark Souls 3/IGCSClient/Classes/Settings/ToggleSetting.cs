using IGCSClient.Interfaces;
using System;
using System.Collections.Generic;
using IGCSClient.Controls;

namespace IGCSClient.Classes.Settings
{
    /// <summary>
    /// Represents a toggle setting that uses ToggleGroupInputWPF as its input control.
    /// This setting sends an integer value, which can be optionally customized for each toggle.
    /// </summary>
    public class ToggleSetting : Setting<int>
    {
        private readonly List<string> _toggleOptions;
        private readonly List<int> _toggleValues;
        private readonly int _defaultValue;

        /// <summary>
        /// Creates a new ToggleSetting.
        /// </summary>
        /// <param name="id">The unique setting ID.</param>
        /// <param name="name">The setting name.</param>
        /// <param name="toggleOptions">A list of labels for the toggles.</param>
        /// <param name="toggleValues">
        /// Optional: a list of integer values for each toggle. If null, the control will use the zero‑based index.
        /// </param>
        /// <param name="defaultValue">The default value for the setting.</param>
        public ToggleSetting(byte id, string name, List<string> toggleOptions, List<int> toggleValues, int defaultValue)
            : base(id, name, SettingKind.NormalSetting)
        {
            if (toggleOptions == null || toggleOptions.Count == 0)
                throw new ArgumentException("Toggle options must have at least one element.", nameof(toggleOptions));
            if (toggleValues != null && toggleOptions.Count != toggleValues.Count)
                throw new ArgumentException("Toggle values must have the same number of items as toggle options.", nameof(toggleValues));

            _toggleOptions = toggleOptions;
            _toggleValues = toggleValues; // if null, the control will fall back to using indices.
            _defaultValue = defaultValue;
        }

        public override void Setup(IInputControl<int> controlToUse)
        {
            base.Setup(controlToUse);

            // Cast the control to our custom ToggleGroupInputWPF.
            if (controlToUse is not ToggleGroupInputWPF toggleControl)
                return;

            // Use our updated Setup method that takes an optional list of values.
            toggleControl.Setup(_toggleOptions, _toggleValues);
            toggleControl.Value = _defaultValue;
        }

        protected override int GetDefaultValue()
        {
            return _defaultValue;
        }
    }
}
