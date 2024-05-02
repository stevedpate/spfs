// SPDX-License-Identifier: GPL-2.0

/*
 * fsdb.c - This is the SPFS fsdb command. A menu of options can be
 *          seen with the "h" command. There is no error checking so
 *          commands entered so must be entered correctly.
 *
 * Copyright 2023-2024 Steve D. Pate
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <linux/fs.h>
#include "../kern/spfs.h"

struct sp_superblock       sb;
int                        devfd;

void
display_help()
{
	printf("q  - quit\n");
	printf("i  - display inode (e.g. \"i2\")\n");
	printf("s  - display superblock\n");
	printf("si - display superblock inode list\n");
	printf("sd - display superblock data block list\n");
	printf("d  - display contents of directory block\n");
	printf("b  - display block contents in ASCII where possible "
           "(e.g. \"b131\")\n");
	printf("u  - undelete file (will prompt for inode)\n");
}

/*
 * Read in an inode from disk. Inside lseek() we calculate the offset
 * within the device where the inode is located.
 */

int
read_inode(ino_t inum, struct sp_inode *spi, int read_anyway)
{
	if (sb.s_inode[inum] == SP_INODE_FREE && !read_anyway) {
		printf("Inode is free\n");
		return 0;
	}
	lseek(devfd, (SP_INODE_BLOCK * SP_BSIZE) + (inum * SP_BSIZE), SEEK_SET);
	read(devfd, (char *)spi, sizeof(struct sp_inode));
	return 1;
}

/* sp_diradd() - used during undelete. We longer have access to the file's
 *               name since that was in the parent's list of directory entries
 *               and is no longer accessible (zeroed out when the file was
 *               removed. So we create a file in lost+found using the inode
 *               number as the name of the file.
 *
 *               NOTE - if there is no more space in left in any of the
 *                      allocated blocks for lost+found, we return an error
 *                      instead of trying allocated a new block.
 */

int
sp_diradd(struct sp_inode *spi, int inum)
{
    struct sp_dirent    *dirent;
	char				name[16], disk_blk[SP_BSIZE];
    int                 blk = 0;
    int                 error =  0, i, pos;

	sprintf(name, "%d", inum);

    /*
     * i_blocks is the number of blocks allocated to lost+found. We loop
     * through each block one at a time until we find an empty slot 
     * (d_ino != 0). Each block contains a number of directory entries
     * defined by "struct sp_dirent".
     */
    
    for (blk=0 ; blk < spi->i_blocks ; blk++) {
		lseek(devfd, spi->i_addr[blk] * SP_BSIZE, SEEK_SET);
		read(devfd, disk_blk, SP_BSIZE);
        dirent = (struct sp_dirent *)disk_blk;
        for (i=0 ; i < SP_DIRS_PER_BLOCK ; i++) {
            if (dirent->d_ino != 0) {
                dirent++;
                continue;
            } else { /* we've found an empty slot */
                dirent->d_ino = inum;
                strcpy(dirent->d_name, name);
                lseek(devfd, spi->i_addr[blk] * SP_BSIZE, SEEK_SET);
				write(devfd, disk_blk, SP_BSIZE);
                return 0;
            }
        }
    }
    return error;
}

/*
 * Undelete a file - we ask for the inode number and then reconnect
 * that inode to lost+found. So if you ask for inode 10, you'll get
 * /lost+found/<inum> as the recovered file.
 */

void
undelete_file(void)
{
	struct sp_inode spi, lfip;
	char			buf[16], inode_buf[SP_BSIZE];
	int				i, inum, error;

	printf("inode num > ") ;
	fflush(stdout);
	scanf("%s", buf);
	inum = atoi(buf);

	/*
	 * 1. Read in the inode or whatever is in that block.
	 */

	read_inode(inum, &spi, 1);

	/*
	 * 2. Try and validate some fields - at least mode (IFREG)
	 */

	if (!S_ISREG(spi.i_mode)) {
		printf("Doesn't look like a regular file to me\n");
		return;
	}

	/*
	 * 3. Mark inode in SB as SP_INODE_INUSE. First check to make sure 
	 *    blocks are not in use first. If so, print error and return.
	 */

	if (sb.s_inode[inum] == SP_INODE_INUSE) {
		printf("Sorry but this inode is in use (reallocated?)\n");
		return;
	}
	sb.s_inode[inum] = SP_INODE_INUSE;
	sb.s_nifree--;

	/*
	 * 4. For all blocks (i) listed in the inode (based on i_size)
		  mark sb.s_block[i]) as inuse 
	 */

	for (i = 0 ; i < spi.i_blocks ; i++) {
		if (sb.s_block[spi.i_addr[i]] != SP_BLOCK_FREE) {
			sb.s_inode[inum] = SP_INODE_FREE;
	        sb.s_nifree++;
			printf("Block %d in use so can't undelete inode\n", spi.i_addr[i]);
			return;
		}
	}

	for (i = 0 ; i < spi.i_blocks ; i++) {
		sb.s_block[spi.i_addr[i]] = SP_BLOCK_INUSE;
		sb.s_nbfree--;
	}

	/*
	 * 5. Add entry to lost+found. Assumes lost+found exists.
	 */

	read_inode(3, &lfip, 0);
	error = sp_diradd(&lfip, inum); /* XXX - can fail if no space available */
	lfip.i_size += SP_DIRENT_SIZE;
	lseek(devfd, (SP_INODE_BLOCK * SP_BSIZE) + (3 * SP_BSIZE), SEEK_SET);
    write(devfd, (char *)&lfip, sizeof(struct sp_inode));

	/*
	 * 6. Write the superblock to reflect changes
	 */

	lseek(devfd, 0, SEEK_SET);
	write(devfd, (char *)&sb, sizeof(struct sp_superblock));
}

/*
 * print_directory_block() - print valid directory entries in the 
 *                           specified block.
 *
 * A directory block is just a series of sp_dirent structures as follows:
 *
 * struct sp_dirent {
 *        __u32       d_ino;
 *        char        d_name[SP_NAMELEN];
 * };
 *
 * If the d_ino field is '0' the entry is "free" and d_name[0] will be '\0'.
 * SP_NAMELEN is 28 bytes so the total size = 28 + 4 bytes (d_ino) = 32 bytes.
 * Therefore, the number of entries in a block is SB_BSIZE / 32 which is 64.
 * Only valid entries will be printed out (where d_ino != 0).
 */

void
print_directory_block(int blk)
{
	char	            buf[SP_BSIZE];
    struct sp_dirent    *dirent = (struct sp_dirent *)buf;
	int		            x, y;

	lseek(devfd, blk * SP_BSIZE, SEEK_SET);
	read(devfd, buf, SP_BSIZE);
	for (x = 0 ; x < (SP_BSIZE / sizeof(struct sp_dirent)); x++) {
        if (dirent->d_ino == 0) {
            continue; /* slot does not contain an inode */
        } else {
            printf("%2d - inum = %2d, name = %s\n",
                   x, dirent->d_ino, dirent->d_name);
        }
    dirent++;
	}
}

/*
 * Blocks are printed in rows of 32 bytes in hexadecimal and ASCII.
 * The block offset is displayed first (also in hex). If a character
 * is non-printable, a "." is dsiplayed.
 */

void
print_block(int blk)
{
	char	buf[SP_BSIZE];
	int		x, y;

	lseek(devfd, blk * SP_BSIZE, SEEK_SET);
	read(devfd, buf, SP_BSIZE);
	for (x = 0 ; x < SP_BSIZE ; x += 16) {
		printf("%04x  ", x);
		for (y = 0 ; y < 16 ; y += 2) {
			printf("%02x%02x ", (int)(buf[x + y]), (int)(buf[x + y + 1]));
		}
		printf("  ");
		for (y = 0 ; y < 16 ; y += 1) {
			if (isprint(buf[x + y])) {
				printf("%c", buf[x + y]);
			} else {
				printf(".");
			}
		}
		printf("\n");
	}
}

/*
 * We are passed an inode (struct sp_inode) so simply display its contents. 
 * We will display blocks allocated and if it's a directory, we'll display 
 * valid directory entries (also see the "d" command).
 */

void
print_inode(int inum, struct sp_inode *spi)
{
	char                    buf[SP_BSIZE];
	struct sp_dirent        *dirent;
	int                     i, x, pi = 0;
	time_t					tm;

	printf("inode number %d\n", inum);
	printf("  i_mode     = %x\n", spi->i_mode);
	printf("  i_nlink    = %d\n", spi->i_nlink);
	tm = spi->i_atime;
	printf("  i_atime    = %s", ctime(&tm));
	tm = spi->i_mtime;
	printf("  i_mtime    = %s", ctime(&tm));
	tm = spi->i_ctime;
	printf("  i_ctime    = %s", ctime(&tm));
	printf("  i_uid      = %d\n", spi->i_uid);
	printf("  i_gid      = %d\n", spi->i_gid);
	printf("  i_size     = %d\n", spi->i_size);
	printf("  i_blocks   = %d\n", spi->i_blocks);
    if (spi->i_blocks) {
        for (i=0 ; i<SP_DIRECT_BLOCKS; i++) {
            if (i % 3 == 0 && pi == 3) {
                    printf("\n");
                    pi = 0;
            }
            if (spi->i_addr[i] != 0) {
                printf("  i_addr[%2d] = %3d ", i, spi->i_addr[i]);
                pi++;
            }
        }
    }

	/*
	 * Print out the directory entries if this is a directory.
	 */

	if (S_ISDIR(spi->i_mode)) {
		printf("\n\n  Directory entries:\n");
		for (i=0 ; i < spi->i_blocks ; i++) {
			lseek(devfd, spi->i_addr[i] * SP_BSIZE, SEEK_SET);
			read(devfd, buf, SP_BSIZE);
			dirent = (struct sp_dirent *)buf;
			for (x = 0 ; x < SP_BSIZE / sizeof(struct sp_dirent) ; x++) {
				if (dirent->d_ino != 0) {
					printf("    inum[%2d], name[%s]\n",
						   dirent->d_ino, dirent->d_name);
				} 
				dirent++;
			}
		}
		printf("\n");
	} else if (S_ISLNK(spi->i_mode)) {
        printf("  symlink    = %s\n", (char *)spi->i_addr);
    } else {
		printf("\n");
	}
}

int
main(int argc, char **argv)
{
	struct sp_inode           inode;
	char                      buf[512];
	char                      command[512];
	off_t                     nsectors;
	int                       error, i, blk;
	ino_t                     inum;

	devfd = open(argv[1], O_RDWR);
	if (devfd < 0) {
		fprintf(stderr, "spfs-mkfs: Failed to open device\n");
		return(1);
	}

	/*
	 * Read in and validate the superblock. We don't check the clean/dirty
     * flag at this point.
	 */

	read(devfd, (char *)&sb, sizeof(struct sp_superblock));
	if (sb.s_magic != SP_MAGIC) {
		printf("This is not an SPFS filesystem\n");
		return(1);
	}

	while (1) {
		printf("spfsdb > ") ;
		fflush(stdout);
		error = scanf("%s", command);
		if (command[0] == 'q') {
			return(0);
		}
		if (command[0] == 'h') {
			display_help();
		}
		if (command[0] == 'u') {
			undelete_file();
		}
		if (command[0] == 'b') {
			blk = atoi(&command[1]);
			print_block(blk);
		}
		if (command[0] == 'd') {
			blk = atoi(&command[1]);
            print_directory_block(blk);
		}
		if (command[0] == 'i') {
			inum = atoi(&command[1]);
			if (read_inode(inum, &inode, 0)) {
				print_inode(inum, &inode);
			}
		}
		if (command[0] == 's' && command[1] == 'd') {
			for (i=0 ; i < (SP_MAXBLOCKS - SP_FIRST_DATA_BLOCK) ; i++) {
				printf("  s_block[%3d] = %s", SP_FIRST_DATA_BLOCK + i, 
					   sb.s_block[i] == SP_BLOCK_INUSE ? "inuse" : "free ");
                if ((i+1) % 3 == 0) {
                   printf("\n");
                }
			}
            printf("\n");
        }
		if (command[0] == 's' && command[1] == 'i') {
			for (i=0 ; i<42 ; i++) {
				printf("  s_inode[%2d] = %s", i, 
					   sb.s_inode[i] == SP_INODE_INUSE ? "inuse" : "free ");
                if ((i+1) % 3 == 0) {
                   printf("\n");
                }
			}
        }
		if (command[0] == 's' && command[1] == '\0') {
			printf("Superblock contents:\n");
			printf("  s_magic   = 0x%x\n", sb.s_magic);
			printf("  s_mod     = %s\n", (sb.s_mod == SP_FSCLEAN) ?
				   "SP_FSCLEAN" : "SP_FSDIRTY");
			printf("  s_nifree  = %d\n", sb.s_nifree);
			printf("  s_nbfree  = %d\n", sb.s_nbfree);
		}
	}
}
