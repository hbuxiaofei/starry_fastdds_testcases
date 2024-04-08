#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define MAX_PAYLOAD 1024

#define USE_IOV 0

struct sockaddr_nl dest_addr;
// struct sockaddr_nl src_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
    for (; RTA_OK(rta, len); rta = RTA_NEXT(rta, len)) {
        if (rta->rta_type <= max)
            tb[rta->rta_type] = rta;
    }
}

void print_link_info(struct nlmsghdr *nlhdr) {
    struct ifinfomsg *ifinfo;
    struct rtattr *tb[IFLA_MAX + 1] = { NULL };
    int len;

    ifinfo = (struct ifinfomsg *) NLMSG_DATA(nlhdr);
    len = nlhdr->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));

    parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifinfo), len);

    if (tb[IFLA_IFNAME])
        printf("\tInterface: %s\n", (char *) RTA_DATA(tb[IFLA_IFNAME]));
    if (tb[IFLA_MTU])
        printf("\tMTU: %d\n", *(int *) RTA_DATA(tb[IFLA_MTU]));
    if (tb[IFLA_ADDRESS]) {
        unsigned char *mac = RTA_DATA(tb[IFLA_ADDRESS]);
        printf("\tMAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
               mac[0],
               mac[1],
               mac[2],
               mac[3],
               mac[4],
               mac[5]);
    }

}

void read_response(int sock_fd) {
    // Allocate memory for receive buffer
    char buffer[4096];
    bool done = false;

    while(!done) {
#if USE_IOV
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);

        // Receive messages from kernel
        int read_bytes = recvmsg(sock_fd, &msg, 0);
#else
        int read_bytes = recvfrom(sock_fd, buffer, sizeof(buffer), 0, NULL, NULL);
#endif
        for (struct nlmsghdr *nlhdr = (struct nlmsghdr *)buffer; NLMSG_OK(nlhdr, read_bytes); nlhdr = NLMSG_NEXT(nlhdr, read_bytes)) {
            if (nlhdr->nlmsg_type == NLMSG_DONE) {
                done = true;
                break;
            } else if (nlhdr->nlmsg_type == RTM_NEWLINK) {
                printf("RTM_NEWLINK(%d) message\n", nlhdr->nlmsg_type);
            } else if (nlhdr->nlmsg_type == RTM_NEWADDR) {
                printf("RTM_NEWADDR(%d) message\n", nlhdr->nlmsg_type);
            } else {
                printf("other type(%d) message\n", nlhdr->nlmsg_type);
            }
            print_link_info(nlhdr);
        }
    }
}

void send_request(int sock_fd) {
    // Allocate memory for nlmsghdr
    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    /* nlh->nlmsg_pid = getpid(); */
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_type = RTM_GETLINK;
    /* nlh->nlmsg_seq = 1; */

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    /* dest_addr.nl_pid = 0; */

#if USE_IOV
   iov.iov_base = (void *)nlh;
   iov.iov_len = nlh->nlmsg_len;
   memset(&msg, 0, sizeof(msg));
   msg.msg_name = (void *)&dest_addr;
   msg.msg_namelen = sizeof(dest_addr);
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;

   // Send netlink message to kernel
   sendmsg(sock_fd, &msg, 0);
#else
    // 发送消息
    if (sendto(sock_fd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto");
        close(sock_fd);
        free(nlh);
        exit(EXIT_FAILURE);
    }
#endif
}



int main() {
    // NETLINK_ROUTE		0	/* Routing/device hook
    int sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock_fd < 0) {
        printf("Error in socket creation.\n");
        return -1;
    }

    // memset(&src_addr, 0, sizeof(src_addr));
    // src_addr.nl_family = AF_NETLINK;
    // src_addr.nl_pid = getpid();
    // src_addr.nl_groups = RTNLGRP_NONE;  // 0
    // bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));


    send_request(sock_fd);
    read_response(sock_fd);

    // Close the socket
    close(sock_fd);

    return 0;
}



