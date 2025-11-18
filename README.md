# clip
Clip TIF file bands from a single Landsat scene

# Options:
  -i, --idir DIR         Input directory to scan *.tif files
  -o, --odir DIR         Output directory to write *.tif files
  -c, --source_crs STR   Source coordinate reference system (e.g. "EPSG:32615")
  -m, --mask FILE        Specify a mask file (*.shp)
  -d, --datasets LIST    List of datasets (comma separated)
  -p, --pattern STR      Pattern to filter files to process
  -n, --label STR        Label for output files
  -v, --version          Show version information
  -h, --help             Show this help message

# Preprocessing

1) This program scans a directory for extracted Landsat scenes in GeoTIFF files (not COG).
2) Reads each file and checks for the projection, UTM is expected.
3) Uses a shapefile polygon, with a single feature, to extract its extent. Inflates by 31 m.
4) Uses the extent to extract exactly the same region on the scene of each date.
5) Saves a clipped *.tif file for each dataset.

# License

Copyright (C) 2025 Eduardo Jimenez Hernandez <eduardojh@arizona.edu>.
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.