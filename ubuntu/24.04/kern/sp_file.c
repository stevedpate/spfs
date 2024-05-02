// SPDX-License-Identifier: GPL-2.0

/*
 * sp_file.c - regular file operations.
 *
 * Copyright (c) 2023-2024 Steve D. Pate
 */

#include <linux/fs.h>
#include <linux/mpage.h>
#include <linux/buffer_head.h>
#include "spfs.h"

struct file_operations sp_file_operations = {
	.fsync			= generic_file_fsync,
	.llseek			= generic_file_llseek,
	.read_iter		= generic_file_read_iter,
	.write_iter		= generic_file_write_iter,
	.mmap			= generic_file_mmap,
	.unlocked_ioctl	= sp_ioctl
};

/*
 * Called when reading or writing to a regular file. If 'create' is set
 * we need to allocate a block if one does not exist already.
 */

int
sp_get_block(struct inode *inode, sector_t block, 
             struct buffer_head *bh_result, int create)
{
	unsigned long			phys;
	struct super_block		*sb = inode->i_sb;
	struct sp_inode_info	*spi = ITOSPI(inode);
	int						blk;

	printk("spfs: sp_get_block (inode = %px, block = %d)\n", inode, (int)block);

	/*
	 * Regardless of whether we're reading or writing, if the block
     * requested exists, map it and return. On the read path, the 
     * data will be read in. For writes, it will be read in and then
     * modifications will be made to the page.
	 */

    phys = spi->i_addr[block];
    if (phys) {
        map_bh(bh_result, sb, phys);
        return 0;
    }

    /*
     * If we're reading and don't have a block backing on disk,
     * we must be reading over a hole. Leave that to the kernel.
     */

    if (!create) {
        return 0;
    }

	/*
	 * We're writing. First check to see is the file can be extended.
	 */

	if (block >= SP_DIRECT_BLOCKS) {
		return -EFBIG;
	}

	/*
	 * We must allocate a new block. Assuming we get a block, we map it 
     * and specify that the buffer is new which will avoid the kernel 
     * reading it data from disk.
	 */

    blk = sp_block_alloc(sb);
    if (blk == 0) {
        printk("spfs: sp_get_block - out of space\n");
        return -ENOSPC;
    }
    printk("spfs: sp_get_block - allocated blk=%d \n", blk);
    spi->i_addr[block] = blk;
    spi->i_blocks++;
    mark_inode_dirty(inode);
    set_buffer_new(bh_result);
    map_bh(bh_result, sb, blk);

	return 0;
}

void
sp_write_failed(struct address_space *mapping, loff_t to)
{
    struct inode *inode = mapping->host;

	printk("spfs: sp_write_failed for inode=%px\n", inode);
    if (to > inode->i_size) {
        truncate_pagecache(inode, inode->i_size);
	}
}

int
sp_write_begin(struct file *file, struct address_space *mapping,
			   loff_t pos, unsigned len, struct page **pagep, void **fsdata)
{
	struct inode	*inode	= mapping->host;
    int				ret;

	printk("spfs: sp_write_begin for inode=%px, off=%lld, len=%d\n",
		   inode, pos, len);
    ret = block_write_begin(mapping, pos, len, pagep, sp_get_block);
    if (unlikely(ret)) {
        sp_write_failed(mapping, pos + len);
	}
	printk("spfs: sp_write_begin got page=%px\n", *pagep);
    return ret;
}

int
sp_write_end(struct file *file, struct address_space *mapping,
			 loff_t pos, unsigned len, unsigned copied,
			 struct page *page, void *fsdata)
{
	struct inode	*inode	= mapping->host;
    int				error;

	printk("spfs: sp_write_end for inode=%px, off=%lld, len=%d, page=%px\n",
		   inode, pos, len, page);
	error = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
    return error;
}

static int sp_writepages(struct address_space *mapping,
                         struct writeback_control *wbc)
{
    return mpage_writepages(mapping, wbc, sp_get_block);
}

int
sp_read_folio(struct file *file, struct folio *folio)
{
	printk("spfs: sp_read_folio\n");
    return block_read_full_folio(folio, sp_get_block);
}

sector_t
sp_bmap(struct address_space *mapping, sector_t block)
{   
	printk("spfs: sp_bmap for sector=%lld\n", block);
	return generic_block_bmap(mapping, block, sp_get_block);
}

struct address_space_operations sp_aops = {
	.dirty_folio		= block_dirty_folio,
	.invalidate_folio	= block_invalidate_folio,
	.read_folio			= sp_read_folio,
	.writepages			= sp_writepages,
	.write_begin		= sp_write_begin,
	.write_end			= sp_write_end,
	.bmap				= sp_bmap
};

struct inode_operations sp_file_inops = {
	.link	= sp_link,
	.unlink	= sp_unlink,
};
