using System;
using System.ComponentModel;
using System.Windows;
using IGCSClient.Controls;

namespace IGCSClient.Forms
{
    public partial class PathControllerWindow : Window
    {
        // Expose the PathController instance if needed.
        public PathController PathControllerInstance => pathControllerControl;
        internal bool _forceClose;
        // Optionally expose the PathController's properties:
        public IGCSClient.ViewModels.MainViewModel MainViewModel => pathControllerControl.ViewModel;
        public System.Windows.Controls.TreeView TreeViewControl => pathControllerControl.PathTreeViewControl;

        public PathControllerWindow()
        {
            InitializeComponent();
            // Ensure the controller is disabled initially
            this.pathControllerControl.IsEnabled = false;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // First fix the white border with Manual mode
            this.SizeToContent = SizeToContent.Manual;

            // Force layout calculation
            this.UpdateLayout();

            // Store current size
            double width = this.Width;
            double height = this.Height;

            // Queue up an action to switch back to auto-sizing
            Dispatcher.BeginInvoke(new Action(() => {
                // Set exact dimensions first (to preserve size)
                this.Width = width;
                this.Height = height;

                // Then restore auto-sizing for future changes
                this.SizeToContent = SizeToContent.WidthAndHeight;
            }), System.Windows.Threading.DispatcherPriority.Background);
        }

        // Handle the Always on Top toggle state changes
        private void AlwaysOnTopToggle_CheckedChanged(object sender, RoutedEventArgs e)
        {
            // Set the Topmost property based on the checkbox state
            this.Topmost = AlwaysOnTopToggle.IsChecked ?? false;
        }

        // Override OnClosing so that the window is hidden, not closed.
        protected override void OnClosing(CancelEventArgs e)
        {
            if (!_forceClose)
            {
                // Cancel normal closes => just hide.
                e.Cancel = true;
                Hide();
            }
            else
            {
                base.OnClosing(e);
            }
        }




    }
}
