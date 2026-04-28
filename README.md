# Multithreaded Operations Implementation for Kiwi

A modification of the **Kiwi** storage engine that provides multithreaded operation capabilities, implemented in C as part of the "Operating Systems" course at the University of Ioannina, Department of Computer Science and Engineering in collaboration with C. G. Mousses.

## About the Kiwi storage engine

The **Kiwi** storage engine is based on a log-structured merge tree (LSM tree).
Storage engines are a critical component of modern cloud infrastructures,
as they are responsible for storing and retrieving data on a machine’s local devices.
A distributed storage system uses thousands of such machines to achieve scalable and reliable operation.

The LSM tree is the data structure on which storage machines are often based.
The provided programming interface (API) includes put and get operations for key-value pairs.
The put function takes as a parameter a key-value pair that must be added to the structure.
The get function accepts a key as a parameter and retrieves the corresponding value if a key-value pair with that
specific key is stored in the structure; otherwise, it returns an error if the key is not found.

## About the modifications to the engine
We have modified the **Kiwi** storage engine so that it
supports multithreading for the "put" and "get" commands it already provides.
Our implementation creates a specific number of threads defined by the user
and allows them to call the "put" and "get" commands simultaneously,
as well as to specify the percentage of threads executing each operation.
The storage engine has also been modified accordingly so that
it performs these operations simultaneously and maintainsstatistics on their execution time.

## Usage
```bash
./kiwi-bench <read|write> <op_num> [random]
./kiwi-bench <p-read> <op_num> <thread_num> [random]
./kiwi-bench <p-rw> <op_num> <thread_num> <writer_perc> [random]

read: serial read from database
write: serial write to database
p-read: parallel read from database
p-rw: parallel read and write
op_num: number of operations to execute
thread_num: number of threads (>0) to execute op_num operations
writer_perc: percentage of write operations in [0, 100] (p-rw mode only)
random: if present, operation values are randomized
```

## Additional Information
Additional information on:
- The Kiwi engine and project spcifications can be found in `MYY601-L1-2023-GR.pdf`
- The modifications implemented and performance metrics in `Report.pdf`
- The raw performance metrics in `stats`
