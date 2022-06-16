cobs = "/hps/nobackup/iqbal/leandro/cobs/original_cobs/build/src/cobs"
out_dir = "mac_integration_test"
reordered_assemblies_dir = "661k_compressed_high_quality/reordered_assemblies"
kmer_size = 1000
nb_positive_queries = 2000
nb_negative_queries = 2000
batches = ["neisseria_gonorrhoeae__01", "dustbin__01", "chlamydia_pecorum__01"]

print("set -eux")
for batch in batches:
    print(f"mkdir -p {out_dir}/reordered_assemblies/{batch}")
    print(f"mkdir -p {out_dir}/COBS_out")
    print(f"cp -L {reordered_assemblies_dir}/{batch}/*.contigs.fa.gz {out_dir}/reordered_assemblies/{batch}")
    print(
        f"{cobs} classic-construct -T 1 {out_dir}/reordered_assemblies/{batch} {out_dir}/COBS_out/{batch}.cobs_classic")
    print(
        f"{cobs} generate-queries -T 1 -n {nb_negative_queries} -p {nb_positive_queries} -k {kmer_size} -o {out_dir}/{batch}_queries.fa {out_dir}/reordered_assemblies/{batch}")
    print(f"sed -i 's/N/A/g' {out_dir}/{batch}_queries.fa")
    print(
        f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {out_dir}/{batch}_queries.fa --load-complete -T 1 > {out_dir}/{batch}.query_results.load_complete.txt")
    print(
        f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {out_dir}/{batch}_queries.fa -T 1 > {out_dir}/{batch}.query_results.no_load_complete.txt")
