/*****************************************************************************
 * Clip TIF file bands from a single Landsat scene                           *
 * Eduardo Jimenez Hernandez <eduardojh@arizona.edu> - November 2025         *
 * Changelog:                                                                *
 * - 2025-11-04: Initial code                                                *
 * - 2025-11-07: Clipping TIF files with a SHP file's extent                 *
 *****************************************************************************/

#include <getopt.h>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "utils.hpp"

int main(int argc, char* argv[]) {

    std::string input_dir;
    std::string output_dir;
    std::string source_crs;
    std::string mask_subset;
    std::string datasets;
    std::string pattern;
    std::string label;
    bool debug = false;

    std::vector<std::string> list_datasets;
    std::vector<u_int8_t> list_tiers;

    // IMPORTANT: when addding a new option, don't forget to update the
    // short option, followed by colon if the argument is required.
    const char* const short_opts = "hvi:o:c:m:d:p:n:g";

    const option long_opts[] = {
        {"help",       no_argument,       nullptr, 'h'},
        {"version",    no_argument,       nullptr, 'v'},
        {"idir",       required_argument, nullptr, 'i'},
        {"odir",       required_argument, nullptr, 'o'},
        {"source_crs", required_argument, nullptr, 'c'},
        {"mask",       required_argument, nullptr, 'm'},
        {"datasets",   required_argument, nullptr, 'd'},
        {"pattern",    required_argument, nullptr, 'p'},
        {"label",      required_argument, nullptr, 'n'},
        {"debug",      no_argument,       nullptr, 'g'},
        {nullptr,      0,                 nullptr,  0 }
    };

    while (true) {
        const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

        if (opt == -1)
            break;

        switch (opt) {
            case 'h':
                print_help();
                return EXIT_SUCCESS;
            case 'v':
                print_version(APP_VERSION, APP_DATETIME);
                return EXIT_SUCCESS;
            case 'i':
                input_dir = optarg;
                break;
            case 'o':
                output_dir = optarg;
                break;
            case 'c':
                source_crs = optarg;
                break;
            case 'm':
                mask_subset = optarg;
                break;
            case 'd':
                datasets = optarg;
                break;
            case 'p':
                pattern = optarg;
                break;
            case 'n':
                label = optarg;
                break;
            case 'g':
                debug = true;
                break;
            case '?': // Unrecognized option
            default:
                print_help();
                return EXIT_FAILURE;
        }
    }

    std::cout << "clip - Clip TIF file bands from a single Landsat scene.\n";

    // Check required arguments

    if (input_dir.empty()) {
        std::cerr << "ERROR: Input directory path is required.\n\n";
        print_help();
        return EXIT_FAILURE;
    }

    if (output_dir.empty()) {
        std::cerr << "ERROR: Output directory path is required.\n\n";
        print_help();
        return EXIT_FAILURE;
    }

    if (source_crs.empty()) {
        std::cerr << "ERROR: Source CRS is required.\n\n";
        print_help();
        return EXIT_FAILURE;
    }

    if (mask_subset.empty()) {
        std::cout << "ERROR: Mask subset is required.\n";
        print_help();
        return EXIT_FAILURE;
    }

    if (datasets.empty()) {
        std::cout << "ERROR: Datasets are required.\n";
        print_help();
        return EXIT_FAILURE;
    }

    if (label.empty()) {
        // Label to identify reprojected HDF files
        label = "";
    }

    if (pattern.empty()) {
        // Label to identify reprojected HDF files
        pattern.clear();
    }

    // Show initial parameters

    std::cout << "Input directory: " << input_dir << "\n";
    if (!directory_exists(input_dir)) {
        std::cerr << "ERROR: Input directory not found: " << input_dir << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Output directory: " << output_dir << "\n";
    if (!directory_exists(output_dir)) {
        std::cerr << "ERROR: Output directory not found: " << output_dir << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Source CRS: " << source_crs << "\n";
    std::cout << "Mask: " << mask_subset << "\n";
    std::cout << "Label: " << label << "\n";
    std::cout << "Pattern: " << pattern << "\n";
    std::cout << "Debug: " << std::boolalpha << debug << "\n";

    list_datasets = split_by_commas(datasets);
    if (list_datasets.empty()) {
        std::cerr << "No datasets provided! Exiting.\n";
        return EXIT_FAILURE;
    }
    std::cout << "Datasets: ";
    for (std::string ds : list_datasets) {
        std::cout << ds << " ";
    }
    std::cout << "\n";

    // Initialize GDAL/OGR
    GDALAllRegister();
    OGRRegisterAll();

    // Get extent from polygon
    double minX, minY, maxX, maxY;
    if (getShapefileExtent(mask_subset, minX, maxX, minY, maxY)) {
        // Inflate the extent
        minX -= 31;
        minY -= 31;
        maxX += 31;
        maxY += 31;
        std::cout << "Extent:\n";
        std::cout << "minX=" << std::fixed << std::setprecision(15) << minX << ", minY="
                  << minY << ", maxX=" << maxX << ", maxY=" << maxY << "\n";
    } else {
        std::cerr << "ERROR: Failed to read shapefile extent\n";
        return EXIT_FAILURE;
    }

    std::vector<std::string> list_files = listFilesInDirectory(input_dir);
    std::sort(list_files.begin(), list_files.end()); // Sort the list of files

    for (std::string band : list_datasets) {

        std::cout << "\n======Processing " << band << "======\n";

        for (std::string &fname : list_files) {

            // Filter by pattern, if provided but not found in file name, ignore this file & continue 
            if (!pattern.empty() && !find_pattern(fname, pattern)) continue;
            
            // Create a filter for the band
            std::string band_pattern =  "_" + band;
            if (!find_pattern(fname, band_pattern)) continue;

            std::cout << "File=" << fname << ", ";
            // std::cout << "File kept=" << fname << "\n";

            PathParts file_parts = splitPath(fname);
            if (debug) {
                std::cout << "Input filename: \n";
                std::cout << "  Directory: " << file_parts.directory << "\n";
                std::cout << "  Basename:  " << file_parts.basename << "\n";
                std::cout << "  Stem:      " << file_parts.stem << "\n";
                std::cout << "  Extension: " << file_parts.extension << "\n";
            }

            if (file_parts.extension != ".tif") {
                std::cout << "\".tif\" extension expected. Skipping.\n";
                continue;
            }

            std::string projection;
            int utm_zone = 0;

            // parse metadata file
            std::string metadataPath = locateMetadataFile(input_dir, fname);
            if (!metadataPath.empty() && extractProjectionInfo(metadataPath, projection, utm_zone)) {
                // Create CRS from the metadata file
                std::cout << "Metadata=" << metadataPath << ", Proj=" << projection << ", Zone=" << utm_zone << "\n";
                std::string temp_crs = getEPSGFromUTMZone(utm_zone);  // All Landsat are Northern hemisphere
                std::cout << "  Source CRS=" << source_crs << ", temp CRS=" << temp_crs << "\n";
                if (!temp_crs.empty() && (source_crs != temp_crs)) {
                    // If valid temp_crs and different than the source CRS, replace the source CRS
                    std::cout << "***Updating CRS " << source_crs << " with " << temp_crs << "\n";
                    source_crs = temp_crs;
                }
            } else {
                // Continue using default CRS
                std::cout << "WARNING: Metadata not found or extraction faileded! Using source CRS=" << source_crs << "\n";
            }

            int epsgCode = parseEPSG(source_crs);
            if (epsgCode == -1) {
                std::cerr << "ERROR: Failed to get EPSG code.\n";
                return EXIT_FAILURE;
            }

            // Create the input file name for clipping
            std::string inFile = input_dir + file_parts.stem + file_parts.extension;
            if (input_dir.back() != '/') {
                inFile = input_dir + "/" + file_parts.stem + file_parts.extension;
            }
            // Create the input filename
            std::string outFile = output_dir + file_parts.stem + label + file_parts.extension;
            if (input_dir.back() != '/') {
                outFile = output_dir + "/" + file_parts.stem + label + file_parts.extension;
            }

            std::cout << "inFile: " << inFile << "\n";
            std::cout << "outFile: " << outFile << "\n";
            std::cout << "epsgCode: " << epsgCode << "\n";

            bool ok = clipRasterByBBox(inFile, outFile, minX, minY, maxX, maxY, epsgCode);
            if (!ok) {
                std::cerr << "ERROR: Failed to clip: " << fname << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
  
    std::cout << "Ice never dies!" << std::endl;
    return EXIT_SUCCESS;
}