#!/bin/bash

SVG="../external/EIA_Resolution_Chart_1956.svg"


inkscape -b white -y 255 -w 1280 -h 1024 -e StartImage_1280x1024.png $SVG
inkscape -b white -y 255 -w 1920 -h 1200 -e StartImage_1920x1200.png $SVG
inkscape -b white -y 255 -w 1920 -h 1080 -e StartImage_1920x1080.png $SVG

