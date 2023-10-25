#include <linux/fs.h>
#include "spfs.h"

long
sp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct inode			*inode = file->f_inode;
	struct sp_inode_info	*spi = ITOSPI(inode);
	struct spfs_sb_info		*sbi = SBTOSPFSSB(inode->i_sb);

	if (!capable(CAP_SYS_ADMIN)) {
        return -EPERM;
	}
	switch (cmd) {
		case SPFS_SB:
			printk("SPFS superblock at %p\n", sbi);
			printk("s_nifree = %lu\n", sbi->s_nifree);
			printk("s_nbfree = %lu\n", sbi->s_nbfree);
			break;
		case SPFS_INODE:
			printk("SPFS inode %p\n", spi);
			break;
		default:
			printk("spfs - invalid ioctl (%d)\n", cmd);
	}
	return 0;
}
