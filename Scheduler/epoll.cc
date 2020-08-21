#include "threads-model.h"
#include <sys/epoll.h>
#include <unistd.h>

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
	while(timeout != 0) {
		int res = real_epoll_wait(epfd, events, maxevents, timeout);
		if (res != 0)
			return res;
		usleep(1);
		if (timeout > 0)
			timeout--;
	}
	return 0;
}
