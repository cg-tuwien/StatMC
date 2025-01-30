#!/usr/bin/env bash

# © 2024-2025 Hiroyuki Sakai

echo "This script will prompt you to reboot your system after installation."
echo

# Ensure the script is run as root
if [ "$(id -u)" -ne 0 ]; then
  echo "Please run with sudo."
  exit 1
fi

# Ensure correct Ubuntu version
if [ "$(lsb_release -r -s)" != "22.04" ]; then
  echo "This script has been tested for Ubuntu 22.04 LTS only."
  echo "Use other OSes at your own risk."
  read -p "Continue anyway? (y/n)" continue_choice
  if [[ "$continue_choice" =~ ^[Nn]$ ]]; then
    echo "Aborting."
    exit 0
  fi
fi

# Ensure up-to-date packages
echo "It may make sense to first run apt update and apt upgrade."
read -p "Continue anyway? (y/n): " update_choice
if [[ "$update_choice" =~ ^[Nn]$ ]]; then
  echo "Aborting."
  exit 0
fi

clang_version="16"
libstdcpp_version="12"
cuda_version="12-3"
nvidia_driver_version="545"
ubuntu_version="ubuntu2204"

# Check for existing CUDA installation
if command -v nvcc &> /dev/null; then
  echo "CUDA is already installed. Exiting."
  exit 1
fi

# Check for existing NVIDIA driver installation
if lsmod | grep -i nvidia &> /dev/null; then
  echo "NVIDIA drivers are already installed. Exiting."
  exit 1
fi

# Install necessary packages
## Clang repository setup
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
add-apt-repository -y 'deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-16 main'

apt-get update

# Install dev packages
apt install -y cmake
apt install -y clang-$clang_version
apt install -y libstdc++-$libstdcpp_version-dev
apt install -y zlib1g-dev
update-alternatives --install /usr/bin/clang clang /usr/bin/clang-$clang_version ${clang_version}0 --slave /usr/bin/clang++ clang++ /usr/bin/clang++-$clang_version

# CUDA installation preparation (see https://docs.nvidia.com/cuda/archive/12.3.2/cuda-installation-guide-linux/index.html#prepare-ubuntu)
apt-get install linux-headers-$(uname -r)
apt-key del 7fa2af80

# CUDA installation
wget https://developer.download.nvidia.com/compute/cuda/repos/$ubuntu_version/x86_64/cuda-keyring_1.1-1_all.deb
if ! dpkg -i cuda-keyring_1.1-1_all.deb; then
  echo "Failed to install CUDA keyring. Exiting."
  exit 1
fi

if ! apt-get update; then
  echo "Failed to update package list. Exiting."
  exit 1
fi

if ! apt-get -y install cuda-toolkit-$cuda_version; then
  echo "Failed to install CUDA toolkit. Exiting."
  exit 1
fi

# Install drivers
if ! apt-get install -y nvidia-kernel-open-$nvidia_driver_version cuda-drivers-$nvidia_driver_version; then
  echo "Failed to install NVIDIA drivers. Exiting."
  exit 1
fi

# Set environment variables permanently
echo "export PATH=/usr/local/cuda-${cuda_version/-/.}/bin${PATH:+:${PATH}}" >> /etc/profile.d/cuda.sh

# Source the updated profile to apply changes immediately
source /etc/profile.d/cuda.sh

# Install additional dependencies
if ! apt-get install -y g++ freeglut3-dev build-essential libx11-dev libxmu-dev libxi-dev libglu1-mesa-dev libfreeimage-dev libglfw3-dev; then
  echo "Failed to install additional dependencies. Exiting."
  exit 1
fi

# Clean up
rm cuda-keyring_1.1-1_all.deb
apt autoremove --purge && apt clean

# Install Persistence daemon (recommended approach; calling nvidia-persistenced (https://docs.nvidia.com/deploy/driver-persistence/index.html) on startup didn't work for some reason)
echo "#!/bin/sh" >> /etc/init.d/nvidia-persistenced.sh
echo "sudo -i" >> /etc/init.d/nvidia-persistenced.sh
echo "nvidia-smi -pm 1" >> /etc/init.d/nvidia-persistenced.sh
echo "exit" >> /etc/init.d/nvidia-persistenced.sh

chmod +x /etc/init.d/nvidia-persistenced.sh
update-rc.d nvidia-persistenced.sh defaults

# https://askubuntu.com/a/401090
echo "#!/bin/sh -e" >> /etc/rc.local
echo "sh '/etc/init.d/nvidia-persistenced.sh'" >> /etc/rc.local
echo "exit 0" >> /etc/rc.local

chown root /etc/rc.local
chmod 755 /etc/rc.local

echo
echo "Installation complete."

# Prompt for reboot
echo "A system reboot is required to complete the installation and load the new kernel modules."
read -p "Reboot the system now? (y/n): " reboot_choice
if [[ "$reboot_choice" =~ ^[Yy]$ ]]; then
  echo "Rebooting system..."
  reboot
else
  echo "Please reboot the system manually to complete the installation."
fi
