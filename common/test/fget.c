#include <stdio.h>

int
main()
{
	FILE *fp;
	int			c;

	fp = fopen("lorem-ipsum", "r+");
	c = fgetc(fp); 

	printf("_IO_read_base = %p\n", fp->_IO_buf_base);
	printf("_IO_read_end  = %p (%d difference)\n\n", fp->_IO_read_end,
			fp->_IO_read_end - fp->_IO_buf_base);
	printf("_IO_read_ptr\t_IO_write_ptr\n");
	printf("------------------------------\n");
	printf("%p\t%p\n", fp->_IO_read_ptr, fp->_IO_write_ptr);

	c = fgetc(fp); 
	printf("%p\t%p\n", fp->_IO_read_ptr, fp->_IO_write_ptr);

	c = fgetc(fp);
	printf("%p\t%p\n", fp->_IO_read_ptr, fp->_IO_write_ptr);

	c = fputc('1', fp);
	printf("%p\t%p\n", fp->_IO_read_ptr, fp->_IO_write_ptr);

	c = fputc('2', fp);
	printf("%p\t%p\n", fp->_IO_read_ptr, fp->_IO_write_ptr);

	c = fputc('3', fp);
	printf("%p\t%p\n", fp->_IO_read_ptr, fp->_IO_write_ptr);

	c = fgetc(fp); 
	printf("%p\t%p\n", fp->_IO_read_ptr, fp->_IO_write_ptr);
	printf("Character read = %c\n", c);
}
