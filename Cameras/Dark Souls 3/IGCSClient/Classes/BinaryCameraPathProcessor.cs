using System;
using System.Collections.Generic;
using System.Windows;
using System.Runtime.InteropServices;
using IGCSClient.Classes;
using IGCSClient.Models;
using IGCSClient.ViewModels;
using IGCSClient.Forms;

namespace IGCSClient.Helpers
{
    public static class BinaryCameraPathProcessor
    {
        public static void HandleBinaryPathData(byte[] data)
        {
            try
            {
                // Skip message type byte to get just the binary data
                byte[] binaryData = new byte[data.Length - 1];
                Array.Copy(data, 1, binaryData, 0, data.Length - 1);

                // Process on UI thread
                Application.Current.Dispatcher.Invoke(() =>
                {
                    var pathControllerWindow = MainWindow.CurrentPathControllerWindow;
                    if (pathControllerWindow != null)
                    {
                        MainViewModel mainViewModel = pathControllerWindow.PathControllerInstance.ViewModel;
                        ProcessBinaryPathData(binaryData, mainViewModel, pathControllerWindow.TreeViewControl);
                    }
                    else
                    {
                        LogHandlerSingleton.Instance().LogLine(
                            "PathControllerWindow is not open.",
                            "MessageHandler", false, true);
                    }
                });
            }
            catch (Exception ex)
            {
                LogHandlerSingleton.Instance().LogLine(
                    "Exception processing binary path data: " + ex.Message,
                    "MessageHandler", false, true);
            }
        }

        private static void ProcessBinaryPathData(byte[] data, MainViewModel viewModel, System.Windows.Controls.TreeView treeView)
        {
            try
            {
                int position = 0;

                // Read the main header
                BinaryPathHeader header = BinaryUtils.BytesToStruct<BinaryPathHeader>(data, position);
                position += Marshal.SizeOf<BinaryPathHeader>();

                // Check version compatibility
                if (header.FormatVersion != BinaryPathFormat.VERSION)
                {
                    MessageBox.Show($"Binary format version mismatch. Expected {BinaryPathFormat.VERSION}, got {header.FormatVersion}");
                    return;
                }

                // Save TreeView state
                var state = TreeViewStateHelper.SaveState(treeView);

                // Begin updating view model
                Dictionary<string, bool> processedPaths = new Dictionary<string, bool>(StringComparer.OrdinalIgnoreCase);

                // Process each path
                for (int i = 0; i < header.PathCount; i++)
                {
                    // Read path header
                    BinaryPathData pathData = BinaryUtils.BytesToStruct<BinaryPathData>(data, position);
                    position += Marshal.SizeOf<BinaryPathData>();

                    // Read path name
                    string pathName = BinaryUtils.ReadString(data, position, pathData.NameLength);
                    position += pathData.NameLength;

                    // Add path to processed list
                    processedPaths[pathName] = true;

                    // Find or create path view model
                    CameraPathViewModel pathViewModel = null;
                    foreach (var path in viewModel.CameraPaths)
                    {
                        if (string.Equals(path.PathName, pathName, StringComparison.OrdinalIgnoreCase))
                        {
                            pathViewModel = path;
                            pathViewModel.Nodes.Clear();
                            break;
                        }
                    }

                    if (pathViewModel == null)
                    {
                        pathViewModel = new CameraPathViewModel { PathName = pathName };
                        viewModel.CameraPaths.Add(pathViewModel);
                    }

                    // Read each node
                    for (int j = 0; j < pathData.NodeCount; j++)
                    {
                        // Read node data
                        BinaryNodeData nodeData = BinaryUtils.BytesToStruct<BinaryNodeData>(data, position);
                        position += Marshal.SizeOf<BinaryNodeData>();

                        // Create node view model
                        var nodeViewModel = new NodeViewModel
                        {
                            Index = nodeData.Index,
                            Position = nodeData.Position,
                            Rotation = nodeData.Rotation,
                            FOV = nodeData.FOV
                        };

                        // Populate details
                        nodeViewModel.PopulateDetails();

                        // Add to path
                        pathViewModel.Nodes.Add(nodeViewModel);
                    }
                }

                // Remove paths that weren't in the update
                for (int i = viewModel.CameraPaths.Count - 1; i >= 0; i--)
                {
                    var path = viewModel.CameraPaths[i];
                    if (!processedPaths.ContainsKey(path.PathName))
                    {
                        viewModel.CameraPaths.RemoveAt(i);
                    }
                }

                // Restore TreeView state
                TreeViewStateHelper.RestoreState(treeView, state);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error processing binary path data: " + ex.Message + "\n\nStack Trace: " + ex.StackTrace);
            }
        }
    }
}