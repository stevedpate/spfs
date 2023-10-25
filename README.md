# SPFS - A simple filesystem for Linux

The goal of SPFS is to help people understand how to develop a kernel-based filesystem for Linux. It will be used as a teaching aid in a future open-source book on Linux filesystems and as the basis for an Udemy course on Linux filesystem development.

## About SPFS

Here are the main characteristics of SPFS:

    - Multi-level directories (directories within directories)
    - Fixed block size (2048 bytes).
    - A filename length up to 28 characters.
    - 760 blocks within the whole filesystem.
    - A maximum file size of approximately 505 KB.
    - File undelete using the SPFS `fsdb` command.
    - File creation, deletion, rename, ... 

Most of these limitations are in place to keep the on-disk structures very simple which result in the code to access them being much easier to understand. The goal here is for teaching.

