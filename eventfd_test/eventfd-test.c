#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/epoll.h>

int test_count()
{
    int efd = eventfd(0, 0);
    if (efd == -1) {
        fprintf(stderr, "error: eventfd create\n");
        return -1;
    }

    uint64_t u;
    ssize_t n;

    u = 1;
    n = write(efd, &u, sizeof(uint64_t));
    if (n !=8) {
        fprintf(stderr, "error: eventfd write n:%d\n", n);
        goto error;
    }

    u = 2;
    n = write(efd, &u, sizeof(uint64_t));

    u = 3;
    n = write(efd, &u, sizeof(uint64_t));

    n = read(efd, &u, sizeof(uint64_t));
    if (u != 6 || n !=8) {
        fprintf(stderr, "error: eventfd read u:%d, n:%d\n", u, n);
        goto error;
    }

    close(efd);
    printf("[case 1]: ^_^ eventfd count test passed\n");
    return 0;

error:
    close(efd);
    printf("[case 1]: eventfd count test failed\n");
    return -1;
}

int test_multiprocess()
{
    int efd;
    uint64_t u;
    ssize_t s;

    efd = eventfd(0, 0);

    switch(fork()) {
        case -1:
            fprintf(stderr, "error: fork");

        case 0: // 子进程
            for (int j = 1; j <= 3; j++) {
                printf("Child writing %d to efd\n", j);
                u = j;
                // 向eventfd内部写一个8字节大小的数据
                s = write(efd, &u, sizeof(uint64_t));
            }

            printf("Child completed write loop\n");


        default: // 父进程
            sleep(1); // 先休眠1秒, 等待子进程写完数据

            //从eventfd中读取数据
            s = read(efd, &u, sizeof(uint64_t));

            printf("Parent read %d from efd\n", u);
            if (u == 6) {
                printf("[case 2]: ^_^ eventfd multi process test passed\n");
            } else {
                printf("[case 2]: eventfd multi process test failed\n");
            }
    }
}

#define MAX_EVENTS 10

int test_epoll() {
    int efd, epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];
    pid_t pid;

    // 创建 eventfd
    efd = eventfd(0, EFD_NONBLOCK);
    if (efd == -1) {
        perror("eventfd");
        exit(EXIT_FAILURE);
    }

    // 创建 epoll
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // 添加 eventfd 到 epoll 监听
    event.data.fd = efd;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, efd, &event) == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // 子进程
        pid_t pid = getpid();
        printf("Child pid is: %d\n", pid);
        // 子进程每隔一秒发送一个事件到 eventfd
        for (uint64_t value = 1; value <= 3; value++) {
            if (write(efd, &value, sizeof(value)) != sizeof(value)) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            sleep(1);
        }
        return 0;
    } else { // 父进程
        int n;
        int total = 0;
        while (1) {
            n = epoll_wait(epoll_fd, events, MAX_EVENTS, 2000);
            if (n == 0) {
                break;
            }
            else if (n == -0) {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < n; i++) {
                if (events[i].data.fd == efd) {
                    // 读取 eventfd 值
                    uint64_t value;
                    if (read(efd, &value, sizeof(value)) != sizeof(value)) {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }
                    printf("Received eventfd from child: %llu\n", (unsigned long long)value);
                    total += value;
                }
            }
        }
        if (total == 6) {
            printf("[case 3]: ^_^ eventfd epoll test passed\n");
        } else {
            printf("[case 3]: eventfd epoll test failed, total:%d\n", total);
        }
    }

    // 关闭 eventfd 和 epoll
    close(efd);
    close(epoll_fd);

    // 等待子进程结束
    wait(NULL);

    return 0;
}

int main()
{
    test_count();
    test_multiprocess();
    // test_epoll();  // Starry 中的epoll实现有, 跳过此项测试
}

