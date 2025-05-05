using System.Linq;
using System.ComponentModel;
using System.Collections.ObjectModel;

namespace IGCSClient.ViewModels
{
    // ViewModel for an individual node.
    public class NodeViewModel : INotifyPropertyChanged
    {
        public int Index { get; set; }
        public float[] Position { get; set; }
        public float[] Rotation { get; set; }
        public float FOV { get; set; }

        // Collection of details that will be shown as a third level.
        public ObservableCollection<NodeDetail> Details { get; } = new ObservableCollection<NodeDetail>();

        // Populate the Details collection using the current property values.
        public void PopulateDetails()
        {
            Details.Clear();
            string posStr = (Position != null) ? string.Join(", ", Position.Select(p => p.ToString("F2"))) : "";
            string rotStr = (Rotation != null) ? string.Join(", ", Rotation.Select(r => r.ToString("F2"))) : "";
            Details.Add(new NodeDetail { Key = "Position", Value = posStr });
            Details.Add(new NodeDetail { Key = "Rotation", Value = rotStr });
            Details.Add(new NodeDetail { Key = "FOV", Value = FOV.ToString("F2") });
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName) =>
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        public override string ToString()
        {
            return $"Node {Index}";
        }

    }

    public class NodeDetail : INotifyPropertyChanged
    {
        private string _key;
        public string Key
        {
            get => _key;
            set { _key = value; OnPropertyChanged(nameof(Key)); }
        }

        private string _value;
        public string Value
        {
            get => _value;
            set { _value = value; OnPropertyChanged(nameof(Value)); }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }


    // ViewModel for a camera path.
    public class CameraPathViewModel : INotifyPropertyChanged
    {
        private string _pathName;
        public string PathName
        {
            get => _pathName;
            set { _pathName = value; OnPropertyChanged(nameof(PathName)); }
        }
        public override string ToString()
        {
            return PathName;
        }
        // Collection of node view models.
        public ObservableCollection<NodeViewModel> Nodes { get; } = new ObservableCollection<NodeViewModel>();

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName) =>
             PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    // The main view model for the window.
    public class MainViewModel
    {
        public ObservableCollection<CameraPathViewModel> CameraPaths { get; } =
            new ObservableCollection<CameraPathViewModel>();
    }
}
