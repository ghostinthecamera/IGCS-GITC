using IGCSClient.Controls;
using IGCSClient.Interfaces;

namespace IGCSClient.Classes
{
    /// <summary>
    /// Represents a gamepad binding setting including a control
    /// </summary>
    public class GamepadBindingSetting : Setting<KeyCombination>
    {
        #region Members
        private KeyCombination _initialCombination;
        #endregion

        public GamepadBindingSetting(byte id, string name, KeyCombination initialCombination)
            : base(id, name, SettingKind.KeyBinding)
        {
            _initialCombination = initialCombination;
            // Ensure the gamepad flag is set
            _initialCombination.IsGamepadButton = true;
        }

        public override void Setup(IInputControl<KeyCombination> controlToUse)
        {
            base.Setup(controlToUse);
            var controlAsCombinationInput = controlToUse as KeyCombinationInputWPF;
            if (controlAsCombinationInput == null)
            {
                return;
            }
            controlAsCombinationInput.Setup(_initialCombination);
        }

        protected override string GetValueAsString()
        {
            return this.Value.GetValueAsString();
        }

        protected override KeyCombination GetDefaultValue()
        {
            return _initialCombination;
        }

        public override void SendValueAsMessage()
        {
            MessageHandlerSingleton.Instance().SendKeyBindingMessage(this.ID, this.Value.GetValueAsByteArray());
        }
    }
}