//using Newtonsoft.Json;
//using System;
//using System.Collections.Generic;
//using System.Linq;
//using System.Text;
//using System.Threading.Tasks;

//namespace IGCSClient.Models
//{
//    //// Matches a node object in the JSON.
//    //public class NodeData
//    //{
//    //    public int index { get; set; }
//    //    public float[] position { get; set; }  // Expect 3 elements: [x, y, z]
//    //    public float[] rotation { get; set; }  // Expect 4 elements: [x, y, z, w]
//    //    public float fov { get; set; }
//    //}

//    //// Matches a single camera path object.
//    //public class CameraPathData
//    //{
//    //    public string pathName { get; set; }
//    //    public List<NodeData> nodes { get; set; }
//    //}

//    //// The top‑level JSON object, containing an array of camera paths.
//    //public class AllCameraPathsData
//    //{
//    //    public List<CameraPathData> paths { get; set; }
//    //}
//    // Matches a node object in the JSON.
//    public class NodeData
//    {
//        [JsonProperty("i")]
//        public int index { get; set; }

//        [JsonProperty("p")]
//        public float[] position { get; set; }  // Expect 3 elements: [x, y, z]

//        [JsonProperty("r")]
//        public float[] rotation { get; set; }  // Expect 4 elements: [x, y, z, w]

//        [JsonProperty("f")]
//        public float fov { get; set; }
//    }

//    // Matches a single camera path object.
//    public class CameraPathData
//    {
//        [JsonProperty("n")]
//        public string pathName { get; set; }

//        [JsonProperty("d")]
//        public List<NodeData> nodes { get; set; }
//    }

//    // The top-level JSON object, containing an array of camera paths.
//    public class AllCameraPathsData
//    {
//        [JsonProperty("p")]
//        public List<CameraPathData> paths { get; set; }
//    }
//}
