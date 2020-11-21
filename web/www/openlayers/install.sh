#!/bin/sh -x

# Openlayers version (https://openlayers.org/)

version=6.4.3

# Layer switcher version (https://github.com/walkermatt/ol-layerswitcher/)

layer_switcher_version=3.7.0


# Download the file.

wget https://github.com/openlayers/openlayers/releases/download/v$version/v$version-dist.zip

# Uncompress it.

unzip v$version-dist.zip

# Move the files

mv v$version-dist/* .
rm -rf v$version-dist


# Download the file

wget https://github.com/walkermatt/ol-layerswitcher/archive/v$layer_switcher_version.zip

# Uncompress it.

unzip v$layer_switcher_version.zip


# Move the files

mv ol-layerswitcher-$layer_switcher_version/dist/ol-layerswitcher.js .
mv ol-layerswitcher-$layer_switcher_version/src/ol-layerswitcher.css .

rm -rf ol-layerswitcher-$layer_switcher_version
