# Scripts to Precompute Albedos by Hiroyuki Sakai

In this directory, you can find the scripts required to precompute the values for the albedo LUTs.
You can find more information about albedo [here](https://pbr-book.org/3ed-2018/Reflection_Models/Basic_Interface#Reflectance).

## Motivation

Precomputing albedos and using it during rendering is much more efficient than computing albedos on the fly.
Benchmarks have shown that the performance increase can be about two magnitudes.
This increase can be observed by using the `--benchmark` option of the `precomputealbedo` program (described below).

## Caveats

For complex materials involving many parameters, a LUT becomes infeasible, as the LUT size increases exponentially with each LUT dimension (this limitation includes measured materials).
Furthermore, we have not considered anisotropic materials as of now.

## Usage

1.  Compile the project according to the [README](README.md).
2.  Perform the precomputation using:

    ```
    ./build/precomputealbedo --material MatteMaterial --nsamples 1024 [> matte.json]
    ```

There are also more options:
`--nthreads`: Set the number of threads (default: use the detected number of cores)
`--seedoffset`: Set the seed offset for the RNGs (useful to combine results from multiple runs; default: 0)
`--comparetopbrt`: Compare calculated values to those from pbrt-v3's rho() function and emit warnings in case of significant differences
`--testlut`: Randomize material parameters and compare calculated values to those from the LUT and emit warnings in case of significant differences
`--benchmark`: Do NOT calculate albedos and test performance of LUT lookups vs. pbrt rho() calls instead

The script produces a JSON file that contains information about the sampling process including the results.

3. Convert the JSON file to a C++ array using:

```
./json2cpp.sh matte.json
```

The script relies on a compiled C++ program called `albedojson2dat` and two Python scripts included in `json2cpp`.
It should not be necessary to look at these files; however, it might be necessary to adapt paths in `json2cpp.sh`, depending on the system configuration.
The script generates a `.cpp` file with a C++ array initialization that can be used as desired.
