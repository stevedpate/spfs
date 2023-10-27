// SPDX-License-Identifier: GPL-2.0

/* 
 * sp_inode.c - mostly inode handling as well as module load/unload and
 *              mount handling code.
 *
 * Copyright (c) 2023 Steve D. Pate
 */

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/buffer_head.h>
#include <linux/vfs.h>
#include <linux/uio.h>
#include <linux/blkdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/writeback.h>
#include <uapi/linux/mount.h>
#include "spfs.h"

MODULE_AUTHOR("Steve Pate <spate@me.com>");
MODULE_DESCRIPTION("A simple Linux filesystem for teaching");
MODULE_LICENSE("GPL");

static struct kmem_cache *spfs_inode_cache;

/*
 * This function looks for "name" in the directory "dip". 
 * If found the inode number is returned.
 */

int
sp_find_entry(struct inode *dip, char *name)
{
    struct sp_inode_info    *spi = ITOSPI(dip);
    struct super_block      *sb = dip->i_sb;
    struct buffer_head      *bh;
    struct sp_dirent        *dirent;
    int                     i, blk = 0;

    printk("spfs: sp_find_entry - looking for %s (dip = %p)\n", name, dip);
    for (blk=0 ; blk < spi->i_blocks ; blk++) {
        bh = sb_bread(sb, spi->i_addr[blk]);
        dirent = (struct sp_dirent *)bh->b_data;
        for (i=0 ; i < SP_DIRS_PER_BLOCK ; i++) {
            if (strcmp(dirent->d_name, name) == 0) {
                brelse(bh);
                printk("spfs: sp_find_entry - found inum %d for %s\n",
                       dirent->d_ino, name);
                return dirent->d_ino;
            }
            dirent++;
        }
    }
    brelse(bh);
    printk("spfs: sp_find_entry - failed for %s\n", name);
    return 0;
}

/*
 * Called internally by sp_fill_super() and also from sp_lookup()
 * when reading a file that's not already in-core.
 */

struct inode *
sp_read_inode(struct super_block *sb, unsigned long ino)
{
    struct buffer_head        *bh, *symbh;
    struct sp_inode           *disk_ip;
    struct sp_inode_info      *spi;
    struct inode              *inode;
    int                       i, block;

    printk("spfs: sp_read_inode for ino=%d\n", (int)ino);
    inode = iget_locked(sb, ino);
    if (!inode) {
        return ERR_PTR(-ENOMEM);
    }
    if (!(inode->i_state & I_NEW)) {
        return inode;
    }

    /*
     * Note that for simplicity, there is only one inode per block!
     */

    block = SP_INODE_BLOCK + ino;
    bh = sb_bread(sb, block);
    if (!bh) {
            printk("spfs: sp_read_inode - Unable to read inode %lu\n", ino);
            goto out;
    }
    disk_ip = (struct sp_inode *)bh->b_data;
    spi = ITOSPI(inode);

    inode->i_mode = le32_to_cpu(disk_ip->i_mode);
    if (S_ISDIR(disk_ip->i_mode)) {
        inode->i_op = &sp_dir_inops;
        inode->i_fop = &sp_dir_operations;
    } else if (S_ISREG(disk_ip->i_mode)) {
        inode->i_op = &sp_file_inops;
        inode->i_fop = &sp_file_operations;
        inode->i_mapping->a_ops = &sp_aops;
    } else if (S_ISLNK(disk_ip->i_mode)) {
        inode->i_op = &sp_symlink_operations;
        inode_nohighmem(inode);     /* XXX does what? See inode.c: BUG_ON */
        inode->i_mapping->a_ops = &sp_aops;
        printk("LNK - reading link from block %d\n", disk_ip->i_addr[0]);
        symbh = sb_bread(sb, disk_ip->i_addr[0]); /* XXX error */
        memcpy(spi->i_symlink, (char *)symbh->b_data, disk_ip->i_size);
        spi->i_symlink[disk_ip->i_size] = '\0';
        inode->i_link = spi->i_symlink;
        printk("LNK - read link =  %s\n", inode->i_link);
        brelse(symbh);
    } else {
        printk("spfs: read_inode - can't get here\n"); /* XXX */
    }
    i_uid_write(inode, (uid_t)disk_ip->i_uid);
    i_gid_write(inode, (uid_t)disk_ip->i_gid);
    set_nlink(inode, le32_to_cpu(disk_ip->i_nlink));
    inode->i_size = le32_to_cpu(disk_ip->i_size);
    inode->i_blocks = disk_ip->i_blocks;
    inode->i_atime.tv_sec = disk_ip->i_atime;
    inode->i_mtime.tv_sec = disk_ip->i_mtime;
    inode->i_ctime.tv_sec = disk_ip->i_ctime;
    inode->i_atime.tv_nsec = 0;
    inode->i_mtime.tv_nsec = 0;
    inode->i_ctime.tv_nsec = 0;
    inode->i_private = spi;
    printk("*** set i_private to 0x%px\n", spi);
    printk("*** addr of inode = 0x%px\n", inode);

    for (i=0 ; i < SP_DIRECT_BLOCKS ; i++) {
        spi->i_addr[i] = disk_ip->i_addr[i];
    }
    spi->i_blocks = disk_ip->i_blocks;

    brelse(bh);
    unlock_new_inode(inode);
    printk("spfs: sp_read_inode gets inode = %p\n", inode);
    return inode;

out:
    iget_failed(inode);
    return ERR_PTR(-EIO);
}

/*
 * This function is called to write a dirty inode to disk. We read
 * the original (XXX not needed), copy all the in-core fields and
 * mark the buffer dirty so it gets flushed.
 * 
 * XXX look at what wbc does
 */

int
sp_write_inode(struct inode *inode, struct writeback_control *wbc)
{
    unsigned long           ino = inode->i_ino;
    struct sp_inode_info    *spi = ITOSPI(inode);
    struct sp_inode         *dip;
    struct buffer_head      *bh;
    __u32                   blk;
    int                     i, error = 0;

    printk("spfs: sp_write_inode (ino=%ld)\n", inode->i_ino);
    blk = SP_INODE_BLOCK + ino;
    bh = sb_bread(inode->i_sb, blk); /* XXX check return */
    dip = (struct sp_inode *)bh->b_data;
    dip->i_mode = cpu_to_le32(inode->i_mode);
    dip->i_nlink = cpu_to_le32(inode->i_nlink);
    dip->i_atime = cpu_to_le32(inode->i_atime.tv_sec);
    dip->i_mtime = cpu_to_le32(inode->i_mtime.tv_sec);
    dip->i_ctime = cpu_to_le32(inode->i_ctime.tv_sec);
    dip->i_uid = cpu_to_le32(i_uid_read(inode));
    dip->i_gid = cpu_to_le32(i_gid_read(inode));
    dip->i_size = cpu_to_le32(inode->i_size);
    dip->i_nlink = cpu_to_le32(inode->i_nlink);
    dip->i_blocks = spi->i_blocks;
    for (i=0 ; i<SP_DIRECT_BLOCKS ; i++) {
        dip->i_addr[i] = cpu_to_le32(spi->i_addr[i]);
    }
    mark_buffer_dirty(bh);
    if (wbc->sync_mode == WB_SYNC_ALL) {
        sync_dirty_buffer(bh);
        if (buffer_req(bh) && !buffer_uptodate(bh)) {
            error = -EIO;
        }
    }
    brelse(bh);
    return(error);
}

static void
sp_free_inode(struct inode *inode)
{
    printk("spfs: sp_free_inode (ino=%ld)\n", inode->i_ino);
    kmem_cache_free(spfs_inode_cache, SPFS_I(inode));
}

/*
 * XXX - see https://www.spinics.net/lists/linux-fsdevel/msg90326.html
 *
 * is sp_free_inode called afterwards?
 */

static void
sp_evict_inode(struct inode *inode)
{
    struct sp_inode_info    *spi = ITOSPI(inode);
    struct super_block      *sb = inode->i_sb;
    struct spfs_sb_info     *sbi = SBTOSPFSSB(sb);
    int                     blkpos, i;

    truncate_inode_pages_final(&inode->i_data);
    invalidate_inode_buffers(inode);
    clear_inode(inode);
    if (inode->i_nlink) {   /* XXX sysvfs different to BFS */
        printk("spfs: sp_evict_inode - nlink > 0 (ino=%ld)\n", inode->i_ino);
        return;
    }

    printk("spfs: sp_evict_inode - nlink = 0 (ino=%ld)\n", inode->i_ino);
    mutex_lock(&sbi->s_lock);
    sbi->s_nifree++;
    sbi->s_inode[inode->i_ino] = SP_INODE_FREE;
    sbi->s_nbfree += spi->i_blocks;
    for (i=0 ; i < spi->i_blocks ; i++) {
        blkpos = spi->i_addr[i] - SP_FIRST_DATA_BLOCK;
        sbi->s_block[blkpos] = SP_BLOCK_FREE;
    }
    mutex_unlock(&sbi->s_lock);
    printk("spfs: sp_evict_inode - end of function\n");
}

/*
 * This function is called when the filesystem is being 
 * unmounted. We read in the disk superblock, update it with
 * fields from the in-core superblock, write it back to disk
 * and free the incore superblock.
 */

void
sp_put_super(struct super_block *sb)
{
    struct spfs_sb_info     *sbi = SBTOSPFSSB(sb);
    struct sp_superblock    *dsb;
    struct buffer_head      *bh;
    int                     i;

    printk("spfs: sp_put_super\n");
    bh = sb_bread(sb, 0);
    if (!bh) {
        printk("spfs: sp_put_super - failed to read superblock\n");
        return;
    }
    dsb = (struct sp_superblock *)bh->b_data;
    dsb->s_mod = SP_FSCLEAN;
    dsb->s_nifree = sbi->s_nifree;
    dsb->s_nbfree = sbi->s_nbfree;
    for (i=0 ; i<SP_MAXFILES ; i++) {
        dsb->s_inode[i] = cpu_to_le32(sbi->s_inode[i]);
    }
    for (i=0 ; i<SP_MAXBLOCKS ; i++) {
        dsb->s_block[i] = cpu_to_le16(sbi->s_block[i]);
    }
    mutex_destroy(&sbi->s_lock);
    kfree(sbi);
    mark_buffer_dirty(bh);
    brelse(bh);
}

/*
 * This function is called by the df command.
 */

int
sp_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    struct super_block   *sb = dentry->d_sb;
    struct spfs_sb_info  *sbi = SBTOSPFSSB(sb);

    buf->f_type = SP_MAGIC;
    buf->f_bsize = SP_BSIZE;
    buf->f_blocks = SP_MAXBLOCKS;
    buf->f_bfree = sbi->s_nbfree;
    buf->f_bavail = sbi->s_nbfree;
    buf->f_files = SP_MAXFILES;
    buf->f_ffree = sbi->s_nifree;
    buf->f_fsid = u64_to_fsid(huge_encode_dev(sb->s_bdev->bd_dev));
    buf->f_namelen = SP_NAMELEN;
    return 0;
}

static struct inode *
sp_alloc_inode(struct super_block *sb)
{
    struct sp_inode_info    *si;

    si = alloc_inode_sb(sb, spfs_inode_cache, GFP_KERNEL);
    if (!si) {
        return NULL;
    }
    return &si->vfs_inode;
}

struct super_operations spfs_sops = {
    .alloc_inode    = sp_alloc_inode,
    .free_inode     = sp_free_inode,
    .write_inode    = sp_write_inode,
    .evict_inode    = sp_evict_inode,
    .put_super      = sp_put_super,
    .statfs         = sp_statfs,
};

/*
 * Called from spfs_mount -> mount_bdev(..., spfs_fill_super)
 *
 * We read in the superblock and the root inode and initialize everything 
 * needed.
 *
 * XXX mutex_destroy(&info->bfs_lock);  - do we need such a lock?
 */

static int
spfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct sp_superblock    *spfs_sb;
    struct spfs_sb_info     *spfs_info;
    struct buffer_head      *bh;
    struct inode            *root_inode;
    int                     i, error;

    printk("spfs: spfs_fill_super entered\n");

    /*
     * Set up our internal per-FS superblock, reference it from the
     * kernel super_block and initialize relevant fields.
     */

    spfs_info = kzalloc(sizeof(struct spfs_sb_info), GFP_KERNEL);
    if (!spfs_info) {
        return -ENOMEM;
    }

    mutex_init(&spfs_info->s_lock);
    sb->s_fs_info = spfs_info;
    sb_set_blocksize(sb, SP_BSIZE);
    sb->s_time_min = 0;
    sb->s_time_max = U32_MAX;

    /*
     * Read in block 0 which should contain our superblock. Check
     * to make sure it's got the right magic number and is clean.
     * We don't yet handle dirty filesystems.
     */

    bh = sb_bread(sb, 0);
    if (!bh) {
        goto out;
    }
    spfs_sb = (struct sp_superblock *)bh->b_data;
    if (spfs_sb->s_magic != SP_MAGIC) {
        if (!silent) {
            printk("spfs: Unable to find spfs filesystem\n");
        }
        goto out1;
    }
    if (spfs_sb->s_mod == SP_FSDIRTY) {
        printk("spfs: Filesystem is not clean. Write and run SPFS fsck!\n");
        goto out1;
    }

    /*
     *  XXX - We should mark the superblock to be dirty and write it to disk.
     */

    sb->s_fs_info = spfs_info;
    sb->s_magic = SP_MAGIC;
    sb->s_op = &spfs_sops;

    spfs_info->s_nifree = le32_to_cpu(spfs_sb->s_nifree);
    spfs_info->s_nbfree = le32_to_cpu(spfs_sb->s_nbfree);


    for (i=0 ; i<SP_MAXFILES ; i++) {
        spfs_info->s_inode[i] = le32_to_cpu(spfs_sb->s_inode[i]);
    }
    for (i=0 ; i<SP_MAXBLOCKS ; i++) {
        spfs_info->s_block[i] = le16_to_cpu(spfs_sb->s_block[i]);
    }

    /*
     * All superblock handling is done so let's read the root inode.
     */

    root_inode = sp_read_inode(sb, SP_ROOT_INO);
    if (!root_inode) {
        goto out1;
    }
    printk("spfs: sp_fill_super - root_inode = %p\n", root_inode);
    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        error = -ENOMEM;
        goto out2;
    }
    return 0;

out2:       
    dput(sb->s_root);
    sb->s_root = NULL;
out1:
    brelse(bh);
out:
    mutex_destroy(&spfs_info->s_lock);
    kfree(spfs_info);
    sb->s_fs_info = NULL;
    return error;
}

struct dentry *
spfs_mount(struct file_system_type *fs_type, int flags,
           const char *dev_name, void *data)
{
    printk("spfs: spfs_mount\n");
    return mount_bdev(fs_type, flags, dev_name, data, spfs_fill_super);
}

static void
init_once(void *ptr)
{   
    struct sp_inode_info *spi = ptr;

    inode_init_once(&spi->vfs_inode);
}

static int __init
sp_init_inodecache(void)
{   
    spfs_inode_cache = kmem_cache_create("spfs_inode_cache",
                         sizeof(struct sp_inode_info), 0, (SLAB_RECLAIM_ACCOUNT|
                         SLAB_MEM_SPREAD|SLAB_ACCOUNT), init_once);
    if (spfs_inode_cache == NULL) {
        return -ENOMEM;
    }
    return 0;
}   

static void
sp_destroy_inodecache(void)
{
    /*
     * Make sure all delayed rcu free inodes are flushed before we
     * destroy cache. XXX - change wording (BFS). 
     * Why would this be true? You can't unload a module which still
     * has mounted filesystems can you?
     */

    rcu_barrier();
    kmem_cache_destroy(spfs_inode_cache);
}

static struct file_system_type spfs_fs_type = {
    .owner      = THIS_MODULE,
    .name       = "spfs",
    .mount      = spfs_mount,
    .kill_sb    = kill_block_super,
    .fs_flags   = FS_REQUIRES_DEV,
};

MODULE_ALIAS_FS("spfs");

static int __init
init_spfs_fs(void)
{
    int error = sp_init_inodecache(); 
    printk("spfs: init_spfs_fs\n");
    if (error) {
        goto out1;
    }
    error = register_filesystem(&spfs_fs_type);
    if (error) {
        goto out;
    }
    return 0;
out:
    sp_destroy_inodecache();
out1:
    return error;
}

static void __exit
exit_spfs_fs(void)
{
    printk("spfs: exit_spfs_fs\n");
    unregister_filesystem(&spfs_fs_type);
    sp_destroy_inodecache();
}

module_init(init_spfs_fs)
module_exit(exit_spfs_fs)
