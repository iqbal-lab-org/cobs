These are just some utilities to test if the `Linux` and `OS X` versions of `COBS` produce the same results, given that
`COBS` was not designed to work on `OS X` and I did some batch changes and was not sure if the results were indeed ok,
given that the unit tests are not comprehensive enough. Subsampling the datasets in these tests can make up for some
integration tests, but these batches are way too heavy for integration tests as they are.

* `create_command_lines_run_on_cluster.py`: this script builds commands that create three batches in a directory, as
well as the `COBS` indexes for these batches, and 2000 positive and negative queries with 1000-bp length each. These
queries are then ran agains the indexes with and without the `--load-complete` flag. This is meant to be run in the
cluster or another `linux` machine with a `linux` version of `COBS`.

* `create_command_lines_run_on_local_mac.py`: this script takes some of the input created previously and create
`COBS` indexes for the three batches, and do the queries mentioned previously against the `COBS OS X` indexes and also
against the `COBS Linux` indexes with and without the `--load-complete` flag. This enable us to verify that the `OS X`
version of `COBS` has the same results as the `Linux` version if the indexes were created in any of the OSes.

* `results.txt`: shows the SHA1sum of the query results I got after running both scripts above. It basically shows that
the `Linux` and `OS X` versions of `COBS` produced identical query results.