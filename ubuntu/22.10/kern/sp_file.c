#include <linux/fs.h>
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
 * XXX - sectors are 512 bytes so same as SP_BSIZE for now
 * 		may need to factor that in at some point if we change blk size
 */

int
sp_get_block(struct inode *inode, sector_t block, 
             struct buffer_head *bh_result, int create)
{
	unsigned long			phys;
	struct super_block		*sb = inode->i_sb;
	struct sp_inode_info	*spi = ITOSPI(inode);
	int						blk;

	printk("spfs: sp_get_block (inode = %p, block = %d)\n", inode, (int)block);

	/*
	 * First check to see is the file can be extended.
	 */

	if (block >= SP_DIRECT_BLOCKS) {
		return -EFBIG;
	}

	/*
	 * If we're creating, we must allocate a new block.
	 */

	if (create) { /* XXX - needs checking ??? */
		blk = sp_block_alloc(sb);
		if (blk == 0) {
			printk("spfs: sp_get_block - out of space\n");
			return -ENOSPC;
		}
		printk("spfs: sp_get_block - allocated blk=%d \n", blk);
		spi->i_addr[block] = blk;
		spi->i_blocks++;
		mark_inode_dirty(inode);
		map_bh(bh_result, sb, blk);
	} else {
		phys = spi->i_addr[block];	/* sector = BLKSZ */
		printk("spfs: sp_get_block - not create, disk blk = %ld\n", phys);
		map_bh(bh_result, sb, phys);
	}
	return 0;
}

static void
sp_write_failed(struct address_space *mapping, loff_t to)
{
    struct inode *inode = mapping->host;

	printk("spfs: sp_write_failed for inode=%p\n", inode);
    if (to > inode->i_size) {
        truncate_pagecache(inode, inode->i_size);
	}
}

static int
sp_write_begin(struct file *file, struct address_space *mapping,
			   loff_t pos, unsigned len, struct page **pagep, void **fsdata)
{
	struct inode	*inode	= mapping->host;
    int				ret;

	printk("spfs: sp_write_begin for inode=%p, off=%lld, len=%d\n",
		   inode, pos, len);
    ret = block_write_begin(mapping, pos, len, pagep, sp_get_block);
    if (unlikely(ret)) {
        sp_write_failed(mapping, pos + len);
	}
	printk("spfs: sp_write_begin got page=%p\n", *pagep);
    return ret;
}

static int
sp_write_end(struct file *file, struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata)
{
	struct inode	*inode	= mapping->host;
    int				error;

	printk("spfs: sp_write_end for inode=%p, off=%lld, len=%d, page=%p\n",
		   inode, pos, len, page);
	error = generic_write_end(file, mapping, pos, len, copied, page, fsdata);
    return error;
}

int
sp_writepage(struct page *page, struct writeback_control *wbc)
{
	printk("spfs: sp_writepage for page=%p\n", page);
	return block_write_full_page(page, sp_get_block, wbc);
}

static int
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
	.writepage			= sp_writepage,
	.write_begin		= sp_write_begin,
	.write_end			= sp_write_end,
	.bmap				= sp_bmap
};

struct inode_operations sp_file_inops = {
	.link	= sp_link,
	.unlink	= sp_unlink,
};
