#!/bin/sh

./"$1"
which magick &>/dev/null \
	&& magick "$1".ppm "$1".png \
	|| convert "$1".ppm "$1".png
