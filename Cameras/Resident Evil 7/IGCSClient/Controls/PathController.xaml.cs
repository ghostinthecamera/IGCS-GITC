////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2025, Hussain Abdullah
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////
using IGCSClient.Classes;
using IGCSClient.Helpers;
using IGCSClient.Selectors;
using IGCSClient.ViewModels;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace IGCSClient.Controls
{
    /// <summary>
    /// Interaction logic for UserControl1.xaml
    /// </summary>
    public partial class PathController : UserControl
    {
        #region Variables

        // Create an instance of the MainViewModel.
        public MainViewModel ViewModel { get; } = new MainViewModel();
        public TreeView PathTreeViewControl => _pathTreeView;
        private int _pmState = 0;
        private bool _isCameraEnabled = false;
        private bool _isVisualisationEnabled = false;
        private string _incomingSelectedPath;
        private DispatcherTimer _uiRefreshTimer;
        public DispatcherTimer UITimer => _uiRefreshTimer;
        private string _previousPathName = string.Empty;
        private int _previousSelectedNode = 0;
        // safety limits
        private const int MinPathNameLength = 4;
        private const int MaxPathNameLength = 10;
        private const int MaxTotalPaths = 5;
        private const int MaxNodesPerPath = 8;

        private bool _isScrubbing = false;
        private bool _isUpdatingSlider = false;
        private readonly object _scrubberLock = new();

        private readonly Dictionary<string, DateTime> _lastActionTimes = new();
        private const int ACTION_THROTTLE_MS = 750; // Prevent actions within 1000ms(1s)

        private static readonly Dictionary<string, string> _tooltipTexts = new()
        {
            // Buttons
            { nameof(_createPath),
                $"Create a new camera path. Max: {MaxTotalPaths}" },
            { nameof(_deletePath),
                "Delete the selected camera path" },
            { nameof(_addNode),
                $"Add a node to the current path. When a node is highlighted, this will change to Insert Node and will create a node before the currently selected node. Maximum: {MaxNodesPerPath} nodes per Path" },
            { nameof(_deleteNode),
                "Remove the selected node from the path. If this is pressed while the Path name is selected, the last node will be deleted" },
            { nameof(_playPath),
                "Play or stop the selected path" },
            { nameof(_refreshPath),
                "Regenerate path metrics. May help address path inconsistencies" },
            { nameof(_updateNode),
                "Update the selected node to the current camera position, rotation and field of view" },

            // Path‑specific inputs
            { nameof(_pathDuration),
                "Duration of the path in seconds" },
            { nameof(_easingValue),
                "How much easing to apply when using easing. Smaller values result in less easing, larger values in more" },
            { nameof(_deltaType),
                "Path Controller Only: Enter a fixed value in seconds. This should be aligned to your FPS generally, otherwise the actual path duration will not align with what you specify. I.e. if you specify 0.016 (60fps) and your game is running at 120fps (0.008s) then your path will travel at double the usual speed." },
            { nameof(_deltaValue),
                "Fixed delta‑time between frames (if fixed selected)" },
            { nameof(_constantSpeed),
                "Enable constant speed along the path. Disable if you have a path which is rotation/fov only to get smooth interpolation. May create better results if you have a path with mixed nodes (i.e. rotation/fov only and position only nodes). However, you will not have a constant speed through these types of path." },
            { nameof(_eulerOrder),
                "Choose Euler rotation order for interpolation" },
            { nameof(_interpolationMode),
                "The interpolation method for position. CatmullRom is fine for 99.9% of paths. All others are experimental. B‑Splines do not go through the nodes you specify." },
            { nameof(_rotationMode),
                "The interpolation method for rotation. Standard uses the defined rotation method for the position method. B‑Splines only work with their own rotation method and so no other method can be selected there. Centripetal works better with SQUAD." },
            { nameof(txtPathName),
                $"Defaults to Path(n). Can be changed – must be between {MinPathNameLength} and {MaxPathNameLength} characters" },

            // Shake & handheld
            { nameof(_enableShake),
                "Toggle simple camera shake effect on/off" },
            { nameof(_shakeAmplitude),
                "Amplitude of the shake. Larger values create larger movements" },
            { nameof(_shakeFrequency),
                "Frequency of the shake. Larger values create more vigorous movements, smaller values create longer movements" },
            { nameof(_enablehandheld),
                "Toggle handheld‑camera effect on/off" },
            { nameof(_handheldIntensity),
                "Intensity of the handheld motion. Increasing this generally increases the impact of the effect" },
            { nameof(_handheldDriftIntensity),
                "Controls the overall impact of drift parameters. Larger values create more pronounced drift" },
            { nameof(_handheldJitterIntensity),
                "Intensity of handheld jitter (the smaller, sharper motions). Increase for more jitter" },
            { nameof(_handheldBreathingIntensity),
                "Intensity of breathing motion" },
            { nameof(_handheldBreathingRate),
                "Rate of breathing motion" },
            { nameof(_handheldPositionToggle),
                "Toggle handheld effect to position. When enabled it will isolate shake just to camera position" },
            { nameof(_handheldRotationToggle),
                "Toggle handheld effect to rotation. When enabled it will isolate shake just to camera rotation" },
            { nameof(_handheldDriftSpeed),
                "Rate of drift for positional component. Increase for faster movements in camera position" },
            { nameof(_handheldRotationDriftSpeed),
                "Rate of drift for rotational component. Increase for faster movements in camera rotation" },

            // Misc toggles & panels
            { nameof(_relativePlayer),
                "Toggle player‑relative path mode. When enabled the path will follow the player." },
            { nameof(_relativePathSmoothness),
                "Controls how much interpolation is applied when following the player. Smaller values will produce smoother, more dampened responses. Larger values will create a tighter response that follows the player closely" },
            { nameof(_waitbeforePlaying),
                "Wait a few seconds before playing" },
            { nameof(_resumeonPlay),
                "Unpause the game (through the tools, not in‑game pause) when Play is pressed" },
            { nameof(_easingPanel),
                "Select which easing curve to use. Ease‑in applies only to the beginning of the path. Ease‑out applies only to the end. Ease‑in‑out applies to both. As the camera needs to maintain the same duration, the camera will have a different speed when compared to a non‑eased playback." },
            { nameof(_sampleSizePanel),
                "Set number of samples for path arc length generation. 256 is usually enough. Higher values should yield smoother interpolation. YOU MUST click 'Refresh Path' for this to take effect" },
            { nameof(_toggleVisualisation),
                "Show/hide on‑screen path visualization" },
            { nameof(_toggleDisplayAll),
                "Display all paths on screen" }
        };

        #endregion

        public PathController()
        {
            InitializeComponent();
            DataContext = ViewModel;

            // Set initial placeholder
            SetPlaceholder();

            // Subscribe to the BoolInputWPF ValueChanged event.
            _toggleDetails.ValueChanged += ToggleDetails_ValueChanged;
            _pathTreeView.SelectedItemChanged += _pathTreeView_SelectedItemChanged;
            ViewModel.CameraPaths.CollectionChanged += CameraPaths_CollectionChanged;
            txtPathName.GotFocus += TxtPathName_GotFocus;
            txtPathName.LostFocus += TxtPathName_LostFocus;

            // Subscribe to mouse events for scrubbing control
            _pathScrubber.PreviewMouseDown += PathScrubber_PreviewMouseDown;
            _pathScrubber.PreviewMouseUp += PathScrubber_PreviewMouseUp;

            _pathScrubber.SetValueSilently(0);

            InitializeUIRefreshTimer(500);
            foreach (var kv in _tooltipTexts)
            {
                if (FindName(kv.Key) is FrameworkElement fe)
                    ToolTipService.SetToolTip(fe, kv.Value);
            }
        }

        private bool CanExecuteAction(string actionName)
        {
            if (_lastActionTimes.TryGetValue(actionName, out var lastTime))
            {
                if ((DateTime.Now - lastTime).TotalMilliseconds < ACTION_THROTTLE_MS)
                    return false;
            }

            _lastActionTimes[actionName] = DateTime.Now;
            return true;
        }

        /// <summary>
        /// Updates the enabled state of all path control buttons based on all relevant conditions
        /// </summary>
        /// <summary>
        /// Updates the enabled state of all path control buttons based on all relevant conditions
        /// </summary>
        private void UpdateButtonStates()
        {
            // Get current state information
            var selected = _pathTreeView.SelectedItem;
            var pathsExist = ViewModel.CameraPaths.Count > 0;

            // Important: If there are no paths, disable ALL path-related buttons immediately
            if (!pathsExist || selected == null)
            {
                _addNode.IsEnabled = false;
                _deleteNode.IsEnabled = false;
                _deletePath.IsEnabled = false;
                _playPath.IsEnabled = false; // Only enabled when playing to allow stopping
                _updateNode.IsEnabled = false;
                _refreshPath.IsEnabled = false;
                _pathScrubber.IsEnabled = false; // Disable scrubber
                _pathProgressText.IsEnabled = false;

                // After handling the empty state, return early
                return;
            }

            var nodeCount = GetNodeCountOfCurrentlySelectedPath();
            var validSelection = selected is CameraPathViewModel or NodeViewModel or NodeDetail;
            var isNodeSelection = selected is NodeViewModel or NodeDetail;
            var isPathSelection = selected is CameraPathViewModel;

            // 1. Create Path button - always enabled as long as limit not reached and not playing
            _createPath.IsEnabled = ViewModel.CameraPaths.Count < MaxTotalPaths && PMState is PathManagerState.Idle;

            // Handle the rest only if we have a valid selection
            if (!validSelection)
            {
                _addNode.IsEnabled = false;
                _deleteNode.IsEnabled = false;
                _deletePath.IsEnabled = false;
                _playPath.IsEnabled = PMState is PathManagerState.PlayingPath; // Only enabled when playing to allow stopping
                _updateNode.IsEnabled = false;
                _refreshPath.IsEnabled = false;
                _pathScrubber.IsEnabled = false; // Disable scrubber
                _pathProgressText.IsEnabled = false;
                return;
            }

            // 2. Delete Path button
            _deletePath.IsEnabled = PMState is PathManagerState.Idle;

            // 3. Play Path button
            _playPath.IsEnabled = (nodeCount >= 2 && PMState is PathManagerState.PlayingPath or PathManagerState.Idle);

            // 4. Add/Insert Node button
            _addNode.IsEnabled = nodeCount < MaxNodesPerPath && PMState is PathManagerState.Idle;

            // 5. Delete Node button
            _deleteNode.IsEnabled = PMState==0;

            // 6. Update Node button - Only enable if a Node is selected (not just a path)
            _updateNode.IsEnabled = isNodeSelection && PMState is PathManagerState.Idle;

            // 7. Refresh Path button
            _refreshPath.IsEnabled = PMState is PathManagerState.Idle;

            // Update button text for Add/Insert Node
            _addNodeText.Text = isPathSelection ? "Add Node" : "Insert Node";



            // Update play button icon and text
            if (PMState is PathManagerState.PlayingPath)
            {
                _playIcon.Symbol = ModernWpf.Controls.Symbol.Stop;
                _playIconText.Text = "Stop";
            }
            else
            {
                _playIcon.Symbol = ModernWpf.Controls.Symbol.Play;
                _playIconText.Text = "Play";
            }

            // Update path scrubber state
            _pathScrubber.IsEnabled = pathsExist &&
                                      validSelection &&
                                      nodeCount >= 2 &&
                                      (PMState is not PathManagerState.gotoNode);

            _pathProgressText.IsEnabled = pathsExist &&
                                      validSelection &&
                                      nodeCount >= 2 &&
                                      (PMState is not PathManagerState.gotoNode);

            // Special case for B-Spline toggle
            _interpolationMode.SetToggleEnabledByLabel("BSpline", nodeCount >= 4);
        }

        public void setIncomingPath(string path)
        {
            Dispatcher.Invoke(() =>
                SelectNewPath(path));
            _incomingSelectedPath = path;
        }

        private void expRelativePlayer_Expanded(object sender, RoutedEventArgs e)
        {
            // Check the bool input if the expander is expanded
            _relativePlayer.Value = true;
        }

        private void expRelativePlayer_Collapsed(object sender, RoutedEventArgs e)
        {
            // Uncheck if collapsed
            _relativePlayer.Value = false;
        }

        private void RelativePlayer_ValueChanged(object sender, EventArgs e)
        {
            // If the user toggles the checkbox, expand/collapse the expander
            expRelativePlayer.IsExpanded = _relativePlayer.Value;

            // Also enable/disable the slider
            _relativePathSmoothness.IsEnabled = _relativePlayer.Value;
        }

        private void expPathLookAt_Expanded(object sender, RoutedEventArgs e)
        {
            // Check the bool input if the expander is expanded
            _togglePathLookat.Value = true;
        }

        private void expPathLookAt_Collapsed(object sender, RoutedEventArgs e)
        {
            // Uncheck if collapsed
            _togglePathLookat.Value = false;
        }

        private void PathLookat_ValueChanged(object sender, EventArgs e)
        {
            // If the user toggles the checkbox, expand/collapse the expander
            expPathLookAt.IsExpanded = _togglePathLookat.Value;

            // Also enable/disable the slider
            _lookatOffsetX.IsEnabled = _togglePathLookat.Value;
            _lookatOffsetY.IsEnabled = _togglePathLookat.Value;
            _lookatOffsetZ.IsEnabled = _togglePathLookat.Value;
            _lookatSmoothness.IsEnabled = _togglePathLookat.Value;
        }

        private void expAdaptiveSpeed_Expanded(object sender, RoutedEventArgs e)
        {
            // Check the bool input if the expander is expanded
            _toggleAdaptivePathSpeed.Value = true;
        }

        private void expAdaptiveSpeed_Collapsed(object sender, RoutedEventArgs e)
        {
            // Uncheck if collapsed
            _toggleAdaptivePathSpeed.Value = false;
        }

        private void adaptivePathSpeed_ValueChanged(object sender, EventArgs e)
        {
            // If the user toggles the checkbox, expand/collapse the expander
            expAdaptiveSpeed.IsExpanded = _toggleAdaptivePathSpeed.Value;

            // Also enable/disable the slider
            _pathSpeedScale.IsEnabled = _toggleAdaptivePathSpeed.Value;
            _pathSpeedSmoothness.IsEnabled = _toggleAdaptivePathSpeed.Value;
            _pathSpeedMin.IsEnabled = _toggleAdaptivePathSpeed.Value;
            _pathSpeedMax.IsEnabled = _toggleAdaptivePathSpeed.Value;
            _pathSpeedBaseline.IsEnabled = _toggleAdaptivePathSpeed.Value;
        }

        private void InitializeUIRefreshTimer(int intervalInMilliseconds)
        {
            if (_uiRefreshTimer != null)
            {
                // If it already exists, stop and dispose to avoid duplication.
                _uiRefreshTimer.Stop();
                _uiRefreshTimer = null;
            }

            _uiRefreshTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(intervalInMilliseconds)
            };
            _uiRefreshTimer.Tick += (sender, args) => RefreshUI();
            _uiRefreshTimer.Start();
        }

        public void StopUIRefreshTimer()
        {
            if (_uiRefreshTimer == null) return;
            _uiRefreshTimer.Stop();
            _uiRefreshTimer = null;
        }

        private void RefreshUI()
        {
            UpdateUIBasedOnNodeCount();
        }


        private void SetPlaceholder()
        {
            txtPathName.Text = "Optional";
            txtPathName.FontStyle = FontStyles.Italic;
            txtPathName.SetResourceReference(ForegroundProperty, "SystemControlForegroundBaseMediumLowBrush");
        }

        private void TxtPathName_GotFocus(object sender, RoutedEventArgs e)
        {
            // If showing placeholder, clear it when focused
            if (txtPathName.FontStyle != FontStyles.Italic || txtPathName.Text != "Optional") return;
            txtPathName.Text = "";
            txtPathName.FontStyle = FontStyles.Normal;
            txtPathName.SetResourceReference(ForegroundProperty, "SystemControlForegroundBaseHighBrush");
        }

        private void TxtPathName_LostFocus(object sender, RoutedEventArgs e)
        {
            // If empty when losing focus, show placeholder again
            if (string.IsNullOrEmpty(txtPathName.Text))
            {
                SetPlaceholder();
            }
        }

        public void UpdatePathPlayingFlag(byte flag)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => UpdatePathPlayingFlag(flag));
                return;
            }

            PMState = flag switch
            {
                PathManagerState.Idle => 0,
                PathManagerState.PlayingPath => 1,
                PathManagerState.gotoNode => 2,
                PathManagerState.Scrubbing => 3,
                _ => PMState
            };
        }

        public int PMState
        {
            get => _pmState;
            set
            {
                _pmState = value;
                UpdatePlayControls();
            }
        }

        public void UpdateCameraEnabledFlag(byte flag)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => UpdateCameraEnabledFlag(flag));
                return;
            }

            IsCameraEnabled = (flag == 1);
        }

        public bool IsCameraEnabled
        {
            get => _isCameraEnabled;
            set
            {
                _isCameraEnabled = value;
                UpdateControllerCameraToggled();
            }
        }


        public void UpdateVisualisationFlag(byte flag)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => UpdateVisualisationFlag(flag));
                return;
            }

            IsVisualisationEnabled = (flag == 1);
        }

        public bool IsVisualisationEnabled
        {
            get => _isVisualisationEnabled;
            set
            {
                _isVisualisationEnabled = value;

                // Update the visualisation state in ViewModel or other relevant place.
                UpdateVisualisation(value);
            }
        }

        private void UpdateVisualisation(bool flag)
        {
            // Ensure this code runs on the UI thread.
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => UpdateVisualisation(flag));
                return;
            }

            _toggleVisualisation.SetValueSilently(flag);
        }

        private void UpdateControllerCameraToggled()
        {
            // Set the entire form's enabled state based on _isCameraEnabled
            this.IsEnabled = _isCameraEnabled;
        }

        private void UpdatePlayControls()
        {
            UpdateButtonStates();
        }

        private void btnRefreshPath_Click(object sender, RoutedEventArgs e)
        {
            // 1. Get the path name from the selected item.
            var pathName = GetSelectedPathName();
            if (string.IsNullOrEmpty(pathName))
            {
                MessageBox.Show("No valid path is selected.", "Refresh Path", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // 2. Convert the path name to a byte array (using ASCII encoding).
            var payload = Encoding.ASCII.GetBytes(pathName);

            // 3. Send the payload to the DLL with an action type for refreshing the path.
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.refreshPath, payload);

            // Log the event.
            //LogHandlerSingleton.Instance().LogLine("RefreshPath message sent for path: " + pathName, "PathController", true);
        }

        private void TreeView_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            // Determine what was double-clicked.
            // We assume the selected item is a NodeViewModel.
            var selected = _pathTreeView.SelectedItem;
            if (selected is NodeViewModel nodeVM)
            {
                // 1) Get the index of the node.
                var nodeIndex = nodeVM.Index;

                // 2) Find the parent CameraPathViewModel.
                var parentPath = ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(nodeVM));
                if (parentPath == null)
                {
                    MessageBox.Show("Parent path not found.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return;
                }
                var pathName = parentPath.PathName;

                // 3) Create the payload: first byte is the node index, then the path name (ASCII encoded).
                var indexByte = (byte)nodeIndex;
                var pathNameBytes = Encoding.ASCII.GetBytes(pathName);
                var payload = new byte[1 + pathNameBytes.Length];
                payload[0] = indexByte;
                Buffer.BlockCopy(pathNameBytes, 0, payload, 1, pathNameBytes.Length);

                // 4) Send the payload to the DLL using the new action type.
                MessageHandlerSingleton.Instance().SendPathAction(PathActionType.gotoNode, payload);
               // LogHandlerSingleton.Instance().LogLine($"goToNode message sent for path: {pathName} with node index: {nodeIndex}", "PathController", true);
            }
            else
            {
                // Optionally: If a NodeDetail was double-clicked, you could decide to find its parent node and then the parent path.
                // For now, we only handle NodeViewModel.
                MessageBox.Show("Double-click a node (not a detail) to perform this action.", "Invalid Selection", MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        private void btnDeleteNode_Click(object sender, RoutedEventArgs e)
        {
            // Check throttle
            if (!CanExecuteAction("DeleteNode"))
                return;

            // Disable button immediately
            _deleteNode.IsEnabled = false;

            var selected = _pathTreeView.SelectedItem;
            if (selected == null)
            {
                MessageBox.Show("No valid selection.", "Delete Node", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            string pathName = null;
            var nodeIndex = 255; // Default value for a full path selection.

            switch (selected)
            {
                case CameraPathViewModel cpvm:
                    // If the top-level (path) is selected, we use its name and send -1.
                    pathName = cpvm.PathName;
                    nodeIndex = 255;
                    break;
                case NodeViewModel nvm:
                {
                    // If a node is selected, get the node index and find its parent path.
                    nodeIndex = nvm.Index;
                    var parentPath = ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(nvm));
                    if (parentPath != null)
                    {
                        pathName = parentPath.PathName;
                    }

                    break;
                }
                case NodeDetail nd:
                {
                    // If a node detail is selected, first find the parent NodeViewModel...
                    var parentNode = ViewModel.CameraPaths.SelectMany(p => p.Nodes).FirstOrDefault(n => n.Details.Contains(nd));
                    if (parentNode != null)
                    {
                        nodeIndex = parentNode.Index;
                        var parentPath = ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(parentNode));
                        if (parentPath != null)
                        {
                            pathName = parentPath.PathName;
                        }
                    }

                    break;
                }
            }

            // Verify we have a valid path name.
            if (string.IsNullOrEmpty(pathName))
            {
                MessageBox.Show("No valid path is selected.", "Delete Node", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // Build the payload:
            // First, convert the nodeIndex to 4 bytes.
            var indexByte = (byte)nodeIndex;
            var pathNameBytes = Encoding.ASCII.GetBytes(pathName);
            var payload = new byte[1 + pathNameBytes.Length];
            payload[0] = indexByte;
            Buffer.BlockCopy(pathNameBytes, 0, payload, 1, pathNameBytes.Length);

            // Send the payload using your message handler with a specific action for delete node.
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.deleteNode, payload);

            //LogHandlerSingleton.Instance().LogLine(
            //    $"DeleteNode message sent for path: {pathName}, node index: {nodeIndex}",
            //    "PathController", true);

            // Re-enable after a delay (using dispatcher timer)
            Task.Delay(ACTION_THROTTLE_MS).ContinueWith(_ =>
                Dispatcher.Invoke(UpdateButtonStates));

        }

        private void btnUpdateNode_Click(object sender, EventArgs e)
        {
            // Check throttle
            if (!CanExecuteAction("UpdateNode"))
                return;

            // Disable button immediately
            _updateNode.IsEnabled = false;

            var selected = _pathTreeView.SelectedItem;
            switch (selected)
            {
                case null:
                    MessageBox.Show("No valid selection.", "Delete Node", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return;
                case CameraPathViewModel cpvm:
                    MessageBox.Show("Not a Node. Please select a node", "Update Node", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return;
            }

            string pathName = null;
            var nodeIndex = 255; // Default value for a full path selection.

            switch (selected)
            {
                case NodeViewModel nvm:
                {
                    // If a node is selected, get the node index and find its parent path.
                    nodeIndex = nvm.Index;
                    var parentPath = ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(nvm));
                    if (parentPath != null)
                    {
                        pathName = parentPath.PathName;
                    }

                    break;
                }
                case NodeDetail nd:
                {
                    // If a node detail is selected, first find the parent NodeViewModel...
                    var parentNode = ViewModel.CameraPaths.SelectMany(p => p.Nodes).FirstOrDefault(n => n.Details.Contains(nd));
                    if (parentNode != null)
                    {
                        nodeIndex = parentNode.Index;
                        var parentPath = ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(parentNode));
                        if (parentPath != null)
                        {
                            pathName = parentPath.PathName;
                        }
                    }

                    break;
                }
            }

            // Verify we have a valid path name.
            if (string.IsNullOrEmpty(pathName))
            {
                MessageBox.Show("No valid path is selected.", "Update Node", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // Build the payload:
            // First, convert the nodeIndex to 4 bytes.
            var indexByte = (byte)nodeIndex;
            var pathNameBytes = Encoding.ASCII.GetBytes(pathName);
            var payload = new byte[1 + pathNameBytes.Length];
            payload[0] = indexByte;
            Buffer.BlockCopy(pathNameBytes, 0, payload, 1, pathNameBytes.Length);

            // Send the payload using your message handler with a specific action for delete node.
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.updateNode, payload);

            //LogHandlerSingleton.Instance().LogLine($"UpdateNode message sent for path: {pathName}, node index: {nodeIndex}", "PathController", true);

            // Re-enable after a delay (using dispatcher timer)
            Task.Delay(ACTION_THROTTLE_MS).ContinueWith(_ =>
                Dispatcher.Invoke(UpdateButtonStates));
        }


        private void CameraPaths_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            UpdateButtonStates();
        }

        private void ToggleDetails_ValueChanged(object sender, EventArgs e)
        {
            // Retrieve the current toggle value.
            var showDetails = _toggleDetails.Value;

            // Find the NodeTemplateSelector in the resources.
            if (this.Resources["NodeTemplateSelector"] is not NodeTemplateSelector selector) return;
            // Update the flag.
            selector.ShowDetails = showDetails;

            // Force the TreeView to refresh its items.
            RefreshTreeView();
        }


        /// <summary>
        /// Event handler for when the TreeView selection changes.
        /// Enables the delete button if a valid path or node is selected; disables it otherwise.
        /// </summary>
        private void _pathTreeView_SelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
        {
            // Update all button states
            UpdateButtonStates();

            // Only update path and node info if we have a valid selection

            // Get the current path name and node index
            var currentPathName = GetSelectedPathName();
            var currentNodeIndex = GetSelectedNodeIndex();

            // Only update path if we have a valid path name
            if (!string.IsNullOrEmpty(currentPathName) && _previousPathName != currentPathName)
            {
                // Important: Since we checked for null already, we won't get a warning message
                UpdateSelectedPath();
            }

            // Only update node index if we have a valid node
            if (currentNodeIndex != -1 && _previousSelectedNode != currentNodeIndex)
            {
                UpdateSelectedNodeIndex();
            }

            // Update the previous values
            _previousPathName = currentPathName ?? string.Empty;
            _previousSelectedNode = currentNodeIndex;

            // Reset scrubber when changing paths
            if (currentPathName != _previousPathName)
            {
                _pathScrubber.Value = 0;
                UpdatePathPositionText(0);
            }
        }

        private void UpdateSelectedPath()
        {
            // Get the currently selected path name.
            var pathName = GetSelectedPathName();
            if (string.IsNullOrEmpty(pathName))
            {
                MessageBox.Show("No valid path is selected.", "Update Path", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            // Send the update message to the DLL.
            var payload = Encoding.ASCII.GetBytes(pathName);
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.selectedPathUpdate, payload);
            // Optionally log the event.
           // LogHandlerSingleton.Instance().LogLine("selectedPath message sent for path: " + pathName, "PathController", true);
        }

        private void UpdateSelectedNodeIndex()
        {
            var pathName = GetSelectedPathName();
            var nodeindex = GetSelectedNodeIndex();
            var payload = new byte[1];
            payload[0] = (byte)nodeindex;
            if (GetSelectedNodeIndex() == -1)
            {
                MessageBox.Show("No valid node is selected.", "Select Valid Node", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            // Send the update message to the DLL.
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.selectedNodeIndexUpdate, payload);
            // Optionally log the event.
            //LogHandlerSingleton.Instance().LogLine("selectedNodeIndex message sent for path: " + pathName +" - with index " + nodeindex, "PathController", true);
        }

        /// <summary>
        /// Helper method that returns the PathName from the currently selected item,
        /// whether it's a CameraPathViewModel, NodeViewModel, or NodeDetail.
        /// </summary>
        private string GetSelectedPathName()
        {
            var selected = _pathTreeView.SelectedItem;
            switch (selected)
            {
                case CameraPathViewModel cpvm:
                    return cpvm.PathName;
                case NodeViewModel nvm:
                {
                    // Find the parent CameraPathViewModel that contains this node.
                    var parentPath = this.ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(nvm));
                    return parentPath?.PathName;
                }
                case NodeDetail nd:
                {
                    // Find the NodeViewModel that contains this NodeDetail.
                    var parentNode = this.ViewModel.CameraPaths
                        .SelectMany(p => p.Nodes)
                        .FirstOrDefault(n => n.Details.Contains(nd));
                    if (parentNode != null)
                    {
                        var parentPath = this.ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(parentNode));
                        return parentPath?.PathName;
                    }

                    break;
                }
            }
            return null;
        }

        private int GetSelectedNodeIndex()
        {
            var selected = _pathTreeView.SelectedItem;
            switch (selected)
            {
                case CameraPathViewModel cpvm:
                    return 0; // default to first node for path selection
                case NodeViewModel nvm:
                    return nvm.Index;
                case NodeDetail nd:
                {
                    // Find the NodeViewModel that contains this NodeDetail.
                    var parentNode = this.ViewModel.CameraPaths
                        .SelectMany(p => p.Nodes)
                        .FirstOrDefault(n => n.Details.Contains(nd));
                    if (parentNode != null)
                    {
                        return parentNode.Index;
                    }

                    break;
                }
            }
            return -1; // Default value for no valid node index.
        }

        /// <summary>
        /// Selects the next path in the TreeView collection relative to the currently selected one.
        /// If no path is currently selected, it selects the first path.
        /// If the last path is selected, it wraps around to the first path.
        /// </summary>
        public void SelectNextPath()
        {
            // Ensure we're on the UI thread
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(SelectNextPath);
                return;
            }

            // If there are no paths, nothing to do
            if (ViewModel.CameraPaths.Count == 0)
                return;

            // Get the currently selected path name
            var currentPathName = GetSelectedPathName();

            // Find the current path's index in the collection
            var currentIndex = -1;
            if (!string.IsNullOrEmpty(currentPathName))
            {
                currentIndex = ViewModel.CameraPaths.ToList().FindIndex(
                    p => string.Equals(p.PathName, currentPathName, StringComparison.OrdinalIgnoreCase));
            }

            // Determine the index of the next path (with wraparound)
            int nextIndex;
            if (currentIndex == ViewModel.CameraPaths.Count - 1 || currentIndex == -1)
            {
                // If this is the last path or no path is selected, wrap around to the first path
                nextIndex = 0;
            }
            else
            {
                // Otherwise, select the next path
                nextIndex = currentIndex + 1;
            }

            // Get the next path
            var nextPath = ViewModel.CameraPaths[nextIndex];

            // Get the container and programmatically select it
            if (_pathTreeView.ItemContainerGenerator.ContainerFromItem(nextPath) is TreeViewItem container)
            {
                // Set focus to the container first
                container.Focus();

                // Set IsSelected to true - this will trigger selection changed events
                container.IsSelected = true;

                // Make sure it's visible
                container.BringIntoView();

                //LogHandlerSingleton.Instance().LogLine($"Selected next path: {nextPath.PathName}", "PathController", true);
            }
            else
            {
                LogHandlerSingleton.Instance().LogLine($"Failed to select next path - container was null", "PathController", false, true);
            }

            //LogHandlerSingleton.Instance().LogLine($"Selected next path: {nextPath.PathName}", "PathController", true);
        }

        private void btnAddNode_Click(object sender, RoutedEventArgs e)
        {
            // Check throttle
            if (!CanExecuteAction("AddNode"))
                return;

            // Disable button immediately
            _addNode.IsEnabled = false;

            // Boundary check
            var cpvm = ViewModel.CameraPaths
                .FirstOrDefault(p => p.PathName == GetSelectedPathName());
            if (cpvm != null && cpvm.Nodes.Count >= MaxNodesPerPath)
            {
                MessageBox.Show(
                    $"Each path may contain at most {MaxNodesPerPath} nodes.",
                    "Node Limit Reached",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var pathName = GetSelectedPathName();
            if (string.IsNullOrEmpty(pathName))
            {
                MessageBox.Show("No valid path is selected.", "Add Node", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var selected = _pathTreeView.SelectedItem;

            switch (selected)
            {
                case CameraPathViewModel:
                {
                    // Convert the path name to a byte array.
                    var payload = Encoding.ASCII.GetBytes(pathName);

                    // Send the message using an action type for adding a node.
                    MessageHandlerSingleton.Instance().SendPathAction(PathActionType.addNode, payload);

                    //LogHandlerSingleton.Instance().LogLine("AddNode message sent for path: " + pathName, "PathController", true);
                    break;
                }
                case NodeViewModel nodeVM:
                {
                    // Insert node before the selected node
                    var nodeIndex = nodeVM.Index;

                    // Create payload: first byte is the node index, then the path name
                    var indexByte = (byte)nodeIndex;
                    var pathNameBytes = Encoding.ASCII.GetBytes(pathName);
                    var payload = new byte[1 + pathNameBytes.Length];
                    payload[0] = indexByte;
                    System.Buffer.BlockCopy(pathNameBytes, 0, payload, 1, pathNameBytes.Length);

                    // Send the message with the new action type
                    MessageHandlerSingleton.Instance().SendPathAction(PathActionType.insertNode, payload);
                    //LogHandlerSingleton.Instance().LogLine($"InsertNodeBefore message sent for path: {pathName}, node index: {nodeIndex}", "PathController", true);
                    break;
                }
                case NodeDetail nd:
                {
                    // If a node detail is selected, find the parent node
                    var parentNode = ViewModel.CameraPaths.SelectMany(p => p.Nodes).FirstOrDefault(n => n.Details.Contains(nd));
                    if (parentNode == null) return;

                    //get parentNode
                    var nodeIndex = parentNode.Index;
                    // Create payload: first byte is the node index, then the path name
                    var indexByte = (byte)nodeIndex;
                    var pathNameBytes = Encoding.ASCII.GetBytes(pathName);
                    var payload = new byte[1 + pathNameBytes.Length];
                    payload[0] = indexByte;
                    System.Buffer.BlockCopy(pathNameBytes, 0, payload, 1, pathNameBytes.Length);

                    // Send the message with the new action type
                    MessageHandlerSingleton.Instance().SendPathAction(PathActionType.insertNode, payload);
                    //LogHandlerSingleton.Instance().LogLine($"InsertNodeBefore message sent for path: {pathName}, node index: {nodeIndex}", "PathController", true);
                    break;
                }
            }

            Task.Delay(ACTION_THROTTLE_MS).ContinueWith(_ =>
                Dispatcher.Invoke(UpdateButtonStates));

        }

        private void btnDeletePath_Click(object sender, RoutedEventArgs e)
        {
            // Check throttle
            if (!CanExecuteAction("DeletePath"))
                return;

            // Disable button immediately
            _deletePath.IsEnabled = false;

            var pathName = GetSelectedPathName();

            if (string.IsNullOrEmpty(pathName))
            {
                MessageBox.Show("No valid path is selected.", "Delete Path", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // Convert the path name to a byte array.
            var payload = Encoding.ASCII.GetBytes(pathName);

            // Send the message using an action type for deleting a path.
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.deletePath, payload);

            //LogHandlerSingleton.Instance().LogLine("Delete Path message sent for path: " + pathName, "PathController", true);

            Task.Delay(ACTION_THROTTLE_MS).ContinueWith(_ =>
                Dispatcher.Invoke(UpdateButtonStates));
        }

        private void btnPlayPath_Click(object sender, RoutedEventArgs e)
        {
            // Retrieve the path name from the currently selected item.
            var pathName = GetSelectedPathName();

            // If no valid path name is found, warn the user.
            if (string.IsNullOrEmpty(pathName))
            {
                MessageBox.Show("No valid path is selected.", "Play Path", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (PMState is PathManagerState.Idle)
            {
                // User requests to start playing the path.
                var payload = Encoding.ASCII.GetBytes(pathName);
                MessageHandlerSingleton.Instance().SendPathAction(PathActionType.playPath, payload);
               // LogHandlerSingleton.Instance().LogLine("PlayPath message sent for path: " + pathName, "PathController", true);
            }
            else
            {
                // User requests to stop playing the path.
                var payload = Encoding.ASCII.GetBytes(pathName);
                MessageHandlerSingleton.Instance().SendPathAction(PathActionType.stopPath, payload);
                //LogHandlerSingleton.Instance().LogLine("StopPath message sent for path: " + pathName, "PathController", true);
            }

        }

        private void UpdateUIBasedOnNodeCount()
        {
            UpdateButtonStates();

            var nodeCount = GetNodeCountOfCurrentlySelectedPath();
            if (nodeCount < 4 && _interpolationMode.Value == 3)
                _interpolationMode.Value = 0;

        }

        private void interpolationMode_ValueChanged(object sender, EventArgs e)
        {
            if (_interpolationMode.Value == 3)
            {
                _rotationMode.Value = 0;
                _rotationMode.SetToggleEnabledByLabel("SQUAD", false);
                _rotationMode.SetToggleEnabledByLabel("Riemann Cubic", false);
            }
            else
            {
                _rotationMode.SetToggleEnabledByLabel("SQUAD", true);
                _rotationMode.SetToggleEnabledByLabel("Riemann Cubic", true);
                _rotationMode.SetToggleEnabledByLabel("Standard", true);
            }
        }


        private int GetNodeCountOfCurrentlySelectedPath()
        {
            var selected = _pathTreeView.SelectedItem;
            switch (selected)
            {
                case CameraPathViewModel cpvm:
                    // If the user selected a path, just return its node count.
                    return cpvm.Nodes.Count;
                case NodeViewModel nvm:
                {
                    // If a node is selected, find its parent path and return that path's node count.
                    var parentPath = ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(nvm));
                    if (parentPath != null)
                    {
                        return parentPath.Nodes.Count;
                    }

                    break;
                }
                case NodeDetail nd:
                {
                    // If a detail is selected, find the parent node, then the parent path, then return the path's node count.
                    var parentNode = ViewModel.CameraPaths
                        .SelectMany(p => p.Nodes)
                        .FirstOrDefault(n => n.Details.Contains(nd));
                    if (parentNode != null)
                    {
                        var parentPath = ViewModel.CameraPaths.FirstOrDefault(p => p.Nodes.Contains(parentNode));
                        if (parentPath != null)
                        {
                            return parentPath.Nodes.Count;
                        }
                    }

                    break;
                }
            }
            // If no valid path is found, return 0.
            return 0;
        }

        private void RefreshTreeView()
        {
            // Save the current expansion state of the TreeView.
            var state = TreeViewStateHelper.SaveState(_pathTreeView);
            
            // A simple way to force the TreeView to refresh is to reset its ItemsSource.
            var items = _pathTreeView.ItemsSource;
            _pathTreeView.ItemsSource = null;
            _pathTreeView.ItemsSource = items;

            // Restore the previously saved expansion state.
            TreeViewStateHelper.RestoreState(_pathTreeView, state);

        }

        private void DeltaType_ValueChanged(object sender, EventArgs e)
        {
            _deltaValue.IsEnabled = _deltaType.Value;
        }



        private void btnCreatePath_Click(object sender, RoutedEventArgs e)
        {
            // Check throttle
            if (!CanExecuteAction("Createpath"))
                return;

            // Disable button immediately
            _createPath.IsEnabled = false;

            // trim and length‐check
            // Get the text from the TextBox.
            var pathName = txtPathName.Text.Trim();

            if (pathName.Length < MinPathNameLength || pathName.Length > MaxPathNameLength)
            {
                MessageBox.Show(
                    $"Path name must be between {MinPathNameLength} and {MaxPathNameLength} characters.",
                    "Invalid Path Name",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            if (ViewModel.CameraPaths.Count >= MaxTotalPaths)
            {
                MessageBox.Show(
                    $"You may create at most {MaxTotalPaths} paths.",
                    "Limit Reached",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // If the TextBox is empty, generate a default name.
            if (string.IsNullOrEmpty(pathName) || pathName=="Optional")
            {
                pathName = GenerateDefaultPathName(ViewModel);
            }

            // Check if the path already exists (case-insensitive check).
            var alreadyExists = ViewModel.CameraPaths.Any(p =>
                string.Equals(p.PathName, pathName, System.StringComparison.OrdinalIgnoreCase));

            if (alreadyExists)
            {
                // Show the error message.
                lblPathExists.Visibility = Visibility.Visible;
                lblPathExists.Content = $"Path \"{pathName}\" already exists.";
                return;
            }
            else
            {
                // Hide the error message (if visible).
                lblPathExists.Visibility = Visibility.Collapsed;
            }

            // At this point, the path name is unique.
            // Send the new path name to the DLL via message handler.
            // Convert the path name to bytes (using ASCII, or UTF8 if needed).
            var payload = Encoding.ASCII.GetBytes(pathName);

            // Create and send the message.
            MessageHandlerSingleton.Instance().SendPathAction(IGCSClient.PathActionType.addPath, payload);

            // Optionally, log the event.
            //LogHandlerSingleton.Instance().LogLine("CreateNewPath message sent with path name: " + pathName, "PathManager", true);

            // Clear the TextBox.
            txtPathName.Text = string.Empty;
            SetPlaceholder();

            // Re-enable after a delay (using dispatcher timer)
            Task.Delay(ACTION_THROTTLE_MS).ContinueWith(_ =>
                Dispatcher.Invoke(UpdateButtonStates));
        }

        private void SelectNewPath(string pathName)
        {
            // Find the new path in the ViewModel.
            var newPathVM = ViewModel.CameraPaths.FirstOrDefault(p => p.PathName.Equals(pathName, StringComparison.OrdinalIgnoreCase));
            if (newPathVM == null)
            {
                LogHandlerSingleton.Instance().LogLine("New path not found in ViewModel.", "PathController", false, true);
                return;
            }

            // Force layout update so that the container is generated.
            _pathTreeView.UpdateLayout();

            // Try to get the TreeViewItem for the new path.
            if (_pathTreeView.ItemContainerGenerator.ContainerFromItem(newPathVM) is TreeViewItem container)
            {
                container.IsSelected = true;
                container.IsExpanded = true;
                container.BringIntoView();
            }
            else
            {
                // If the container is still not generated, you might need to delay further.
                Dispatcher.BeginInvoke(new Action(() =>
                {
                    _pathTreeView.UpdateLayout();
                    container = _pathTreeView.ItemContainerGenerator.ContainerFromItem(newPathVM) as TreeViewItem;
                    if (container == null) return;
                    container.IsExpanded = true;
                    container.IsSelected = true;
                    container.BringIntoView();
                }), DispatcherPriority.Background);
            }
        }


        private string GenerateDefaultPathName(MainViewModel viewModel)
        {
            const string baseName = "Path";
            // Get all existing names that start with the baseName.
            var existingNames = viewModel.CameraPaths
                .Select(p => p.PathName)
                .Where(n => n.StartsWith(baseName))
                .ToList();
            var maxNumber = 0;
            foreach (var numberPart in existingNames.Select(name => name.Substring(baseName.Length)))
            {
                if (!int.TryParse(numberPart, out var number)) continue;
                if (number > maxNumber)
                    maxNumber = number;
            }
            // Return the base name with the next number.
            return baseName + (maxNumber + 1);
        }



        internal void Setup()
        {
            // Initialize scrubber
            _pathScrubber.Value = 0;
            _pathScrubber.IsEnabled = false;

            var settings = AppStateSingleton.Instance().Settings;
            foreach (var setting in settings)
            {
                switch (setting.ID)
                {
                    case SettingType.PathDuration:
                        setting.Setup(_pathDuration);
                        break;
                    case SettingType.PathEasingValue:
                        setting.Setup(_easingValue);
                        break;
                    case SettingType.DeltaType:
                        setting.Setup(_deltaType);
                        break;
                    case SettingType.DeltaValue:
                        setting.Setup(_deltaValue);
                        break;
                    case SettingType.UniformParam:
                        setting.Setup(_constantSpeed);
                        break;
                    case SettingType.EulerOrder:
                        setting.Setup(_eulerOrder);
                        break;
                    case SettingType.PathToggleInterpolationMode:
                        setting.Setup(_interpolationMode);
                        break;
                    case SettingType.RotationMode:
                        setting.Setup(_rotationMode);
                        break;
                    case SettingType.CameraShakeToggle:
                        setting.Setup(_enableShake);
                        break;
                    case SettingType.Amplitude:
                        setting.Setup(_shakeAmplitude);
                        break;
                    case SettingType.Frequency:
                        setting.Setup(_shakeFrequency);
                        break;
                    case SettingType.HandheldCameraToggle:
                        setting.Setup(_enablehandheld);
                        break;
                    case SettingType.HandheldIntensity:
                        setting.Setup(_handheldIntensity);
                        break;
                    case SettingType.HandheldDriftIntensity:
                        setting.Setup(_handheldDriftIntensity);
                        break;
                    case SettingType.HandheldJitterIntensity:
                        setting.Setup(_handheldJitterIntensity);
                        break;
                    case SettingType.HandheldBreathingIntensity:
                        setting.Setup(_handheldBreathingIntensity);
                        break;
                    case SettingType.HandheldBreatingRate:
                        setting.Setup(_handheldBreathingRate);
                        break;
                    case SettingType.HandheldPositionToggle:
                        setting.Setup(_handheldPositionToggle);
                        break;
                    case SettingType.HandheldRotationToggle:
                        setting.Setup(_handheldRotationToggle);
                        break;
                    case SettingType.PlayerRelativeToggle:
                        setting.Setup(_relativePlayer);
                        break;
                    case SettingType.PathEasingType:
                        setting.Setup(_easingPanel);
                        break;
                    case SettingType.PathSampleCount:
                        setting.Setup(_sampleSizePanel);
                        break;
                    case SettingType.PlayerRelativeSmoothness:
                        setting.Setup(_relativePathSmoothness);
                        break;
                    case SettingType.WaitBeforePlaying:
                        setting.Setup(_waitbeforePlaying);
                        break;
                    case SettingType.UnpauseOnPlay:
                        setting.Setup(_resumeonPlay);
                        break;
                    case SettingType.HandheldDriftSpeed:
                        setting.Setup(_handheldDriftSpeed);
                        break;
                    case SettingType.HandheldRotationDriftSpeed:
                        setting.Setup(_handheldRotationDriftSpeed);
                        break;
                    case SettingType.VisualisationToggle:
                        setting.Setup(_toggleVisualisation);
                        break;
                    case SettingType.VisualiseAllPaths:
                        setting.Setup(_toggleDisplayAll);
                        break;
                    case SettingType.ScrubbingProgress:
                        setting.Setup(_pathScrubber);
                        break;
                    case SettingType.PathLookAtEnabled:
                        setting.Setup(_togglePathLookat);
                        break;
                    case SettingType.PathLookAtOffsetX:
                        setting.Setup(_lookatOffsetX);
                        break;
                    case SettingType.PathLookAtOffsetY:
                        setting.Setup(_lookatOffsetY);
                        break;
                    case SettingType.PathLookAtOffsetZ:
                        setting.Setup(_lookatOffsetZ);
                        break;
                    case SettingType.PathLookAtSmoothness:
                        setting.Setup(_lookatSmoothness);
                        break;
                    case SettingType.PathSpeedMatchingEnabled:
                        setting.Setup(_toggleAdaptivePathSpeed);
                        break;
                    case SettingType.PathSpeedScale:
                        setting.Setup(_pathSpeedScale);
                        break;
                    case SettingType.PathSpeedSmoothness:
                        setting.Setup(_pathSpeedSmoothness);
                        break;
                    case SettingType.PathMinSpeed:
                        setting.Setup(_pathSpeedMin);
                        break;
                    case SettingType.PathMaxSpeed:
                        setting.Setup(_pathSpeedMax);
                        break;
                    case SettingType.PathBaselineSpeed:
                        setting.Setup(_pathSpeedBaseline);
                        break;
                    default:
                        break;
                }

            }
        }

        private void PathScrubber_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            lock (_scrubberLock)
            {
                _isScrubbing = true;
            }

            // Send message to enter scrubbing state
            var payload = new byte[1];
            payload[0] = 1; // 1 indicates scrubbing started
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.pathScrubbing, payload);
        }

        private void PathScrubber_PreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            lock (_scrubberLock)
            {
                _isScrubbing = false;
            }

            // Send message to exit scrubbing state
            var payload = new byte[1];
            payload[0] = 0; // 0 indicates scrubbing ended
            MessageHandlerSingleton.Instance().SendPathAction(PathActionType.pathScrubbing, payload);
        }

        public void UpdatePathProgress(float progress)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => UpdatePathProgress(progress));
                return;
            }

            UpdatePathPositionText(progress);

            lock (_scrubberLock)
            {
                if (_isScrubbing)
                    return; // Don't update slider while scrubbing
            }

            _isUpdatingSlider = true;
            try
            {
                _pathScrubber.SetValueSilently(progress);
                
            }
            finally
            {
                _isUpdatingSlider = false;
            }
        }

        private void UpdatePathPositionText(float progress)
        {
            // Fix precision issues - clamp and round
            progress = Math.Max(0.0f, Math.Min(1.0f, progress));
            var percentage = progress * 100f;
            _pathProgressText.Text = $"Progress: {percentage:F1}%";
        }
    }
}
