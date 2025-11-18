#pragma once

#include <string>
#include <vector>
#include <map>

#include "gdal_priv.h"

struct PathParts {
    std::string directory;  // parent directory
    std::string basename;   // filename with extension
    std::string stem;       // filename without extension
    std::string extension;  // extension (with leading dot, if any)
};

void print_help();
void print_version(const std::string &version, const std::string &date);
std::vector<std::string> split_by_commas(const std::string &);
std::vector<int> string_to_int(const std::vector<std::string> &);
PathParts splitPath(const std::string& );
bool directory_exists(const std::string&);
bool ends_with(const std::string &, const std::string &);
bool find_pattern(const std::string&, const std::string&);
std::vector<std::string> listFilesInDirectory(const std::string& dirPath);
bool fileExists(const std::string& filename);
std::string locateMetadataFile(const std::string& dirPath, const std::string& tifFilename);
bool extractProjectionInfo(const std::string& filename, std::string& projection, int& utmZone);
std::string stripString(const std::string& input);
std::string getEPSGFromUTMZone(int zone, bool isSouthernHemisphere = false);
bool clipRasterByBBox(const std::string& inFile,
                      const std::string& outFile,
                      double minX, double minY,
                      double maxX, double maxY,
                      int epsgCode);
int parseEPSG(const std::string& epsgStr);
bool getShapefileExtent(const std::string& shpFile,
                        double &xmin, double &xmax,
                        double &ymin, double &ymax);