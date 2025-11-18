#!/bin/bash
# ============================================================
# Description: Clip TIF file bands from a single Landsat scene
# Script: run_clip.sh
# Purpose: Run the clip tool with configurable parameters.
# Author: Eduardo Jimenez Hernandez <eduardojh@arizona.edu>
# Date: 2025-11-17
# ============================================================

Program="./bin/clip"

# === CONFIGURE ===
SourceCRS="EPSG:32615"
Datasets="QA_PIXEL,NDVI,EVI"
# FileFilter="_021047_2017"
Label="_clipped"

CWD="/error/"
InputDirs="${CWD}LANDSAT/SCENES/021047/"
OutputDir="${CWD}LANDSAT/CLIPPED/"
Mask="${CWD}GIS/Vector/SembrandoVida_Parcela1.shp"

# === DISPLAY SETTINGS ===
echo "------------------------------------------------------------"
echo " Running clip with the following parameters:"
echo " Program:       $Program"
echo " Input dirs:    $InputDirs"
echo " Output dir:    $OutputDir"
echo " Source CRS:    $SourceCRS"
echo " Datasets:      $Datasets"
echo " Pattern:       $FileFilter"
echo " Label:         $Label"
echo " Mask:          $Mask"
echo "------------------------------------------------------------"
echo

# === EXECUTION ===
"$Program" \
  --idir "$InputDirs" \
  --odir "$OutputDir" \
  --source_crs "$SourceCRS" \
  --label "$Label" \
  --mask "$Mask" \
  --datasets "$Datasets"
  # --pattern "$FileFilter" \

# === CHECK EXIT STATUS ===
exit_code=$?
if [ $exit_code -eq 0 ]; then
  echo "clip completed SUCCESSFULLY!"
else
  echo "clip encountered an error (exit code $exit_code)."
  exit $exit_code
fi
