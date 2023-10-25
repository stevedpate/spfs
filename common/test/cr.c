#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

char *buf = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque dignissim nisl quis arcu posuere semper. Cras dolor enim, mollis ut elit fringilla, dignissim tempus lorem. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Mauris et ligula orci. Donec gravida sed nibh vitae pulvinar. Pellentesque ultrices eros leo, vitae pellentesque lorem suscipit ac. Curabitur posuere, est ac laoreet varius, lectus mauris tristique dui, non sodales turpis tortor vel diam. Etiam vehicula a augue eu iaculis. Quisque a egestas nulla. Nullam a semper orci. Nulla congue turpis vitae massa accumsan sollicitudin. Maecenas sodales dolor ut nibh efficitur, in congue ligula pulvinar. Etiam sollicitudin a sem a porta.\n\nAenean porta sollicitudin mauris, non eleifend lacus tincidunt nec. Ut quis mi nibh. Aenean lobortis ornare nisi at congue. Sed id augue suscipit quam luctus aliquet. Quisque sollicitudin, ipsum ac hendrerit pulvinar, lectus dolor laoreet sapien, vitae ullamcorper metus neque sed arcu. Suspendisse sed tristique nibh. Phasellus vitae efficitur purus, eu dapibus lorem. Maecenas quam orci, tristique quis malesuada eu, pellentesque at mi.\n\nQuisque in elit eu enim finibus dictum. Suspendisse porttitor sapien porta, pellentesque est sed, blandit sapien. Sed luctus nisl sit amet aliquet aliquam. Nulla vel est sit amet orci vulputate fringilla quis ut nibh. Morbi condimentum vulputate enim vel molestie. Fusce maximus facilisis sem, quis pharetra ligula vestibulum quis. Praesent et erat diam. Donec luctus tortor eget condimentum auctor.\n\nNulla facilisi. Nulla vel tortor fringilla lectus congue porttitor. Nunc pellentesque ex eget lacus mattis, vitae ultricies ipsum rhoncus. Sed semper odio felis, et elementum mauris accumsan non. Quisque dapibus urna id dolor molestie tempor. Proin ullamcorper erat massa, at faucibus arcu sollicitudin a. Donec molestie sollicitudin lorem vel pellentesque. Cras ut enim ipsum. Donec massa turpis, iaculis vulputate imperdiet vel, sollicitudin sit amet leo. Praesent at tellus porttitor, convallis nunc sed, tristique magna. Aenean tempor in diam ut facilisis. Donec nec porttitor nisl. Duis sagittis odio sed mauris pulvinar lobortis. Pellentesque aliquet lectus a tempus sodales. Nullam venenatis justo sed nibh ornare, in dictum erat vestibulum.\n\nIn consequat tincidunt dui eu ultrices. Maecenas nibh odio, malesuada vitae consectetur laoreet, pharetra nec massa. Duis at ultricies ipsum. Nam non convallis tortor. Sed sed fermentum magna. Nullam et nisl a metus suscipit dapibus eget in urna. Etiam ullamcorper urna eget blandit fringilla. Vivamus vitae efficitur sapien. Ut libero nunc, eleifend eget velit eget, fermentum sodales velit. Suspendisse pellentesque, purus et condimentum finibus, dolor felis porttitor lacus, a pulvinar ipsum purus vel lectus. Duis id magna eu leo pulvinar porta sed eget nibh. Praesent turpis eros, dapibus sed leo id, facilisis pulvinar nisi.\n";

int
main()
{
	int error;
	int fd = open("/mnt/myfile", O_CREAT|O_WRONLY, 0644);
	if (fd < 0) {
		printf("write failed - %d\n", errno);
	} else {
		error = write(fd, buf, strlen(buf));
		printf("size of Lorem = %d\n", (int)strlen(buf));
	}
}
