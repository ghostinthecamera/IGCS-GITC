//#pragma once
//#include <string>
//#include "PathManager.h"
//#include <nlohmann/json.hpp>
//using json = nlohmann::json;
//using ordered_json = nlohmann::ordered_json;
//
//namespace IGCS
//{
//    //inline std::string serializeAllCameraPaths(const CameraPathManager& manager)
//    //{
//    //    
//    //    ordered_json j;
//    //    j["paths"] = json::array();
//
//    //    // Assume manager.paths is accessible (or you add a getter for it)
//    //    for (const auto& pair : manager.getPaths())
//    //    {
//    //        ordered_json pathJson;
//    //        const std::string& pathName = pair.first;
//    //        const CameraPath& path = pair.second;
//    //        pathJson["pathName"] = pathName;
//    //        pathJson["nodes"] = json::array();
//
//    //        size_t nodeCount = path.GetNodeCount();
//    //        for (size_t i = 0; i < nodeCount; i++)
//    //        {
//    //            XMVECTOR posVector = path.getNodePosition(i);
//    //            XMFLOAT3 pos;
//    //            XMStoreFloat3(&pos, posVector);
//
//    //            XMVECTOR rotVector = path.getNodeRotation(i);
//    //            XMFLOAT4 rot;
//    //            XMStoreFloat4(&rot, rotVector);
//
//    //            float fov = path.getNodeFOV(i);
//
//    //            ordered_json node;
//    //            node["index"] = i;
//    //            node["position"] = { pos.x, pos.y, pos.z };
//    //            node["rotation"] = { rot.x, rot.y, rot.z, rot.w };
//    //            node["fov"] = fov;
//
//    //            pathJson["nodes"].push_back(node);
//    //        }
//
//    //        j["paths"].push_back(pathJson);
//    //    }
//
//    //    return j.dump();
//    //}
//    inline std::string serializeAllCameraPaths(const CameraPathManager& manager)
//    {
//        ordered_json j;
//        j["p"] = json::array(); // "p" for paths
//
//        // Assume manager.paths is accessible (or you add a getter for it)
//        for (const auto& pair : manager.getPaths())
//        {
//            ordered_json pathJson;
//            const std::string& pathName = pair.first;
//            const CameraPath& path = pair.second;
//            pathJson["n"] = pathName; // "n" for name
//            pathJson["d"] = json::array(); // "d" for data/nodes
//
//            size_t nodeCount = path.GetNodeCount();
//            for (size_t i = 0; i < nodeCount; i++)
//            {
//                XMVECTOR posVector = path.getNodePosition(i);
//                XMFLOAT3 pos;
//                XMStoreFloat3(&pos, posVector);
//
//                XMVECTOR rotVector = path.getNodeRotation(i);
//                XMFLOAT4 rot;
//                XMStoreFloat4(&rot, rotVector);
//
//                float fov = path.getNodeFOV(i);
//
//                ordered_json node;
//                node["i"] = i;        // "i" for index
//                node["p"] = { pos.x, pos.y, pos.z }; // "p" for position
//                node["r"] = { rot.x, rot.y, rot.z, rot.w }; // "r" for rotation
//                node["f"] = fov;      // "f" for fov
//
//                pathJson["d"].push_back(node);
//            }
//
//            j["p"].push_back(pathJson);
//        }
//
//        // Return JSON with no indentation for minimal size
//        return j.dump(0);
//    }
//}