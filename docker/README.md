# Docker Build Instructions


## TL;DR

Here, we assume that Docker is already installed and can access the GPU.
If you did not install Docker yet, please refer to the [complete instructions below](#setup).

```
git clone --recursive https://github.com/cg-tuwien/StatMC.git
cd StatMC/
sudo docker build -t statmc -f ./docker/Dockerfile . --network=host
sudo docker run -d -i -t --name statmc -v .:/StatMC --gpus all --network=host statmc
sudo docker exec -it statmc bash
./docker/install-dependencies-container.sh
./scripts/_build.sh
./scripts/_download-scenes.sh
./scripts/1-staircase.sh
```

## Setup 

1. If not already done, check out our repository:
    ```bash
    git clone --recursive https://github.com/cg-tuwien/StatMC.git
    cd StatMC/
    ```

    **Important**:
    Make sure all shell scripts inside the `scripts/` directory have UNIX line endings (`\n`).
    On Windows, Git might convert them automatically to Windows-style line endings (`\r\n`) which will result in errors when executed with bash inside the container.

2. Optional: if not already installed, install the NVIDIA driver and CUDA on the host:
    ```bash
    sudo ./scripts/_install-dependencies.sh
    ```
    If you reboot, make sure to go back into the `StatMC/` directory afterwards.

3. Optional: if not already installed, install Docker and the NVIDIA Container Toolkit on the host:
    ```bash
    ./docker/install_docker.sh
    ./docker/install_nvidia_container_toolkit.sh 
    ``` 

4. Create the container:
    ```bash
    sudo docker build -t statmc -f ./docker/Dockerfile . --network=host
    ```    

5. Start the container in detached mode:
    ```bash
    sudo docker run -d -i -t --name statmc -v .:/StatMC --gpus all --network=host statmc
    ```
    This enables the GPU for the container and mounts the current (`StatMC`) directory (as a persistent volume) at `/StatMC` inside the container.

6. If not already done, make the shell scripts for Docker executable:
    ```bash
    chmod +x ./docker/install-dependencies-container.sh
    ```

7. Open an interactive shell inside the container:
    ```bash
    sudo docker exec -it statmc bash
    ```

    Inside the interactive shell:

    1. Install dependencies inside the container, compile the sources, and download scenes:
        ```bash
        ./docker/install-dependencies-container.sh
        ./scripts/_build.sh
        ./scripts/_download-scenes.sh
        ```

    2. Compute the first result from the paper.
        ```bash 
        ./scripts/1-staircase.sh
        ```
