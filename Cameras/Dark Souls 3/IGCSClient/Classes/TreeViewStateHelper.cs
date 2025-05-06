using System.Collections.Generic;
using System.Windows.Controls;

namespace IGCSClient.Helpers
{
    public static class TreeViewStateHelper
    {
        public static Dictionary<object, bool> SaveState(TreeView treeView)
        {
            var state = new Dictionary<object, bool>();
            foreach (var item in treeView.Items)
            {
                if (treeView.ItemContainerGenerator.ContainerFromItem(item) is TreeViewItem tvi)
                {
                    SaveItemState(tvi, state);
                }
            }
            return state;
        }

        private static void SaveItemState(TreeViewItem item, Dictionary<object, bool> state)
        {
            if (item == null)
                return;

            state[item.DataContext] = item.IsExpanded;

            foreach (var child in item.Items)
            {
                if (item.ItemContainerGenerator.ContainerFromItem(child) is TreeViewItem childTvi)
                {
                    SaveItemState(childTvi, state);
                }
            }
        }

        public static void RestoreState(TreeView treeView, Dictionary<object, bool> state)
        {
            foreach (var item in treeView.Items)
            {
                if (treeView.ItemContainerGenerator.ContainerFromItem(item) is TreeViewItem tvi)
                {
                    RestoreItemState(tvi, state);
                }
            }
        }

        private static void RestoreItemState(TreeViewItem item, Dictionary<object, bool> state)
        {
            if (item == null)
                return;

            if (state.TryGetValue(item.DataContext, out bool isExpanded))
            {
                item.IsExpanded = isExpanded;
            }

            foreach (var child in item.Items)
            {
                if (item.ItemContainerGenerator.ContainerFromItem(child) is TreeViewItem childTvi)
                {
                    RestoreItemState(childTvi, state);
                }
            }
        }
    }
}
