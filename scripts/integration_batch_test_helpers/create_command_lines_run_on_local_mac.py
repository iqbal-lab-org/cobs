cobs = "cobs"
out_dir = "local_mac_results"
linux_results = "linux_results"
reordered_assemblies_dir = f"{linux_results}/reordered_assemblies"
batches = ["neisseria_gonorrhoeae__01", "dustbin__01", "chlamydia_pecorum__01"]
query_lengths = [100, 200, 500, 1000, 10_000, 100_000]

print("set -eux")
for batch in batches:
    print(f"mkdir -p {out_dir}/COBS_out")
    print(
        f"{cobs} classic-construct -T 1 {reordered_assemblies_dir}/{batch} {out_dir}/COBS_out/{batch}.cobs_classic")

    for query_length in query_lengths:
        batch_query_file = f"{linux_results}/{batch}_queries.query_length_{query_length}.fa"
        print(
            f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {batch_query_file} --load-complete -T 1 > {out_dir}/{batch}.query_results.query_length_{query_length}.load_complete.txt")
        print(
            f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {batch_query_file} -T 1 > {out_dir}/{batch}.query_results.query_length_{query_length}.no_load_complete.txt")
        print(
            f"{cobs} query -i {linux_results}/COBS_out/{batch}.cobs_classic -f {batch_query_file} --load-complete -T 1 > {out_dir}/{batch}.query_results.query_length_{query_length}.load_complete.against_linux_index.txt")
        print(
            f"{cobs} query -i {linux_results}/COBS_out/{batch}.cobs_classic -f {batch_query_file} -T 1 > {out_dir}/{batch}.query_results.query_length_{query_length}.no_load_complete.against_linux_index.txt")
