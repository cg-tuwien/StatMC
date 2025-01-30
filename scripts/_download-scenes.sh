#!/usr/bin/env bash

# © 2024-2025 Hiroyuki Sakai

dpkg -s unzip &> /dev/null

if [ $? -ne 0 ]; then
  echo "This script requires unzip, which was not found on your system."
  read -p "Do you want to install it now? You may have to provide your password. (y/n)" unzip_choice
  if [[ "$unzip_choice" =~ ^[Yy]$ ]]; then
    sudo apt update
    sudo apt install unzip
  else
    echo "Aborting."
    exit 0
  fi
fi

cd scenes/

declare -A urls
urls["staircase"]="https://benedikt-bitterli.me/resources/pbrt-v3/staircase.zip"
urls["bathroom"]="https://benedikt-bitterli.me/resources/pbrt-v3/bathroom.zip"
urls["bathroom2"]="https://benedikt-bitterli.me/resources/pbrt-v3/bathroom2.zip"
urls["classroom"]="https://benedikt-bitterli.me/resources/pbrt-v3/classroom.zip"
urls["veach-ajar"]="https://benedikt-bitterli.me/resources/pbrt-v3/veach-ajar.zip"
urls["measure-one"]="https://owncloud.tuwien.ac.at/index.php/s/2rJf3TKUcnMBiAF/download"
urls["veach-mis"]="https://benedikt-bitterli.me/resources/pbrt-v3/veach-mis.zip"
urls["living-room"]="https://benedikt-bitterli.me/resources/pbrt-v3/living-room.zip"
urls["living-room-2"]="https://benedikt-bitterli.me/resources/pbrt-v3/living-room-2.zip"
urls["living-room-3"]="https://benedikt-bitterli.me/resources/pbrt-v3/living-room-3.zip"
urls["caustic"]="https://owncloud.tuwien.ac.at/index.php/s/wAAxtzdeBHZF2zR/download"
urls["furball"]="https://benedikt-bitterli.me/resources/pbrt-v3/furball.zip"

for url in "${!urls[@]}"
do
  wget -O "${url}.zip" "${urls[${url}]}"
  unzip -o "${url}.zip"
  find "${url}" -type d -exec chmod 775 {} +
  rm "${url}.zip"
done

cd ../
