using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace IGCSClient.Converters
{
    public class FloatArrayToStringConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            float[] arr = value as float[];
            if (arr == null)
                return string.Empty;
            // Format each float (for example, to 2 decimal places)
            return string.Join(", ", arr.Select(f => f.ToString("F2")));
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
