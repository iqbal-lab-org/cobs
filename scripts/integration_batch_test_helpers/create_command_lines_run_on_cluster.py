cobs = "/hps/nobackup/iqbal/leandro/cobs/original_cobs/build/src/cobs"
out_dir = "mac_integration_test"
reordered_assemblies_dir = "661k_compressed_high_quality/reordered_assemblies"
batches = ["neisseria_gonorrhoeae__01", "dustbin__01", "chlamydia_pecorum__01"]
query_lengths = [100, 200, 500, 1000, 10_000, 100_000]
nb_positive_queries_per_query_length = 500
nb_negative_queries_per_query_length = 500


print("set -eux")
for batch in batches:
    print(f"mkdir -p {out_dir}/reordered_assemblies/{batch}")
    print(f"mkdir -p {out_dir}/COBS_out")
    print(f"cp -L {reordered_assemblies_dir}/{batch}/*.contigs.fa.gz {out_dir}/reordered_assemblies/{batch}")
    print(
        f"{cobs} classic-construct -T 1 {out_dir}/reordered_assemblies/{batch} {out_dir}/COBS_out/{batch}.cobs_classic")
    for query_length in query_lengths:
        batch_query_file = f"{out_dir}/{batch}_queries.query_length_{query_length}.fa"
        print(
            f"{cobs} generate-queries -T 1 -n {nb_negative_queries_per_query_length} "
            f"-p {nb_positive_queries_per_query_length} -k {query_length} "
            f"-o {batch_query_file} {out_dir}/reordered_assemblies/{batch}")
        print(f"sed -i 's/N/A/g' {batch_query_file}")
        print(
            f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {batch_query_file} --load-complete -T 1 > "
            f"{out_dir}/{batch}.query_results.query_length_{query_length}.load_complete.txt")
        print(
            f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {batch_query_file} -T 1 > "
            f"{out_dir}/{batch}.query_results.query_length_{query_length}.no_load_complete.txt")

