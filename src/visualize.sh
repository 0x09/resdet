#!/usr/bin/env bash
# visualize the resolution detected by resdet over the image's spectrogram

set -e

if ! command -v resdet > /dev/null; then
	echo "This script requires resdet to be installed." 1>&2
	exit 1
fi

if ! command -v spec > /dev/null; then
	echo "This script requires the spec tool from https://github.com/0x09/dspfun to be installed." 1>&2
	exit 1
fi

if ! command -v magick > /dev/null; then
	echo "This script requires the ImageMagick command-line tools to be installed." 1>&2
	exit 1
fi

usage() {
	echo "usage: visualize.sh [-l <line_length> -c <line_color> -t <line_thickness> -r <resdet_opts> -s <spec_opts>] image output" 1>&2
	exit 2
}

help() {
	printf "usage: visualize.sh [-l <line_length> -c <line_color> -t <line_thickness> -r <resdet_opts> -s <spec_opts>] image output\n\
  -l <line_length>  Highlight line length in pixels. Default: 50\n\
  -c <line_color>   Highlight line color. Default: green\n\
  -t <thickness>    Highlight line thickness. Default: 1\n\
  -r <resdet_opts>  Quoted option string to pass to resdet\n\
  -s <spec_opts>    Quoted option string to pass to spec\n"
	exit 0
}

line_length=50
line_color=green
line_thickness=1
while getopts ":l:c:t:r:s:h" opt; do
	case $opt in
		l) line_length=$OPTARG;;
		c) line_color=$OPTARG;;
		t) line_thickness=$OPTARG;;
		r) resdet_opts=$OPTARG;;
		s) spec_opts=$OPTARG;;
		h) help;;
		[\?:]) usage;;
	esac
done

shift $((OPTIND-1))

image=$1
output=$2

[ -z "$output" ] && usage

read full_width full_height < <(identify -ping -format '%w %h\n' "$image")
read detected_width detected_height < <(resdet $resdet_opts -v 1 -- "$image")

spec $spec_opts -- "$image" - | magick - -stroke "$line_color" -strokewidth "$line_thickness" -draw \
	"line $((full_width-line_length)),$detected_height $full_width,$detected_height \
	 line $detected_width,$((full_height-line_length)) $detected_width,$full_height" \
	"$output"