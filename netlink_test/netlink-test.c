//
// 执行面的命令, 可以触发内核发送 netlink 消息
// ip link add vnet0 type veth
// ip link delete vnet0
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define NETLINK_BUF_SIZE 8192

int open_netlink_socket() {
    int sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock_fd < 0) {
        perror("socket failed");
        return -1;
    }
    return sock_fd;
}

int join_netlink_group(int sock_fd, int group_id) {
    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));

    addr.nl_family = AF_NETLINK;
    addr.nl_groups = group_id; // RTNLGRP_LINK

    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(sock_fd);
        return -1;
    }
    return 0;
}

void print_message_type(struct nlmsghdr *nlh) {
    switch (nlh->nlmsg_type) {
        case RTM_NEWLINK:
            printf("recv RTM_NEWLINK\n");
            break;
        case RTM_DELLINK:
            printf("recv RTM_DELLINK\n");
            break;
        default:
            printf("other message type: %d\n", nlh->nlmsg_type);
            break;
    }
}

// 循环接收和处理Netlink消息
void receive_netlink_messages(int sock_fd) {
    ssize_t read_len;
    char buffer[NETLINK_BUF_SIZE];

    printf("Ready to recv netlink msg ...\n");
    while (1) {
        read_len = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (read_len < 0) {
            perror("read failed");
            continue;
        }

        struct nlmsghdr *nlh = (struct nlmsghdr *)buffer;
        while (NLMSG_OK(nlh, read_len)) {
            print_message_type(nlh);

            nlh = NLMSG_NEXT(nlh, read_len);
        }

        sleep(1);
    }
}

int main(void) {
    int sock_fd;

    sock_fd = open_netlink_socket();
    if (sock_fd < 0) {
        return -1;
    }

    if (join_netlink_group(sock_fd, RTNLGRP_LINK) < 0) {
        return -1;
    }

    receive_netlink_messages(sock_fd);

    close(sock_fd);
    return 0;
}
