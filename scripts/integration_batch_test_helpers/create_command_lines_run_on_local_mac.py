cobs = "cobs"
out_dir = "local_mac_results"
linux_results = "linux_results"
reordered_assemblies_dir = f"{linux_results}/reordered_assemblies"
batches = ["neisseria_gonorrhoeae__01", "dustbin__01", "chlamydia_pecorum__01"]

print("set -eux")
for batch in batches:
    print(f"mkdir -p {out_dir}/COBS_out")
    print(
        f"{cobs} classic-construct -T 1 {reordered_assemblies_dir}/{batch} {out_dir}/COBS_out/{batch}.cobs_classic")
    print(
        f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {linux_results}/{batch}_queries.fa --load-complete -T 1 > {out_dir}/{batch}.query_results.load_complete.txt")
    print(
        f"{cobs} query -i {out_dir}/COBS_out/{batch}.cobs_classic -f {linux_results}/{batch}_queries.fa -T 1 > {out_dir}/{batch}.query_results.no_load_complete.txt")
    print(
        f"{cobs} query -i {linux_results}/COBS_out/{batch}.cobs_classic -f {linux_results}/{batch}_queries.fa --load-complete -T 1 > {out_dir}/{batch}.query_results.load_complete.against_linux_index.txt")
    print(
        f"{cobs} query -i {linux_results}/COBS_out/{batch}.cobs_classic -f {linux_results}/{batch}_queries.fa -T 1 > {out_dir}/{batch}.query_results.no_load_complete.against_linux_index.txt")
