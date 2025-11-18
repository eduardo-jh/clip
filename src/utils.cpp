#include <iostream>
#include <dirent.h>
#include <fstream>

#include "utils.hpp"

#include "gdal_utils.h"
#include "cpl_conv.h"
#include "ogrsf_frmts.h"

void print_help() {
    std::cout << "Usage: clip [OPTIONS]\n"
              << "Clip TIF file bands from a single Landsat scene.\n"
              << "Options:\n"
              << "  -i, --idir DIR         Input directory to scan *.tif files\n"
              << "  -o, --odir DIR         Output directory to write *.tif files\n"
              << "  -c, --source_crs STR   Source coordinate reference system (e.g. \"EPSG:32615\")\n"
              << "  -m, --mask FILE        Specify a mask file (*.shp)\n"
              << "  -d, --datasets LIST    List of datasets (comma separated)\n"
              << "  -p, --pattern STR      Pattern to filter files to process\n"
              << "  -n, --label STR        Label for output files\n"
              << "  -v, --version          Show version information\n"
              << "  -h, --help             Show this help message\n"
              << std::endl;
}

void print_version(const std::string &version, const std::string &date) {
    std::cout << "Clip TIF file bands from a single Landsat scene) v"
              << version << " release " << date << "\n"
              << "\nCopyright (C) 2025 Eduardo Jimenez Hernandez <eduardojh@arizona.edu>.\n"
              << "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
              << "This is free software: you are free to change and redistribute it.\n"
              << "There is NO WARRANTY, to the extent permitted by law."
              << std::endl;
}

std::vector<std::string> split_by_commas(const std::string &input) {
    std::vector<std::string> result;
    std::string token;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == ',') {
            if (!token.empty()) {
                result.push_back(token);
                // std::cout << "word:" << token << "\n";
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        result.push_back(token);
    }

    // std::cout << "String split into " << result.size() << " elements." << std::endl;

    return result;
}

std::vector<int> string_to_int(const std::vector<std::string> &strings) {
    std::vector<int> numbers;
    numbers.reserve(strings.size());

    for (const auto &s : strings) {
        try {
            size_t pos;
            int value = std::stoi(s, &pos);
            if (pos != s.size()) {
                return {}; // invalid conversion
            }
            numbers.push_back(value);
        } catch (...) {
            return {}; // conversion failed
        }
    }
    
    return numbers;
}

PathParts splitPath(const std::string& path) {
    PathParts parts;

    // Find last path separator (works for Unix '/' and Windows '\\')
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == std::string::npos) {
        parts.basename = path;  // no directory
    } else {
        parts.directory = path.substr(0, slashPos);
        parts.basename = path.substr(slashPos + 1);
    }

    // Find extension in basename
    size_t dotPos = parts.basename.find_last_of('.');
    if (dotPos == std::string::npos) {
        parts.stem = parts.basename;  // no extension
    } else {
        parts.stem = parts.basename.substr(0, dotPos);
        parts.extension = parts.basename.substr(dotPos);
    }

    return parts;
}

bool directory_exists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false; // cannot access path
    }
    return (info.st_mode & S_IFDIR) != 0;
}

bool ends_with(const std::string &str, const std::string &suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

bool find_pattern(const std::string& filename, const std::string& pattern) {
    // Returns true if 'pattern' is found anywhere inside 'filename'.
    return filename.find(pattern) != std::string::npos;
}

// ---------- Utility: list files in directory (POSIX) -------------
std::vector<std::string> listFilesInDirectory(const std::string& dirPath) {
    std::vector<std::string> files;
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        perror(("opendir: " + dirPath).c_str());
        return files;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name(entry->d_name);
        if (name == "." || name == "..") continue;
        files.push_back(name);
    }
    closedir(dir);
    return files;
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good(); // Checks if the file is open and in a good state
}

// Try to find ancillary file next to tif: "_MTL.txt"
std::string locateMetadataFile(const std::string& dirPath, const std::string& tifFilename) {
    
    // Build base, example: LC08_L2SP_021047_20250923_20251001_02_T1_MTL.txt
    std::string base = tifFilename;
    base = base.substr(0, 40);

    std::string metadataFilename = dirPath + base + "_MTL.txt";
    if (dirPath.back() != '/') {
        metadataFilename = dirPath + "/" + base + "_MTL.txt";
    }
    // std::cout << metadataFilename << "\n";

    // Check if file exits
    if (!fileExists(metadataFilename)) {
        return "";
    }

    return metadataFilename;    
}

bool extractProjectionInfo(const std::string& filename, std::string& projection, int& utmZone) {
    
    // std::cout << "Metadata=" << filename << " ";

    // Open the file for reading
    std::ifstream file(filename);
    
    // Check if the file opened successfully
    if (!file.is_open()) {
        std::cerr << "ERROR: Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;

    // Read the file line by line
    while (std::getline(file, line)) {
        // Remove leading and trailing whitespaces from the line
        line = line.substr(line.find_first_not_of(" \t"), line.find_last_not_of(" \t") + 1);

        // Check for MAP_PROJECTION and extract value
        if (line.find("MAP_PROJECTION") != std::string::npos) {
            // Extract value after "=" and remove quotes
            size_t startPos = line.find("=") + 1;
            // size_t endPos = line.find("\"", startPos + 1);
            // projection = line.substr(startPos + 1, endPos - startPos - 1);
            projection = stripString(line.substr(startPos));
            // std::cout << " startPos=" << startPos ;
            // std::cout << " proj=" << projection << " ";
        }

        // Check for UTM_ZONE and extract value
        if (line.find("UTM_ZONE") != std::string::npos) {
            // Extract the integer value after "="
            size_t startPos = line.find("=") + 1;
            // std::cout << "utmZone str=" << line.substr(startPos) << " ";
            utmZone = std::stoi(line.substr(startPos));
            // std::cout << "utmZone=" << utmZone << "\n";
        }

        // If both projection and utmZone are found, break out of the loop
        if (!projection.empty() && utmZone != 0) {
            // std::cout << "Found, exiting.\n";
            break;
        }
    }

    // Close the file
    file.close();

    // Return true if both the projection and UTM zone were extracted
    return !projection.empty() && utmZone != 0;
}

std::string stripString(const std::string& input) {
    // Create a copy of the input string
    std::string result = input;

    // Remove leading spaces
    result.erase(0, result.find_first_not_of(" \t"));

    // Remove trailing spaces
    result.erase(result.find_last_not_of(" \t") + 1);

    // Remove leading double quotes
    if (!result.empty() && result.front() == '"') {
        result.erase(0, 1);
    }

    // Remove trailing double quotes
    if (!result.empty() && result.back() == '"') {
        result.erase(result.size() - 1);
    }

    return result;
}

// Function to create an EPSG string from a UTM zone and hemisphere (Northern or Southern)
std::string getEPSGFromUTMZone(int zone, bool isSouthernHemisphere) {

    // Check the UTM zone between 1-60
    if (zone < 1 || zone > 60) {
        return "";
    }
    // UTM zones for Northern Hemisphere start with EPSG:326, for Southern Hemisphere with EPSG:327
    int epsgCode = isSouthernHemisphere ? 32700 + zone : 32600 + zone;
    
    // Construct the EPSG string (e.g., "EPSG:32615")
    return "EPSG:" + std::to_string(epsgCode);
}

bool clipRasterByBBox(const std::string& inFile,
                      const std::string& outFile,
                      double minX, double minY,
                      double maxX, double maxY,
                      int epsgCode)
{
    // GDALAllRegister();

    GDALDataset *src = (GDALDataset*)GDALOpen(inFile.c_str(), GA_ReadOnly);
    if (!src)
        return false;

    // ---- Build projection WKT from EPSG ----
    OGRSpatialReference srs;
    srs.importFromEPSG(epsgCode);
    char *wkt = nullptr;
    srs.exportToWkt(&wkt);

    // ---- Build ONLY GDALTranslate option strings ----
    std::vector<std::string> optStrs = {
        "-projwin",
        std::to_string(minX),
        std::to_string(maxY),
        std::to_string(maxX),
        std::to_string(minY),
        "-a_srs",
        std::string(wkt)
    };

    // Convert to char* array
    std::vector<char*> opts;
    for (auto &s : optStrs)
        opts.push_back(&s[0]);
    opts.push_back(nullptr);

    GDALTranslateOptions *trOpts =
        GDALTranslateOptionsNew(opts.data(), nullptr);

    // ---- Correct call: filenames NOT in options ----
    GDALDataset *outDs = (GDALDataset*)GDALTranslate(
        outFile.c_str(),  // output file from argument
        src,              // input dataset
        trOpts,           // options
        nullptr
    );

    GDALTranslateOptionsFree(trOpts);
    CPLFree(wkt);
    GDALClose(src);

    return (outDs != nullptr);
}

int parseEPSG(const std::string& epsgStr)
{
    const std::string prefix = "EPSG:";
    if (epsgStr.compare(0, prefix.size(), prefix) != 0)
        return -1; // invalid prefix

    try {
        return std::stoi(epsgStr.substr(prefix.size()));
    } catch (...) {
        return -1; // conversion failed
    }
}

bool getShapefileExtent(const std::string& shpFile,
                        double &xmin, double &xmax,
                        double &ymin, double &ymax)
{
    // OGRRegisterAll();

    // Open dataset
    GDALDataset *ds = (GDALDataset*)OGROpen(shpFile.c_str(), FALSE, nullptr);
    if (!ds) {
        std::cerr << "ERROR: Cannot read shapefile: " << shpFile << "\n";
        return false;
    }

    // Expect a single layer
    OGRLayer *layer = ds->GetLayer(0);
    if (!layer) {
        std::cerr << "ERROR: Expected a single layer in shapefile\n";
        GDALClose(ds);
        return false;
    }

    layer->ResetReading();
    OGRFeature *feat = layer->GetNextFeature();
    if (!feat) {
        std::cerr << "ERROR: No features in shapefile\n";
        GDALClose(ds);
        return false;
    }

    OGRGeometry *geom = feat->GetGeometryRef();
    if (!geom) {
        std::cerr << "ERROR: No geometry in shapefile\n";
        OGRFeature::DestroyFeature(feat);
        GDALClose(ds);
        return false;
    }

    // Extract envelope (bounding box)
    OGREnvelope env;
    geom->getEnvelope(&env);

    xmin = env.MinX;
    xmax = env.MaxX;
    ymin = env.MinY;
    ymax = env.MaxY;

    std::cout << "xmin: " << xmin << ", ymin: " << ymin << ", xmax: " << xmax << ", ymax" << ymax << "\n";

    OGRFeature::DestroyFeature(feat);
    GDALClose(ds);

    return true;
}