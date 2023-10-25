/* 
 * fillfs.c - similar to mkfs but adds some additional files which make
 *            it easier to get started (running "ls", "cat" etc), i.e.
 *            some simple, read-only type commands.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include "../kern/spfs.h"

int devfd;

/*
 * fill_in_inode() - write an inode to disk. We will in the fields of
 *                   the disk inode, lseek to the right location on
 *                   disk and write it. The first inode is stored at
 *                   SP_INODE_BLOCK. Since inodes 0 and 1 are not used,
 *                   the root inode (2) is stored at block SP_INODE_BLOCK + 2
 *                   and so on. Inode fields not shown are filled in by
 *                   the caller.
 */

void
fill_in_inode(struct sp_inode *inode, int type, int uid, int gid, 
			  int nlink, int inum)
{
	time_t	tm;

	time(&tm);
	inode->i_atime = (__u32)tm;
	inode->i_mtime = (__u32)tm;
	inode->i_ctime = (__u32)tm;
	inode->i_uid = (__u32)uid;
	inode->i_gid = (__u32)gid;
	inode->i_mode = (__u32)type;
	inode->i_nlink = (__u32)nlink;

	lseek(devfd, (SP_INODE_BLOCK + inum) * SP_BSIZE, SEEK_SET);
	write(devfd, (char *)inode, sizeof(struct sp_inode));
}

/*
 * main() - quite simple. Write the superblock, fill in inode structures,
 *          write to disk and then write relevant blocks which are the
 *          directory entries for root and lost+found.
 *
 *          Unlike mkfs, we create another file and give it come contents.
 */

int
main(int argc, char **argv)
{
        struct sp_dirent        dir;
        struct sp_superblock    sb;
        struct sp_inode         inode;
        time_t                  tm;
        off_t                   nsectors = SP_MAXBLOCKS;
        int                     error, i;
        int                     map_blks, inum;
        char                    block[SP_BSIZE];

        if (argc != 2) {
                fprintf(stderr, "SPFS mkfs: Need to specify device\n");
                return(1);
        }
        devfd = open(argv[1], O_WRONLY);
        if (devfd < 0) {
                fprintf(stderr, "SPFS mkfs: Failed to open device\n");
                return(1);
        }
        error = lseek(devfd, (off_t)(nsectors * 512), SEEK_SET);
        if (error == -1) {
                fprintf(stderr, "SPFS mkfs: Cannot create filesystem"
                        " of specified size\n");
                return(1);
        }
        lseek(devfd, 0, SEEK_SET);

        /*
         * Fill in the fields of the superblock and write
         * it out to the first block of the device.
         */

        memset((void *)&sb, 0, sizeof(struct sp_superblock));

        sb.s_magic = SP_MAGIC;
        sb.s_mod = SP_FSCLEAN;
        sb.s_nifree = SP_MAXFILES - 5;  
        sb.s_nbfree = SP_MAXBLOCKS - 3;

        /*
         * First 4 inodes are in use. Inodes 0 and 1 are not
         * used by anything, 2 is the root directory and 3 is
         * lost+found. Others were marked FREE by memset above
         */

        sb.s_inode[0]  = SP_INODE_INUSE;
        sb.s_inode[1]  = SP_INODE_INUSE;
        sb.s_inode[2]  = SP_INODE_INUSE;
        sb.s_inode[3]  = SP_INODE_INUSE;
        sb.s_inode[4]  = SP_INODE_INUSE;

        /*
         * The first two blocks are allocated for the entries
         * for the root and lost+found directories. Others were 
		 * marked FREE by memset above
         */

        sb.s_block[0] = SP_BLOCK_INUSE; /* root directory entries */
        sb.s_block[1] = SP_BLOCK_INUSE; /* lost_found directory entries */
        sb.s_block[2] = SP_BLOCK_INUSE; /* contents for /hello */

        write(devfd, (char *)&sb, sizeof(struct sp_superblock));

        /*
         * The root directory and lost+found directory inodes
         * must be initialized and written to disk.
		 *
		 * Link count for root is 3 - ".", ".." and "lost+found"
		 * Link count for lost+found is 2 - "." and ".."
		 *
		 * fill_in_inode(&inode, type, uid, gid, nlink, inum)
         */

		memset((void *)&inode, 0, sizeof(struct sp_inode));
        inode.i_size = 4 * sizeof(struct sp_dirent);
        inode.i_blocks = 1;
        inode.i_addr[0] = SP_FIRST_DATA_BLOCK;
		fill_in_inode(&inode, S_IFDIR | 0755, 0, 0, 3, 2);

		memset((void *)&inode, 0, sizeof(struct sp_inode));
        inode.i_size = 2 * sizeof(struct sp_dirent);
        inode.i_blocks = 1;
        inode.i_addr[0] = SP_FIRST_DATA_BLOCK + 1;
		fill_in_inode(&inode, S_IFDIR | 0755, 0, 0, 2, 3);

		char *file_contents = "Hello, this is a file\n";

		memset((void *)&inode, 0, sizeof(struct sp_inode));
        inode.i_size = strlen(file_contents);
        inode.i_blocks = 1;
        inode.i_addr[0] = SP_FIRST_DATA_BLOCK + 2;
		fill_in_inode(&inode, S_IFREG | 0644, 0, 0, 1, 4);

        /*
         * Fill in the directory entries for root 
         */

        lseek(devfd, SP_FIRST_DATA_BLOCK * SP_BSIZE, SEEK_SET);
        memset((void *)block, 0, SP_BSIZE);
        write(devfd, block, SP_BSIZE);
        lseek(devfd, SP_FIRST_DATA_BLOCK * SP_BSIZE, SEEK_SET);
        dir.d_ino = 2;
        strcpy(dir.d_name, ".");
        write(devfd, (char *)&dir, sizeof(struct sp_dirent));
        dir.d_ino = 2;
        strcpy(dir.d_name, "..");
        write(devfd, (char *)&dir, sizeof(struct sp_dirent));
        dir.d_ino = 3;
        strcpy(dir.d_name, "lost+found");
        write(devfd, (char *)&dir, sizeof(struct sp_dirent));
        dir.d_ino = 4;
        strcpy(dir.d_name, "hello");
        write(devfd, (char *)&dir, sizeof(struct sp_dirent));

        /*
         * Fill in the directory entries for lost+found 
         */

        lseek(devfd, SP_FIRST_DATA_BLOCK * SP_BSIZE + SP_BSIZE, SEEK_SET);
        memset((void *)block, 0, SP_BSIZE);
        write(devfd, block, SP_BSIZE);
        lseek(devfd, SP_FIRST_DATA_BLOCK * SP_BSIZE + SP_BSIZE, SEEK_SET);
        dir.d_ino = 3;
        strcpy(dir.d_name, ".");
        write(devfd, (char *)&dir, sizeof(struct sp_dirent));
        dir.d_ino = 2;
        strcpy(dir.d_name, "..");
        write(devfd, (char *)&dir, sizeof(struct sp_dirent));

		/*
		 * Write to the file "hello" in the root directory
		 */

		lseek(devfd, (SP_FIRST_DATA_BLOCK + 2) * SP_BSIZE, SEEK_SET);
        memset((void *)block, 0, SP_BSIZE);
        memcpy(block, file_contents, strlen(file_contents));
        write(devfd, block, SP_BSIZE);
}
