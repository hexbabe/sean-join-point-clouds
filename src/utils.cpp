#include "utils.h"

// Override stream operator for RawPCD better debug printing
std::ostream& operator<<(std::ostream& os, const RawPCD& pcd) {
    os << "RawPCD:" << std::endl;
    os << "  Fields: ";
    for (const auto& field : pcd.fields) {
        os << field << " ";
    }
    os << "\n  Sizes: ";
    for (const auto& size : pcd.sizes) {
        os << size << " ";
    }
    os << "\n  Types: ";
    for (const auto& type : pcd.types) {
        os << type << " ";
    }
    os << "\n  Counts: ";
    for (const auto& count : pcd.counts) {
        os << count << " ";
    }
    os << "\n  Viewpoint: ";
    for (const auto& num : pcd.viewpoint) {
        os << num << " ";
    }
    os << "\n  Width: " << pcd.width;
    os << "\n  Height: " << pcd.height;
    os << "\n  Points: " << pcd.points;
    os << "\n  Data Type: " << pcd.dataType;
    os << "\n  Raw Data Size: " << pcd.rawData.size() << " bytes";
    return os;
};

// Helper function to convert a single pcl::PointXYZ to binary
std::vector<unsigned char> pointToBinary(const pcl::PointXYZ& point) {
    std::vector<unsigned char> bytes;
    bytes.resize(sizeof(point.x) * 3); // Each point has three float coordinates
    std::memcpy(bytes.data(), &point.x, sizeof(point.x));
    std::memcpy(bytes.data() + sizeof(point.x), &point.y, sizeof(point.y));
    std::memcpy(bytes.data() + sizeof(point.x) * 2, &point.z, sizeof(point.z));
    return bytes;
}

// Converts PCL point cloud data to raw PCD payload in binary format
std::vector<unsigned char> pclCloudToPCDBytes(const pcl::PointCloud<pcl::PointXYZ>& pc) {
    std::stringstream header;
    header << "VERSION .7\n";
    header << "FIELDS x y z\n";
    header << "SIZE 4 4 4\n";
    header << "TYPE F F F\n";
    header << "COUNT 1 1 1\n";
    header << "WIDTH " << pc.points.size() << "\n";
    header << "HEIGHT 1\n";
    header << "VIEWPOINT 0 0 0 1 0 0 0\n";
    header << "POINTS " << pc.points.size() << "\n";
    header << "DATA binary\n";

    std::vector<unsigned char> pcd_bytes;
    std::string header_str = header.str();
    pcd_bytes.insert(pcd_bytes.end(), header_str.begin(), header_str.end());

    for (const auto& point : pc.points) {
        auto binary_point = pointToBinary(point);
        pcd_bytes.insert(pcd_bytes.end(), binary_point.begin(), binary_point.end());
    }

    return pcd_bytes;
}

// Helper that converts raw Viam PCD data into structured data
RawPCD parseRawPCD(const std::vector<unsigned char>& input) {
    std::string content(input.begin(), input.end());
    std::istringstream stream(content);
    std::string line;
    RawPCD pcd;
    std::size_t dataStartPosition = 0;

    while (std::getline(stream, line)) {
        dataStartPosition += line.size() + 1;  // +1 for the newline character
        std::istringstream lineStream(line);
        std::string tag;
        lineStream >> tag;

        if (tag == "FIELDS") {
            std::string field;
            while (lineStream >> field) {
                pcd.fields.push_back(field);
            }
        } else if (tag == "SIZE") {
            int size;
            while (lineStream >> size) {
                pcd.sizes.push_back(size);
            }
        } else if (tag == "TYPE") {
            char type;
            while (lineStream >> type) {
                pcd.types.push_back(type);
            }
        } else if (tag == "COUNT") {
            int count;
            while (lineStream >> count) {
                pcd.counts.push_back(count);
            }
        } else if (tag == "HEIGHT") {
            lineStream >> pcd.height;
        } else if (tag == "WIDTH") {
            lineStream >> pcd.width;
            std::cout << "Parsed WIDTH: " << pcd.width << std::endl;
        } else if (tag == "POINTS") {
            lineStream >> pcd.points;
            std::cout << "Parsed POINTS: " << pcd.points << std::endl;
        } else if (tag == "VIEWPOINT") {
            int viewpoint_data;
            while (lineStream >> viewpoint_data) {
                pcd.viewpoint.push_back(viewpoint_data);
            }
        } else if (tag == "DATA") {
            pcd.dataType = line.substr(5);
            break;
        }
    }

    // Assign the binary data starting after the header and the DATA line
    pcd.rawData.assign(input.begin() + dataStartPosition, input.end());

    return pcd;
}

// Converts RawPCD struct to pcl::PointCloud
pcl::PointCloud<pcl::PointXYZ>::Ptr convertToPointCloud(const RawPCD& rawPCD) {
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    int pointSize = sizeof(float) * 3;  // x, y, z each float
    cloud->width = rawPCD.width;
    cloud->height = rawPCD.height;
    cloud->points.resize(cloud->width * cloud->height);

    const float* floatData = reinterpret_cast<const float*>(rawPCD.rawData.data());
    for (size_t i = 0; i < cloud->points.size(); ++i) {
        pcl::PointXYZ& pt = cloud->points[i];
        pt.x = floatData[i * 3 + 0];
        pt.y = floatData[i * 3 + 1];
        pt.z = floatData[i * 3 + 2];
    }

    return cloud;
}
