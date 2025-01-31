# A Statistical Approach to Monte Carlo Denoising

![A visual and quantitative comparison between a noisy input rendering of the Staircase scene and the corresponding denoised outputs from Moon CI, OptiX, OIDN, ProDen, and our method.](https://users.cg.tuwien.ac.at/~hiroyuki/StatMC/static/images/saconferencepapers24-34-fig1.jpg)

This repository contains our implementation of our research paper ["A Statistical Approach to Monte Carlo Denoising" [Sakai et al. 2024]](https://www.cg.tuwien.ac.at/StatMC).
Our implementation extends [pbrt-v3](https://github.com/mmp/pbrt-v3) to provide the following:

- a new unidirectional path-tracing integrator `StatPathIntegrator`, which tracks the required statistics for denoising and implements approximate-contribution Russian roulette (ACRR) and selective multiple importance sampling (SMIS) described in Section 7 of our paper,
- [OpenCV](https://opencv.org/) integration for buffer management and CUDA abstraction,
- support for our CUDA denoiser implemented on top of OpenCV (hosted on a [separate repository](https://github.com/cg-tuwien/StatMC-opencv_contrib.git)),
- albedo lookup tables for faster and more accurate albedo queries (compared to pbrt-v3's own `rho()` function), and
- support for the [tev image viewer](https://github.com/Tom94/tev).

The extensions are mostly implemented in [`src/statistics/`](src/statistics) and [`src/display/`](src/display).

With the focus on research, this code is not intended for production.
We appreciate your feedback, questions, and reports of any issues you encounter; feel free to [contact us](https://www.cg.tuwien.ac.at/staff/HiroyukiSakai)!


## Build Instructions

### Prerequisites

We developed our denoiser using [CUDA 12.3](https://developer.nvidia.com/cuda-12-3-0-download-archive) and [OpenCV 4.8.1](https://github.com/opencv/opencv/releases/tag/4.8.1).
Note that [later CUDA versions (>= 12.4) are incompatible with OpenCV 4.8.1](https://github.com/opencv/opencv_contrib/issues/3690).

For reproducing the results presented in our paper, we recommend using Clang 16.0.6 on Ubuntu 22.04 LTS or Linux Mint 20 (as used for the paper).
While we have successfully tested GCC 11.4.0, it produces slightly different results (mostly due to differences in random number generation).

In the following, we describe two alternative ways to build our code: an automatic approach tested for Ubuntu 22.04 LTS and a manual approach, which we recommend if you want to retrace the steps of the build process or use another operating system.

### Automatic Building (for Ubuntu 22.04 LTS)

1.  Clone this repository (OpenCV will be cloned automatically as a submodule):
    ```bash
    git clone --recursive https://github.com/cg-tuwien/StatMC.git
    cd StatMC/
    ```

2.  Install dependencies:
    ```bash
    sudo ./scripts/_install-dependencies.sh
    ```

3.  Build our code:
    ```bash
    ./scripts/_build.sh
    ```
    Our version of the pbrt binary should now be located in `build/pbrt-v3/`.

### Manual Building

Skip this if you have used the automatic approach [above](#automatic-building-for-ubuntu-2204-lts).

1.  Clone this repository (OpenCV will be cloned automatically as a submodule):
    ```bash
    git clone --recursive https://github.com/cg-tuwien/StatMC.git
    cd StatMC/
    ```

2.  Make sure that CUDA 12.3, as well as the packages `cmake`, `clang`, `libstdc++-12-dev`, and `zlib1g-dev`, are installed.
    The packages may vary depending on the operating system.

#### Building OpenCV

1.  In the root directory of the repository, create the directories for building OpenCV:
    ```bash
    mkdir build
    cd build/
    mkdir opencv
    cd opencv/
    ```

2.  Build OpenCV according to [these instructions](https://github.com/cg-tuwien/StatMC-opencv_contrib#building-opencv) using the directories `../../src/ext/opencv` and `../../src/ext/opencv_contrib` for `<opencv_source_directory>` and `<opencv_contrib>`.
    We build OpenCV and pbrt separately to have better control over the individual builds.

3.  Change to the `build/` directory for building pbrt in the next step:
    ```bash
    cd ../
    ```

#### Building pbrt-v3

1.  In the [previously created](#building-opencv) `build/` directory, create the build directory for pbrt-v3:
    ```bash
    mkdir pbrt-v3
    cd pbrt-v3/
    ```

2.  Create the CMake buildsystem:
    ```bash
    cmake \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} -march=native" \
    -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -march=native" \
    ../../
    ```

3.  Build pbrt-v3
    ```bash
    make -j 16
    ```


## Usage

The directory [`scenes/`](scenes) contains configurations and scene description files for reproducing the results in our paper.
We do not include the complete scenes; they can be downloaded by running the [`scripts/download-scenes.sh`](scripts/_download-scenes.sh) shell script from the root directory of the repository:
```bash
./scripts/_download-scenes.sh
```
Note that the scenes are hosted on creator's or licensee's websites and are subject to being changed or taken down without prior notice: we are not responsible for (the availability of) the content hosted on other websites (except for the Glass Caustics scene created by our coauthor [Christian Freude](https://www.cg.tuwien.ac.at/staff/ChristianFreude) for our paper and [pbrt-v3's measure-one scene](https://www.pbrt.org/scenes-v3), which we rehost for your convenience).

Once the scenes are downloaded, you can reproduce the results from our paper by simply running the shell scripts for the corresponding figures in the [`scripts/`](scripts) directory. E.g.,
```bash
./scripts/1-staircase.sh
```
Result images will be saved to the `out/` directory.
The prefixes (here, `1`) refer to the figure number in the [paper](https://www.cg.tuwien.ac.at/research/publications/2024/sakai-2024-asa/sakai-2024-asa-paper.pdf) and [supplementary document](https://dl.acm.org/doi/suppl/10.1145/3680528.3687591/suppl_file/StatMC-Supplementary.pdf).
The images generated by these scripts should match our paper results, which we have published in lossless format [here](https://doi.org/10.48436/p3ert-wkm13) (including references).
Image filenames indicate the number of samples per pixel (SPP) used for rendering.
To reproduce a figure, pbrt must be run up to the corresponding SPP value.

Feel free to experiment with different scenes and configurations.
For a starting point, refer to the [quick reference](#quick-reference) further below.


## Notes on Reproducing Our Results

We reproduced the results in our paper by following the instructions on this page using two machines:

1. a desktop PC equipped with an AMD Ryzen 9 5950X CPU, an NVIDIA RTX 3080 Ti GPU, and running Linux Mint 20, as well as
2. a virtual machine hosted on a computing cluster equipped with an AMD EPYC 7413 CPU, an NVIDIA A40 GPU, and running Ubuntu 22.04 LTS.

While our instructions maximize reproducibility, minor variations in hardware, operating systems, compilers, and other factors may still result in differences in the generated images.

### Comparisons

In this repository, we do not include implementations of the neural denoisers we compared against in our paper.
For those comparisons, we have used the commits linked here:

- [NVIDIA's OptiX](https://github.com/DeclanRussell/NvidiaAIDenoiser/tree/3c0c3abc79de8a0a0044897a582036b928968fe4)
- [Intel's OIDN](https://github.com/RenderKit/oidn/tree/713ec7838ba650f99e0a896549c0dca5eeb3652d)
- [ProDen [Firmino et al. 2022]](https://github.com/ArthurFirmino/progressive-denoising/tree/3500970acb321c07278057ff6413b57823f365a8)

However, we include our own implementation of [Moon et al.'s confidence-interval approach [2013]](https://doi.org/10.1111/cgf.12004) in this repository; to activate it, OpenCV must be compiled with this compiler flag, which can be passed via `CMAKE_CXX_FLAGS` through CMake: `-DMEMFNC=1`.
Note that using this flag globally switches to their approach, thereby completely disabling our denoiser.
To reproduce the results reported in our paper for their approach, it is necessary to change the significance level to 0.002 (from 0.005); this is done by assigning `&t_002_quantiles[0]` to `*t_quantiles` (and uncommenting the corresponding line right above) [here](https://github.com/cg-tuwien/StatMC-opencv_contrib/blob/master/modules/cudaimgproc/src/cuda/stat_denoiser.cu#L67).
Furthermore, Box-Cox transformation must be disabled by setting [this flag](src/statistics/statpath.cpp#L1043) to `false`.

### Additional Steps for Reproducing Specific Figures

Specific figures require additional steps to reproduce:

- Figure 7 requires recompiling pbrt after changing the maximum number of samples [here](src/statistics/statpath.cpp#L283) and changing whether Russian roulette starts at the first or fifth bounce [here](src/statistics/statpath.cpp#L932).
- Figure 8 and S4 require recompiling pbrt after changing the maximum number of samples [here](src/statistics/statpath.cpp#L283).
- Figure S10 requires recompiling OpenCV after changing the significance level to 0.05 (from 0.005); this is done by assigning `&t_05_quantiles[0]` to `*t_quantiles` (and uncommenting the corresponding line right above) [here](https://github.com/cg-tuwien/StatMC-opencv_contrib/blob/master/modules/cudaimgproc/src/cuda/stat_denoiser.cu#L67).


## Quick Reference

### Additional Command-Line Options

Our version of the pbrt executable extends the original with the following options:

| Option | Description |
| - | - |
| `--writeimages` | Write images to disk. |
| `--displayserver <socket>` | Write images to the specified network socket (format `<IP address>:<port number>`). |
| `--baseseed <num>` | Use the specified base seed for `RandomSampler`. |
| `--denoise` | Skip rendering and use prerendered images on disk instead (useful for performing multiple denoising passes without rerendering). |
| `--warmup` | Perform a warm-up iteration (useful for consistent performance measurements). |

### Extended Scene Description Format

Most of the configuration is done in the scene description files.
In the following, we provide an overview over our extensions to the original scene description format.

#### StatPathIntegrator Options

We have extended the original format with options for our `StatPathIntegrator`.
To illustrate, here is an example configuration, which utilizes all relevant options:

```
Integrator "statpath" 
  "integer  maxdepth"           [65]
  "bool     expiterations"      ["true"]
  "integer  iterations"         [13]
  "integer  trackedbounces"     [0]
  "bool     multichannelstats"  ["true"]

  "bool     denoiseimage"     ["true"]
  "bool     acrr"             ["false"]
  "bool     smis"             ["false"]
  "bool     calcstats"        ["false"]
  "bool     calcprodenstats"  ["false"]
  "bool     calcmoonstats"    ["false"]
  "bool     calcgbuffers"     ["false"]
  "bool     calcitstats"      ["false"]

  "float    filtersd"         [10]
  "integer  filterradius"     [20]
  "string   filterbuffers"    ["albedo" "normal"]
  "float    filterbuffersds"  [0.02 0.1]

  "string   outputregex"  ["film|film-f"]
```

The following table summarizes all available options for our `StatPathIntegrator`:

| Type | Name | Default Value | Description |
| - | - | - | - |
| integer | `maxdepth` | `5` | Same as in the [original](https://pbrt.org/fileformat-v3#integrators): "Maximum length of a light-carrying path sampled by the integrator." |
| integer[4] | `pixelbounds` | (Entire image) | Same as in the [original](https://pbrt.org/fileformat-v3#integrators): "Subset of image to sample during rendering; in order, values given specify the starting and ending x coordinates and then starting and ending y coordinates. (This functionality is primarily useful for narrowing down to a few pixels for debugging.)" |
| float | `rrthreshold` | `1` | Same as in the [original](https://pbrt.org/fileformat-v3#integrators): "Determines when Russian roulette is applied to paths: when the maximum spectral component of the path contribution falls beneath this value, Russian roulette starts to be used." |
| string | `lightsamplestrategy` | `"spatial"` | Same as in the [original](https://pbrt.org/fileformat-v3#integrators): "Technique used for sampling light sources. Options include 'uniform', which samples all light sources uniformly, 'power', which samples light sources according to their emitted power, and 'spatial', which computes light contributions in regions of the scene and samples from a related distribution." |
| bool | `expiterations` | `true` | Our integrator operates iteratively, with each iteration comprising a rendering and denoising pass. `true` enables exponential growth of the total number of samples per pixel for rendering (e.g., 4, 16, 64, etc.), while `false` enables linear growth (e.g., 4, 8, 12, etc.). The (initial) number of samples per pixel (4 in the examples) is specified via the `pixelsamples` option of the `Sampler`. |
| integer | `iterations` | `16` | Total number of iterations |
| integer | `trackedbounces` | `maxdepth` | Number of bounces for which to track statistics (only relevant for ACRR and SMIS) |
| bool | `multichannelstats` | `true` | `true` enables statistics for the individual RGB channels, while `false` enables statistics for single-channel luminance only. The former provides more accurate results, since it allows to better differentiate between indivual colors for denoising. |
| bool | `denoiseimage` | `false` | `true` enables denoising of the rendered image. |
| bool | `acrr` | `false` | `true` enables approximate-contribution Russian roulette (ACRR). |
| bool | `smis` | `false` | `true` enables selective multiple importance sampling (SMIS). |
| bool | `calcstats` | `false` | `true` enables the calculation of G-buffers and statistics required by our denoiser. Use this option to precompute everything required for denoising without performing the denoising itself. |
| bool | `calcprodenstats` | `false` | `true` enables the calculation of G-buffers and statistics required by [ProDen [Firmino et al. 2022]](https://doi.org/10.1111/cgf.14454). |
| bool | `calcmoonstats` | `false` | `true` enables the calculation of G-buffers and statistics required by [Moon et al.'s confidence-interval approach [2013]](https://doi.org/10.1111/cgf.12004). |
| bool | `calcgbuffers` | `false` | `true` enables the calculation of G-buffers required by [NVIDIA's OptiX denoiser](https://developer.nvidia.com/optix-denoiser) and [Intel's OIDN](https://www.openimagedenoise.org/). |
| bool | `calcitstats` | `false` | `true` enables the calculation of statistics **per iteration**, required for evaluating the sampling efficiency of ACRR and SMIS. |
| float | `filtersd` | `10.0` | Standard deviation of the denoising filter kernel |
| integer | `filterradius` | `20` | Radius of the denoising filter kernel (limiting the kernel to a finite number of pixels) |
| string[] | `filterbuffers` | `["albedo" "normal"]` | G-buffers for denoising; possible options are `materialid`, `depth`, `normal`, `albedo`. `materialid` refers to unique numbers that are assigned to different materials by the renderer. For fair comparisons, we used albedos and normals only. |
| float[] | `filterbuffersds` | `[0.02 0.1]` | Standard deviations associated with the G-buffers ($\sigma_r$ as described in [one of the original joint-bilateral-filter papers](https://hhoppe.com/flash.pdf)); lower values make the filter more discriminative. |
| string | `outputregex` | `film.*` | Regular expression specifying the buffers to output (to disk or network socket as determined by the `--writeimages` and `--displayserver` [command-line options](#additional-command-line-options)); buffers whose unique names match the specified regular expression are output. This way of specification provides a high degree of flexibility, e.g., `film.*\|t0-.*` matches all buffers whose name begins with `film` or `t0-`. We provide a complete list of buffers [below](#buffer-system). |

#### Including Files

Similarly to [pbrt-v4](https://pbrt.org/fileformat-v4#include-import), our scene description format supports file includes:
```
Include "../_active.pbrt"
```
We have implemented this feature to quickly switch between rendering and denoising configurations without changing the scene description file itself.
We provide the following configurations in the [`scenes/`](scenes) directory:

| Configuration File | Description |
| - | - |
| [`render.pbrt`](scenes/render.pbrt) | Render |
| [`render-denoise*.pbrt`](scenes/render-denoise.pbrt) | Render and denoise using our denoiser |
| [`render-for-ours.pbrt`](scenes/render-for-ours.pbrt) | Render and output buffers required for our denoiser |
| [`render-for-moon.pbrt`](scenes/render-for-moon.pbrt) | Render and output buffers required for [Moon et al.'s confidence-interval approach [2013]](https://doi.org/10.1111/cgf.12004) |
| [`render-for-optix-oidn.pbrt`](scenes/render-for-optix-oidn.pbrt) | Render and output buffers required for [OptiX](https://developer.nvidia.com/optix-denoiser) and [OIDN](https://www.openimagedenoise.org/) |
| [`render-for-proden.pbrt`](scenes/render-for-proden.pbrt) | Render and outputs buffer required for [ProDen [Firmino et al. 2022]](https://doi.org/10.1111/cgf.14454) |
| [`reference.pbrt`](scenes/reference.pbrt) | Render a reference |
| [`denoise.pbrt`](scenes/denoise.pbrt) | Denoise using our denoiser and previously stored buffers |
| [`rr.pbrt`](scenes/rr.pbrt) | Render using standard Russian roulette |
| [`acrr.pbrt`](scenes/acrr.pbrt) | Render using approximate-contribution Russian roulette (ACRR) |
| [`rr-it-stats.pbrt`](scenes/rr-it-stats.pbrt) | Render using standard Russian roulette, tracking per-iteration statistics for evaluating sampling efficiency |
| [`acrr-it-stats.pbrt`](scenes/acrr-it-stats.pbrt) | Render using approximate-contribution Russian roulette (ACRR), tracking per-iteration statistics for evaluating sampling efficiency |
| [`mis.pbrt`](scenes/mis.pbrt) | Render using standard multiple importance sampling |
| [`smis.pbrt`](scenes/smis.pbrt) | Render using selective multiple importance sampling (SMIS) |

As can be seen in the scripts for reproducing the figures, a configuration file is activated by overwriting `scenes/_active.pbrt` with it.
Once a configuration is activated, pbrt can be run normally, supplying the desired scene description file as parameter, e.g.,:
```bash
pbrt ../../scenes/bathroom/scene-stat.pbrt
```

### Buffer System

This section provides an overview of the buffer system in `StatPathIntegrator`, which enables working with various inputs for and outputs from our denoiser.
Note that a more detailed description goes beyond the scope of this overview; for more details, refer to the code itself or [contact us](https://www.cg.tuwien.ac.at/staff/HiroyukiSakai)!

There are seven **buffer types**:

| Index | Name | Box-Cox Transformation | Description |
| - | - | - | - |
| 0 | `Radiance` | applied | Monte Carlo radiance estimate |
| 1 | `MISBSDFWinRate` | not applied | Multiple-importance-sampling (MIS) win rate for BSDF sampling used for SMIS |
| 2 | `MISLightWinRate` | not applied | MIS win rate for light sampling used for SMIS |
| 3 | `StatMaterialID` | not applied | Material ID G-buffer |
| 4 | `StatDepth` | not applied | Depth G-buffer |
| 5 | `StatNormal` | not applied | Normal G-buffer |
| 6 | `StatAlbedo` | not applied | Albedo G-buffer |
| 7 | `ItRadiance` | not applied | **Per-iteration** Monte Carlo radiance estimate used for evaluating the sampling efficiency of ACRR and SMIS |

These types are enabled as required by the [`StatPathIntegrator` configuration](#extended-scene-description-format).
For instance,

- `smis` enables `MISBSDFWinRate`, `MISLightWinRate`,
- `filterbuffers` determines the enabled G-buffer types, and
- `calcitstats` enables `ItRadiance`.

The Box-Cox transformation of radiance samples makes our approach more robust to non-normality; details can be found in [our paper](https://www.cg.tuwien.ac.at/StatMC).

Each **enabled** type is assigned a consecutively numbered ID (for performance reasons).
For instance, if denoising, SMIS, as well as normal, and albedo G-buffers are enabled, IDs would be assigned as follows:

| ID | Name |
| - | - |
| 0 | `Radiance` |
| 1 | `MISBSDFWinRate` |
| 2 | `MISLightWinRate` |
| 3 | `StatNormal` |
| 4 | `StatAlbedo` |

For each enabled type, a set of buffers is created.
The maximum number of buffers in this set is determined by the `trackedbounces` option, where each buffer corresponds to a specific bounce index, starting from the camera.
These per-bounce buffers are required for ACRR and SMIS, as described in Section 7 of our paper.

Based on the configuration and these rules, the following buffers are potentially created:

| Type | Name | Description |
| - | - | - |
| RGB | `film` | Noisy rendered image |
| RGB | `film-f` | Denoised rendered image |
| integer | `tX-bY-n` | Number of samples taken for type `X` at bounce `Y` | 
| RGB/float | `tX-bY-mean` | Sample mean of potentially Box-Cox transformed samples for type `X` at bounce `Y` |
| RGB/float | `tX-bY-m2` | Sum of squared deviations of potentially Box-Cox transformed samples for type `X` at bounce `Y` (division by the number of samples gives the second sample central moment) |
| RGB/float | `tX-bY-m3` | Sum of cubed deviations of potentially Box-Cox transformed samples for type `X` at bounce `Y` (division by the number of samples gives the third sample central moment) |
| RGB/float | `tX-bY-mean-corr` | Johnson-corrected sample mean of potentially Box-Cox transformed samples for type `X` at bounce `Y` |
| RGB/float | `tX-bY-discriminator` | Discriminator term described in the supplementary document, Section B |
| RGB/float | `tX-bY-film-mean` | Sample mean of untransformed samples for type `X` at bounce `Y` (same as `tX-bY-mean` if Box-Cox transformation is not applied) |
| RGB/float | `tX-bY-film-m2` | Sum of squared deviations of untransformed samples for type `X` at bounce `Y` (same as `tX-bY-m2` if Box-Cox transformation is not applied; division by the number of samples gives the second sample central moment) |
| RGB/float | `tX-bY-film-mean-var` | Variance of the sample mean of untransformed samples for type `X` at bounce `Y` |
| RGB/float | `tX-bY-film-mean-f` | Denoised mean of untransformed samples for type `X` at bounce `Y` |

For the example above, `t0-b0-mean` holds the mean radiance for the zeroth bounce (i.e., at the camera) and `t1-b2-m2` holds the second central moment for the MIS BSDF win rates at the second bounce (starting from the camera).
If applicable, the `multichannelstats` option switches between RGB and float buffers.
The [`outputregex` option](#extended-scene-description-format) provides a convenient way to select output buffers.
Note that G-buffers are never computed or stored for bounces higher than zero.

### Limitations

`StatPathIntegrator` supports the `BoxFilter` only.


## Acknowledgments

We thank Lukas Lipp for fruitful discussions, Károly Zsolnai-Fehér and Jaroslav Křivánek for valuable contributions to early versions of this work, and Bernhard Kerbl for help with our CUDA implementation.
Moreover, we thank the creators of the scenes we have used: [Wig42](https://blendswap.com/profile/130393) for "Wooden Staircase" (Fig. 1), "Grey and White Room" (Fig. S6), and "Modern Living Room" (Fig. S8); [nacimus](https://www.blendswap.com/profile/72536) for "Bathroom" (Fig. 3, S5); NovaZeeke for "Japanese Classroom" (Fig. 4, 6); [Beeple](https://www.beeple-crap.com/) for "Zero-Day" (Fig. 8); [Jay-Artist](https://blendswap.com/profile/1574) for "White Room" (Fig. S7); [Mareck](https://www.blendswap.com/profile/53736) for "Contemporary Bathroom" (Fig. 2); Christian Freude for "Glass Caustics" (Fig. S10); and [Benedikt Bitterli](https://benedikt-bitterli.me/) for "Veach Ajar" (Fig. 7, S2), "Veach MIS" (Fig. S4), and "Fur Ball" (Fig. S11).
This work has received funding from the Vienna Science and Technology Fund (WWTF) project ICT22-028 ("Toward Optimal Path Guiding for Photorealistic Rendering") and the Austrian Science Fund (FWF) project F 77 (SFB "Advanced Computational Design").
