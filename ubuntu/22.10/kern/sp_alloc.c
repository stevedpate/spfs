// SPDX-License-Identifier: GPL-2.0

/*
 * sp_alloc.c - routines for allocating an inode and a data block.
 *
 * Copyright (c) 2023 Steve D. Pate
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include "spfs.h"

/*
 * Allocate a new inode. We update the superblock and return
 * the inode number.
 */

ino_t
sp_ialloc(struct super_block *sb)
{
    struct spfs_sb_info  *sbi = SBTOSPFSSB(sb);
    int                   i;

    if (sbi->s_nifree == 0) {
        printk("spfs: Out of inodes\n");
    } else {
        mutex_lock(&sbi->s_lock);
        for (i = 4 ; i < SP_MAXFILES ; i++) {
            if (sbi->s_inode[i] == SP_INODE_FREE) {
                sbi->s_inode[i] = SP_INODE_INUSE;
                sbi->s_nifree--;
                printk("spfs: sp_ialloc alloc inode %d\n", i);
                mutex_unlock(&sbi->s_lock);
                break;
            }
        }
    }
    return i;
}

/*
 * Allocate a new data block. We update the superblock and return
 * the new block number.
 */

int
sp_block_alloc(struct super_block *sb)
{
    struct spfs_sb_info  *sbi = SBTOSPFSSB(sb);
    int                   i;

    if (sbi->s_nbfree == 0) {
        printk("spfs: Out of space\n");
        return 0;
    }

    /*
     * Start looking at block 2. Block 0 is for the root directory.
     * Block 1 is for lost+found.
     */

    mutex_lock(&sbi->s_lock);
    for (i = 1 ; i < SP_MAXBLOCKS ; i++) {
        if (sbi->s_block[i] == SP_BLOCK_FREE) {
            sbi->s_block[i] = SP_BLOCK_INUSE;
            sbi->s_nbfree--;
            /* XXX sb->s_dirt = 1;*/
            mutex_unlock(&sbi->s_lock);
            return SP_FIRST_DATA_BLOCK + i;
        }
    }
    printk("spfs: sp_block_alloc - We should never reach here\n");
    mutex_unlock(&sbi->s_lock);
    return 0;
}
