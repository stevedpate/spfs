// SPDX-License-Identifier: GPL-2.0

/*
 * sp_dir.c - directory specific operations (mkdir, rmdir, readdir, rename,
 *            file creation, symlinks, hard links, ...
 *
 * Copyright (c) 2023 Steve D. Pate
 */

#include <linux/sched.h>
#include <linux/string.h>
#include <linux/buffer_head.h>
#include <linux/time.h>
#include "../../../common/kern/spfs.h"

/*
 * Remove "name" from the directory "dip".
 */

int
sp_dirdel(struct inode *dip, char *name)
{
	struct sp_inode_info    *spi = (struct sp_inode_info *)&dip->i_private;
	struct buffer_head      *bh;
	struct super_block      *sb = dip->i_sb;
	struct sp_dirent        *dirent;
	__u32                   blk = 0;
	int                     i;

	printk("spfs: In sp_dirdel for %s\n", name);

	while (blk < spi->i_blocks) {
		bh = sb_bread(sb, spi->i_addr[blk]);
		blk++;
		dirent = (struct sp_dirent *)bh->b_data;
		for (i=0 ; i < SP_DIRS_PER_BLOCK ; i++) {
			if (strcmp(dirent->d_name, name) != 0) {
				dirent++;
				continue;
			} else {
				dirent->d_ino = 0;
				dirent->d_name[0] = '\0';
				mark_buffer_dirty(bh);
				break;
			}
		}
		brelse(bh);
	}
	return 0;
}

/*
 * Add file "name" to the directory "dip"
 */

int
sp_diradd(struct inode *dip, const char *name, int inum)
{
	struct sp_inode_info  *spi = ITOSPI(dip);
	struct buffer_head    *bh;
	struct super_block    *sb = dip->i_sb;
	struct sp_dirent      *dirent;
	int					  blk = 0;
	int                   error = 0, i, pos;

	printk("spfs: In sp_diradd for %s (inum = %d)\n", name, inum);

	for (blk=0 ; blk < spi->i_blocks ; blk++) {
		bh = sb_bread(sb, spi->i_addr[blk]);
		dirent = (struct sp_dirent *)bh->b_data;
		for (i=0 ; i < SP_DIRS_PER_BLOCK ; i++) {
			if (dirent->d_ino != 0) {
				dirent++;
				continue;
			} else {
				dirent->d_ino = inum;
				strcpy(dirent->d_name, name);
				dip->i_size += SP_DIRENT_SIZE;
				mark_buffer_dirty(bh);
				inode_inc_link_count(dip);
				brelse(bh);
				return 0;
			}
		}
		brelse(bh);
	}

	/*
	 * We didn't find an empty slot so need to allocate
	 * a new block if there's space in the inode.
	 */

	if (spi->i_blocks < SP_DIRECT_BLOCKS) {
		pos = spi->i_blocks;
		blk = sp_block_alloc(sb);
		spi->i_blocks++;
		dip->i_size += SP_DIRENT_SIZE;
		dip->i_blocks++;
		spi->i_addr[pos] = blk;
		bh = sb_bread(sb, blk);
		memset(bh->b_data, 0, SP_BSIZE);
		mark_inode_dirty(dip);
		dirent = (struct sp_dirent *)bh->b_data;
		dirent->d_ino = inum;
		strcpy(dirent->d_name, name);
		mark_buffer_dirty(bh);
		brelse(bh);
	} else {
		error = -ENOSPC;
	}
	return error;
}

/*
 * Rename file old_dentry/old_dir to new_dentry/new_dir
 */

int
sp_rename(struct user_namespace *mnt_userns, struct inode *old_dir,
          struct dentry *old_dentry, struct inode *new_dir,
          struct dentry *new_dentry, unsigned int flags)
{
    struct inode    *inode = d_inode(old_dentry);
    int             error;

    printk("spfs: In sp_rename %s -> %s\n",
            old_dentry->d_name.name, new_dentry->d_name.name);

    error = sp_diradd(new_dir, new_dentry->d_name.name,
                      inode->i_ino);
    if (error == 0) {
        sp_dirdel(old_dir, (char *)old_dentry->d_name.name);
    }
    return error;
}

int
sp_readdir(struct file *f, struct dir_context *ctx)
{
	struct inode			*dip = file_inode(f);
	struct sp_inode_info	*spi = ITOSPI(dip);
	struct sp_dirent		*de;
	struct buffer_head		*bh;
	unsigned int			offset;
	int						blk, disk_blk;

	/*printk("spfs: sp_readdir - dip = %p (spi = %p)\n", dip, spi);*/
	printk("spfs: sp_readdir - i_size = %d, ctx->pos = %d\n", (int)dip->i_size, (int)ctx->pos);

    while (ctx->pos < dip->i_size) {
		offset = ctx->pos % SP_BSIZE;
        blk = ctx->pos / SP_BSIZE;
		disk_blk = spi->i_addr[blk];
		printk("spfs: sp_readdir - blk = %d, disk_blk = %d\n", blk, disk_blk);

        bh = sb_bread(dip->i_sb, disk_blk);
        if (!bh) {
            ctx->pos += SP_BSIZE - offset;
            continue;
        }
        do {
            de = (struct sp_dirent *)(bh->b_data + offset);
            if (de->d_ino) {
                int size = strnlen(de->d_name, SP_NAMELEN);
				printk("spfs: sp_readdir - return %s\n", de->d_name);
                if (!dir_emit(ctx, de->d_name, size,
                        	  le16_to_cpu(de->d_ino), DT_UNKNOWN)) {
                    brelse(bh);
                    return 0;
                }
            }
            offset += SP_DIRENT_SIZE;
            ctx->pos += SP_DIRENT_SIZE;
        } while ((offset < SP_BSIZE) && (ctx->pos < dip->i_size));
        brelse(bh);
    }
	return 0;
}

struct file_operations sp_dir_operations = {
	.read			= generic_read_dir, /* XXX - doesn't do anything! */
	.iterate_shared	= sp_readdir,
	.fsync			= generic_file_fsync,
};

struct inode *
sp_new_inode(struct inode *dip, struct dentry *dentry, umode_t mode,
		     const char *symlink_target)
{
	struct super_block		*sb = dip->i_sb;
	struct buffer_head		*bh;
	struct sp_dirent		*dirent;
    struct inode			*inode;
    struct sp_inode_info	*spi;
    int						error, inum, blk, slen;
	char					*name = (char *)dentry->d_name.name;

	printk("spfs: sp_new_inode for %s - mode=%o\n", name, mode);
	inode = new_inode(sb);
	if (!inode) {
		return ERR_PTR(-ENOMEM);
	}
	inum = sp_ialloc(sb);
	if (!inum) {
		iput(inode);
		return ERR_PTR(-ENOSPC);
	}
	inode_init_owner(&init_user_ns, inode, dip, mode);
	inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
	inode->i_ino = inum;
	insert_inode_hash(inode);
	spi = ITOSPI(inode);
    inode->i_private = spi;
    spi->i_fs[0] = 'S';
    spi->i_fs[1] = 'P';
    spi->i_fs[2] = 'F';
    spi->i_fs[3] = 'S';
	memset(spi->i_addr, 0, SP_DIRECT_BLOCKS); /* XXX 247 0s? 32-bit? */

	if (S_ISREG(mode)) {
		inode->i_blocks = 0;
		inode->i_op = &sp_file_inops;
		inode->i_fop = &sp_file_operations;
		inode->i_mapping->a_ops = &sp_aops;
		inode->i_size = 0;
		spi->i_blocks = 0;
	} else if (S_ISDIR(mode)) {
		inode->i_blocks = 1;
		inode->i_op = &sp_dir_inops;
		inode->i_fop = &sp_dir_operations;
		inode->i_mapping->a_ops = &sp_aops;
		inode->i_size = 2 * SP_DIRENT_SIZE;

		spi->i_blocks = 1;
		blk = sp_block_alloc(sb);	/* XXX failure? */
		spi->i_addr[0] = blk;
		bh = sb_bread(sb, blk);
		if (!bh) {
			/* XXX - need to handle failure */
			return 0;
		}
		memset(bh->b_data, 0, SP_BSIZE);
		dirent = (struct sp_dirent *)bh->b_data;
		dirent->d_ino = inum;
		strcpy(dirent->d_name, ".");
		dirent++;
		dirent->d_ino = dip->i_ino;
		strcpy(dirent->d_name, "..");

		mark_buffer_dirty(bh);
		brelse(bh);
	} else { /* S_IFLNK  - size set to 0 as kernel will write symlink */
		slen = strlen(symlink_target) + 1;
		inode->i_blocks = 0;
		inode->i_size = 0;
		inode->i_op = &sp_symlink_operations;
		inode_nohighmem(inode);  /* XXX does what? */
		inode->i_mapping->a_ops = &sp_aops;

		/*
		 * XXX - calls write_begin/end, creates page with link and sets i_size
		 */

		error = page_symlink(inode, symlink_target, slen);
	}

	/*
	 * Finish off things ...
	 */

	mark_inode_dirty(inode);
	mark_inode_dirty(dip);
	sp_diradd(dip, name, inum);		/* XXX check errors */
	d_instantiate(dentry, inode);

	return inode;
}

/*
 * When we reach this point, sp_lookup() has already been called
 * to create a negative entry in the dcache. Thus, we need to
 * allocate a new inode on disk and associate it with the dentry.
 */

int
sp_create(struct user_namespace *mnt_userns, struct inode *dip,
          struct dentry *dentry, umode_t mode, bool excl)
{
	struct inode	*inode;
	char			*name = (char *)dentry->d_name.name;
	int				error;

	/*printk("spfs: In sp_create for %s\n", name);*/
	inode = sp_new_inode(dip, dentry, S_IFREG | mode, NULL);
	if (IS_ERR(inode)) {
		error = PTR_ERR(inode);
        goto out;
	}
	printk("spfs: sp_create - inode %p created for %s\n", inode, name);
out:
	return error;
}

/*
 * Make a new directory. We already have a negative dentry
 * so must create the directory and instantiate it.
 */

int
sp_mkdir(struct user_namespace *mnt_userns, struct inode *dip,
         struct dentry *dentry, umode_t mode)
{
	struct inode            *inode;
	char					*name = (char *)dentry->d_name.name;
	int                     error;

	printk("spfs: sp_mkdir for %s\n", name);
	inode = sp_new_inode(dip, dentry, S_IFDIR | mode, NULL);
	if (IS_ERR(inode)) {
		error = PTR_ERR(inode);
        goto out;
	}
	printk("spfs: sp_mkdir - inode %p created for %s\n", inode, name);

out:
	return error;
}

/*
 * Remove the specified directory.
 */

int
sp_rmdir(struct inode *dip, struct dentry *dentry)
{
	struct super_block     *sb = dip->i_sb;
	struct spfs_sb_info    *sbi = SBTOSPFSSB(sb);
	struct inode           *inode = dentry->d_inode;
	struct sp_inode_info   *spi = ITOSPI(inode);
	int                    inum, i;

	printk("spfs: sp_rmdir for %s\n", (char *)dentry->d_name.name);

	if (inode->i_nlink > 2) {
			return -ENOTEMPTY;
	}

	/*
	 * Remove the entry from the parent directory
	 */

	inum = sp_find_entry(dip, (char *)dentry->d_name.name);
	if (!inum) {
			return -ENOTDIR;
	}
	sp_dirdel(dip, (char *)dentry->d_name.name);

	/*
	 * Clean up the inode
	 */

	mutex_lock(&sbi->s_lock);
	for (i=0 ; i<SP_DIRECT_BLOCKS ; i++) {
			if (spi->i_addr[i] != 0) {
					sbi->s_block[spi->i_addr[i]] = SP_BLOCK_FREE;
					sbi->s_nbfree++;
			}
	}

	/*
	 * Update the superblock summaries.
     * XXX - need to update free block count
	 */

	sbi->s_inode[inode->i_ino] = SP_INODE_FREE;
	sbi->s_nifree++;
	mutex_unlock(&sbi->s_lock);

	dip->i_ctime = dip->i_mtime = current_time(dip);
	mark_inode_dirty(dip);

	inode->i_ctime = dip->i_ctime;
	inode_dec_link_count(inode);

	return 0;
}

/*
 * Lookup the specified file. If found, we'll bring it into core
 * otherwise we instantiate a negative dentry.
 *
 * XXX - there is a comment in dcache.c (__d_move) that "negative
 * dcache entries should not be moved in this way"
 */

struct dentry *
sp_lookup(struct inode *dip, struct dentry *dentry, unsigned int flags)
{
	struct inode        *inode = NULL;
	char				*name = (char *)dentry->d_name.name;
	int                 inum;

	printk("spfs: sp_lookup - for %s\n", name);
	if (dentry->d_name.len > SP_NAMELEN) {
		return ERR_PTR(-ENAMETOOLONG);
	}

	inum = sp_find_entry(dip, name);
	if (inum) {
		inode = sp_read_inode(dip->i_sb, inum); /* XXX errror */
		printk("spfs: sp_lookup name = %s, inode = %p (ino=%d)\n",
		       name, inode, inum);
	} else {
		printk("spfs: sp_lookup - %s not found\n", name);
	}
	return d_splice_alias(inode, dentry);
}

int
sp_getattr(struct user_namespace *mnt_userns, const struct path *path,
           struct kstat *stat, u32 request_mask, unsigned int flags)
{
    generic_fillattr(&init_user_ns, d_inode(path->dentry), stat);
    stat->blocks = SP_MAXBLOCKS; /* XXX */
    stat->blksize = SP_BSIZE;
    return 0;
}

static int
sp_readlink(struct dentry *dentry, char __user *buffer, int buflen)
{
	struct inode			*inode = d_inode(dentry);
	struct sp_inode_info	*spi = ITOSPI(inode);

	printk("spfs: sp_page_get_link for %s\n", (char *)dentry->d_name.name);
	return copy_to_user(buffer, spi->i_symlink, inode->i_size);
}

const char *sp_page_get_link(struct dentry *dentry, struct inode *inode,
			  struct delayed_call *callback)
{
	printk("spfs: sp_page_get_link for %s\n", (char *)dentry->d_name.name);
	return page_get_link(dentry, inode, callback);
}

static const struct inode_operations sp_symlink_operations = {
	/* XXX .get_link   = page_get_link,*/
	.readlink   = sp_readlink,
	.get_link   = sp_page_get_link,
    .getattr    = sp_getattr,
};

/*
 * Called in response to an "ln -s" command/syscall to create a 
 * symbolic link.
 */

int
sp_symlink(struct user_namespace *mnt_userns, struct inode *dip,
		   struct dentry *dentry, const char *target)

{
	struct inode			*inode;
	char					*name = (char *)dentry->d_name.name;
	int						error = 0;

	printk("spfs: In sp_symlink - new file = %s -> %s\n", name, target);

	inode = sp_new_inode(dip, dentry, S_IFLNK | S_IRWXUGO, target);
	if (IS_ERR(inode)) {
		error = PTR_ERR(inode);
        goto out;
	}
	printk("spfs: sp_symlink - inode %p created for %s\n", inode, name);
	printk("spfs: sp_symlink - i_size=%lld, i_blocks=%lld\n", inode->i_size, inode->i_blocks);
out:
	return error;
}

/*
 * Called in response to an "ln" command/syscall to create a hard
 * link to the file referenced via "old".
 */

int
sp_link(struct dentry *old, struct inode *dip, struct dentry *new)
{
	struct inode	*inode = d_inode(old);
	int				error;

	printk("spfs: In sp_link - new file = %s\n", new->d_name.name);

	/*
	 * Add the new file (new) to its parent directory (dip)
	 */

	error = sp_diradd(dip, new->d_name.name, inode->i_ino);

	/*
	 * Increment the link count of the target inode
	 */

	inc_nlink(inode);
	inode->i_ctime = current_time(inode);
	mark_inode_dirty(inode);
	ihold(inode);
	d_instantiate(new, inode);
	return 0;
}

/*
 * Called to remove a link to a file.
 */

int
sp_unlink(struct inode *dip, struct dentry *dentry)

{
	struct super_block		*sb = dip->i_sb;
	struct spfs_sb_info		*sbi = SBTOSPFSSB(sb);
	struct inode			*inode = dentry->d_inode;
	struct sp_inode_info	*spi = ITOSPI(inode);
	int						blk, i, error = -ENOENT;
	ino_t					inum = 0;

	inum = sp_find_entry(dip, (char *)dentry->d_name.name);
	if (!inum) {
		goto out;
	}
	printk("spfs: In sp_unlink for %s (inum = %ld)\n",
			(char *)dentry->d_name.name, inum);
	sp_dirdel(dip, (char *)dentry->d_name.name);

	dip->i_ctime = dip->i_mtime = current_time(dip);
	mark_inode_dirty(dip);

	inode->i_ctime = dip->i_ctime;
	inode_dec_link_count(inode);
	error = 0;

out:
	return error;
}

struct inode_operations sp_dir_inops = {
	.create		= sp_create,
	.lookup		= sp_lookup,
	.mkdir		= sp_mkdir,
	.rmdir		= sp_rmdir,
	.link		= sp_link,
	.unlink		= sp_unlink,
	.symlink	= sp_symlink,
	.rename  	= sp_rename
};
