# xz_compressor

Creates and compresses ordered [COBS](https://github.com/bingmann/cobs) indexes to later be used by
[mof-search](https://github.com/karel-brinda/mof-search).

# Dependencies

* `python 3.7+`;
* `snakemake`;
* `singularity`;
* `xz`;
* `tar`;

# Running

## Input

Just configure the four entries in [config.yaml](config.yaml):
* `ordering_dir`: a directory containing batches (text files) of ordered samples (one per line),
see [sample_data/ordering](sample_data/ordering) for one example;
* `assemblies_dir`: a directory containing assemblies. This directory can be subdivided in batches.
See [sample_data/assemblies](sample_data/assemblies) for one example;
* `output_dir`: the output dir;
* `cobs_container`: the `COBS` container to use, can be used to specify a version of `COBS`.

## Running

There are premade scripts to run locally or on a `LSF` cluster:

### Running locally

```
bash run_local.sh
```

### Running on a LSF cluster

Please have the [LSF profile](https://github.com/Snakemake-Profiles/lsf) installed and run:

```
bash run_local.sh
```

## Output

The main output is in `<output_dir>/compressed_indexes`. There you can find the `xz`-compressed `COBS` indexes, one
per batch, and also all of them in the single `tar` file `all.cobs_classic.xz.tar`.

## Running on the sample example

Just run locally:

```
bash run_local.sh
```

and take a look at the output files in `sample_data/out`.

