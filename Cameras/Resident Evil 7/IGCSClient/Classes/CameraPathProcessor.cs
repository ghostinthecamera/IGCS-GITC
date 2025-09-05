//using System;
//using System.Collections.Generic;
//using System.Linq;
//using System.Runtime.InteropServices;
//using System.Text;
//using System.Threading.Tasks;
//using System.Windows;
//using System.Windows.Controls;
//using IGCSClient.Classes;
//using IGCSClient.Forms;
//using IGCSClient.Models;
//using IGCSClient.ViewModels;
//using Newtonsoft.Json;

//namespace IGCSClient.Helpers
//{


//    public static class CameraPathProcessor
//    {
//        public static void handleCameraPathDataMessage(byte[] value)
//        {
//            try
//            {
//                var utf8Encoding = new UTF8Encoding();
//                // Decode the payload (everything after the first byte) as a JSON string.
//                string jsonPayload = utf8Encoding.GetString(value, 1, value.Length - 1);

//                // Retrieve the PathController instance using FindName.
//                Application.Current.Dispatcher.Invoke(() =>
//                {
//                    // Retrieve the PathControllerWindow from the static property.
//                    var pathControllerWindow = MainWindow.CurrentPathControllerWindow;
//                    if (pathControllerWindow != null)
//                    {
//                        MainViewModel mainViewModel = pathControllerWindow.PathControllerInstance.ViewModel;
//                        ProcessCameraPathJson(jsonPayload, mainViewModel, pathControllerWindow.TreeViewControl);
//                    }
//                    else
//                    {
//                        LogHandlerSingleton.Instance().LogLine("PathControllerWindow is not open.", "MessageHandler", false, true);
//                    }
//                });
//            }
//            catch (Exception ex)
//            {
//                LogHandlerSingleton.Instance().LogLine("Exception processing CameraPathData: " + ex.Message, "MessageHandler", false, true);
//            }
//        }

//        // Processes the JSON payload and updates the main view model.
//        public static void ProcessCameraPathJson(string jsonPayload, MainViewModel mainViewModel, TreeView treeView)
//        {
//            try
//            {
//                // Custom settings to handle non-standard JSON format if needed
//                var jsonSettings = new JsonSerializerSettings
//                {
//                    NullValueHandling = NullValueHandling.Ignore,
//                    MissingMemberHandling = MissingMemberHandling.Ignore
//                };

//                // Deserialize the JSON payload into our data model.
//                var allPathsData = JsonConvert.DeserializeObject<AllCameraPathsData>(jsonPayload, jsonSettings);

//                // Save the current expansion state of the TreeView.
//                var state = TreeViewStateHelper.SaveState(treeView);

//                // If the payload is empty or "paths" is null, clear the viewmodel's collection.
//                if (allPathsData == null || allPathsData.paths == null)
//                {
//                    Application.Current.Dispatcher.Invoke(() =>
//                    {
//                        mainViewModel.CameraPaths.Clear();
//                    });
//                    return;
//                }

//                // Update the UI on the UI thread.
//                Application.Current.Dispatcher.Invoke(() =>
//                {
//                    // Build a dictionary of JSON paths for quick lookup (case-insensitive).
//                    var jsonPathsDict = allPathsData.paths.ToDictionary(
//                        p => p.pathName,
//                        p => p,
//                        StringComparer.OrdinalIgnoreCase);

//                    // Remove any CameraPathViewModels that are not present in the JSON.
//                    for (int i = mainViewModel.CameraPaths.Count - 1; i >= 0; i--)
//                    {
//                        var vmPath = mainViewModel.CameraPaths[i];
//                        if (!jsonPathsDict.ContainsKey(vmPath.PathName))
//                        {
//                            mainViewModel.CameraPaths.RemoveAt(i);
//                        }
//                    }

//                    // Now update existing paths and add new ones.
//                    foreach (var jsonPath in allPathsData.paths)
//                    {
//                        // Try to find an existing path with the same name.
//                        var existingPath = mainViewModel.CameraPaths
//                            .FirstOrDefault(p => string.Equals(p.PathName, jsonPath.pathName, StringComparison.OrdinalIgnoreCase));

//                        if (existingPath == null)
//                        {
//                            // Create a new CameraPathViewModel.
//                            existingPath = new CameraPathViewModel { PathName = jsonPath.pathName };
//                            mainViewModel.CameraPaths.Add(existingPath);
//                        }
//                        else
//                        {
//                            // Clear nodes for an update.
//                            existingPath.Nodes.Clear();
//                        }

//                        // For each node in the JSON, create a new NodeViewModel.
//                        foreach (var node in jsonPath.nodes)
//                        {
//                            var nodeViewModel = new NodeViewModel
//                            {
//                                Index = node.index,
//                                Position = node.position,
//                                Rotation = node.rotation,
//                                FOV = node.fov
//                            };

//                            // Populate the details (for level 3) from node data.
//                            nodeViewModel.PopulateDetails();

//                            existingPath.Nodes.Add(nodeViewModel);
//                        }
//                    }
//                    TreeViewStateHelper.RestoreState(treeView, state);
//                });

//                //// Restore the TreeView state.
//                //Application.Current.Dispatcher.Invoke(() =>
//                //{
//                //    TreeViewStateHelper.RestoreState(treeView, state);
//                //});
//            }
//            catch (Exception ex)
//            {
//                MessageBox.Show("Error processing CameraPath JSON: " + ex.Message);
//            }
//        }
//    }

//    //public static class CameraPathProcessor
//    //{

//    //    public static void handleCameraPathDataMessage(byte[] value)
//    //    {
//    //        try
//    //        {
//    //            var utf8Encoding = new UTF8Encoding();
//    //            // Decode the payload (everything after the first byte) as a JSON string.
//    //            string jsonPayload = utf8Encoding.GetString(value, 1, value.Length - 1);
//    //            // Retrieve the PathController instance using FindName.
//    //            // This assumes that the MainWindow is already loaded and _pathController exists in its namescope.
//    //            Application.Current.Dispatcher.Invoke(() =>
//    //            {
//    //                // Retrieve the PathControllerWindow from the static property.
//    //                var pathControllerWindow = MainWindow.CurrentPathControllerWindow;
//    //                if (pathControllerWindow != null)
//    //                {
//    //                    MainViewModel mainViewModel = pathControllerWindow.PathControllerInstance.ViewModel;
//    //                    ProcessCameraPathJson(jsonPayload, mainViewModel, pathControllerWindow.TreeViewControl);
//    //                }
//    //                else
//    //                {
//    //                    LogHandlerSingleton.Instance().LogLine("PathControllerWindow is not open.", "MessageHandler", false, true);
//    //                }
//    //            });
//    //        }
//    //        catch (Exception ex)
//    //        {
//    //            LogHandlerSingleton.Instance().LogLine("Exception processing CameraPathData: " + ex.Message, "MessageHandler", false, true);
//    //        }
//    //    }


//    //    // Processes the JSON payload and updates the main view model.
//    //    public static void ProcessCameraPathJson(string jsonPayload, MainViewModel mainViewModel, TreeView treeView)
//    //    {
//    //        try
//    //        {
//    //            // Deserialize the JSON payload into our data model.
//    //            var allPathsData = JsonConvert.DeserializeObject<AllCameraPathsData>(jsonPayload);

//    //            // Save the current expansion state of the TreeView.
//    //            var state = TreeViewStateHelper.SaveState(treeView);

//    //            // If the payload is empty or "paths" is null, clear the viewmodel's collection.
//    //            if (allPathsData == null || allPathsData.paths == null)
//    //            {
//    //                Application.Current.Dispatcher.Invoke(() =>
//    //                {
//    //                    mainViewModel.CameraPaths.Clear();
//    //                });
//    //                return;
//    //            }

//    //            // Update the UI on the UI thread.
//    //            Application.Current.Dispatcher.Invoke(() =>
//    //            {
//    //                // Build a dictionary of JSON paths for quick lookup (case-insensitive).
//    //                var jsonPathsDict = allPathsData.paths.ToDictionary(
//    //                    p => p.pathName,
//    //                    p => p,
//    //                    StringComparer.OrdinalIgnoreCase);

//    //                // Remove any CameraPathViewModels that are not present in the JSON.
//    //                for (int i = mainViewModel.CameraPaths.Count - 1; i >= 0; i--)
//    //                {
//    //                    var vmPath = mainViewModel.CameraPaths[i];
//    //                    if (!jsonPathsDict.ContainsKey(vmPath.PathName))
//    //                    {
//    //                        mainViewModel.CameraPaths.RemoveAt(i);
//    //                    }
//    //                }

//    //                // Now update existing paths and add new ones.
//    //                foreach (var jsonPath in allPathsData.paths)
//    //                {
//    //                    // Try to find an existing path with the same name.
//    //                    var existingPath = mainViewModel.CameraPaths
//    //                        .FirstOrDefault(p => string.Equals(p.PathName, jsonPath.pathName, StringComparison.OrdinalIgnoreCase));

//    //                    if (existingPath == null)
//    //                    {
//    //                        // Create a new CameraPathViewModel.
//    //                        existingPath = new CameraPathViewModel { PathName = jsonPath.pathName };
//    //                        mainViewModel.CameraPaths.Add(existingPath);
//    //                    }
//    //                    else
//    //                    {
//    //                        // Clear nodes for an update.
//    //                        existingPath.Nodes.Clear();
//    //                    }

//    //                    // For each node in the JSON, create a new NodeViewModel.
//    //                    foreach (var node in jsonPath.nodes)
//    //                    {
//    //                        var nodeViewModel = new NodeViewModel
//    //                        {
//    //                            Index = node.index,
//    //                            Position = node.position,
//    //                            Rotation = node.rotation,
//    //                            FOV = node.fov
//    //                        };

//    //                        // Populate the details (for level 3) from node data.
//    //                        nodeViewModel.PopulateDetails();

//    //                        existingPath.Nodes.Add(nodeViewModel);
//    //                    }
//    //                }
//    //            });
//    //            //// Restore the TreeView state.
//    //            //Application.Current.Dispatcher.Invoke(() =>
//    //            //{
//    //            //    TreeViewStateHelper.RestoreState(treeView, state);
//    //            //});
//    //        }
//    //        catch (Exception ex)
//    //        {
//    //            MessageBox.Show("Error processing CameraPath JSON: " + ex.Message);
//    //        }

//    //    }
//    //}

//}
