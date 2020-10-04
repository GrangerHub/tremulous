#!/bin/sh

# ImageMagick is needed to run this script. It provide `convert`.


# Take care to escape wildcard with quote
emoticons=$(find ../ -maxdepth 1 -type f \( -iname "*x1.tga" ! -iname "*-ol*" \) -exec echo "\"{}\"" \;)

for emoticon in $emoticons
do
  filename=$(basename -- "$emoticon")
  name=${filename%.*}
	echo "Processing $name ..."
  for level in 0 1 2
  do
    output="${name}_${level}.tga"
    convert "$(echo $emoticon | tr -d '"')" -background black -shadow 100x$(echo "(1 + $level) * 2" | bc)+0+0 "$output"
    convert "$output" "$output" # ImageMagick seem to not compress image finely the first time
  done
done
