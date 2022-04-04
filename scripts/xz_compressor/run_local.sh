#!/usr/bin/env bash
set -eux
snakemake -j4 --use-singularity
