/*********************************************************************
 * ConfD Subscriber intro example
 * Implements a DHCP server adapter
 *
 * (C) 2005-2007 Tail-f Systems
 * Permission to use this code as a starting point hereby granted
 *
 * UPDATED BY E. Lavinal - University of Toulouse - 2017
 *
 * See the README file for more information
 ********************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>

#include <confd_lib.h>
#include <confd_cdb.h>

/* include generated file */
#include "dhcpd.h"

/********************************************************************/

static void do_subnet(int rsock, FILE *fp)
{
   struct in_addr ip;
   char buf[BUFSIZ];
   char *ptr;
   u_int32_t time_value;

   cdb_get_ipv4(rsock, &ip, "net");
   fprintf(fp, "subnet %s ", inet_ntoa(ip));
   cdb_get_ipv4(rsock, &ip, "mask");
   fprintf(fp, "netmask %s {\n", inet_ntoa(ip));

   if (cdb_exists(rsock, "range") == 1) {
       int bool;
       fprintf(fp, " range ");
       cdb_get_bool(rsock, &bool, "range/dynamicBootP");
       if (bool) fprintf(fp, " dynamic-bootp ");
       cdb_get_ipv4(rsock, &ip, "range/lowAddr");
       fprintf(fp, "%s ", inet_ntoa(ip));
       cdb_get_ipv4(rsock, &ip, "range/hiAddr");
       fprintf(fp, "%s;\n", inet_ntoa(ip));
   }

   if(cdb_get_str(rsock, &buf[0], BUFSIZ, "routers") == CONFD_OK) {
       /* replace space with comma */
       for (ptr = buf; *ptr != '\0'; ptr++) {
           if (*ptr == ' ')
               *ptr = ',';
       }
       fprintf(fp, " option routers %s;\n", buf);
   }

   cdb_get_u_int32(rsock, &time_value, "maxLeaseTime");
   fprintf(fp, " max-lease-time %d;\n", time_value);

   fprintf(fp, "}\n");
}

static int read_conf(struct sockaddr_in *addr)
{
    FILE *fp;
    int i, n, tmp;
    int rsock;
    u_int32_t time_value;

    if ((rsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open socket\n");

    if (cdb_connect(rsock, CDB_READ_SOCKET, (struct sockaddr*)addr,
                      sizeof (struct sockaddr_in)) < 0)
        return CONFD_ERR;
    if (cdb_start_session(rsock, CDB_RUNNING) != CONFD_OK)
        return CONFD_ERR;
    cdb_set_namespace(rsock, dhcpd__ns);

    if ((fp = fopen("dhcpd.conf.tmp", "w")) == NULL) {
        cdb_close(rsock);
        return CONFD_ERR;
    }

    // READ CONF FROM CDB
    // ==================

    cdb_get_u_int32(rsock, &time_value, "/dhcp/defaultLeaseTime");
    fprintf(fp, "default-lease-time %d;\n", time_value);

    cdb_get_u_int32(rsock, &time_value, "/dhcp/maxLeaseTime");
    fprintf(fp, "max-lease-time %d;\n", time_value);

    cdb_get_enum_value(rsock, &tmp, "/dhcp/logFacility");
    switch (tmp) {
    case dhcpd_kern:
        fprintf(fp, "log-facility kern;\n");
        break;
    case dhcpd_daemon:
        fprintf(fp, "log-facility daemon;\n");
        break;
    case dhcpd_local7:
        fprintf(fp, "log-facility local7;\n");
        break;
    }

    // YANG leaf-list
    //￼struct confd_list {
    // unsigned int size;
    // struct confd_value *ptr;
    // }
    if (cdb_exists(rsock, "/dhcp/dns")) {
        confd_value_t *values;
        cdb_get_list(rsock, &values, &n, "/dhcp/dns");
        if (n > 0) {
            // fprintf(fp, "leaf-list num: %d\n", n);
            fprintf(fp, "option domain-name-servers ");
            fprintf(fp, "%s", inet_ntoa(CONFD_GET_IPV4(&values[0])));
            confd_free_value(&values[0]);
            if (n == 2) {
                fprintf(fp, ", %s;\n", inet_ntoa(CONFD_GET_IPV4(&values[1])));
                confd_free_value(&values[1]);
            } else
                fprintf(fp, ";\n");
            free(values);
            fprintf(fp, "\n");
        }
    }

    n = cdb_num_instances(rsock, "/dhcp/subNetworks/subNetwork");
    for (i=0; i<n; i++) {
        cdb_cd(rsock, "/dhcp/subNetworks/subNetwork[%d]", i);
        do_subnet(rsock, fp);
    }
    fclose(fp);
    return cdb_close(rsock);
}

/********************************************************************/
/********************************************************************/
int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    int subsock;
    int status;
    int spoint;

    // char *mv_args[] = { "sudo", "mv", "dhcpd.conf", "/etc/dhcp/dhcpd.conf", NULL };
    // char *service_args[] = { "sudo", "service", "isc-dhcp-server", "restart", NULL };
    // runs in mininet as sudo user:
    // char *mv_args[] = { "mv", "dhcpd.conf", "/etc/dhcp/dhcpd.conf", NULL };
    // char *dhcp_args[] = { "isc-dhcp-server", "restart", NULL };

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFD_PORT);

    confd_init(argv[0], stderr, CONFD_TRACE);

    /*
     * Setup subscriptions
     */
    if ((subsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open socket\n");

    if (cdb_connect(subsock, CDB_SUBSCRIPTION_SOCKET, (struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to cdb_connect() to confd \n");

    if ((status = cdb_subscribe(subsock, 3, dhcpd__ns, &spoint, "/dhcp"))
        != CONFD_OK) {
        fprintf(stderr, "Terminate: subscribe %d\n", status);
        exit(0);
    }
    if (cdb_subscribe_done(subsock) != CONFD_OK)
        confd_fatal("cdb_subscribe_done() failed");
    printf("Subscription point = %d\n", spoint);

    /*
     * Read initial config
     */
    if ((status = read_conf(&addr)) != CONFD_OK) {
        fprintf(stderr, "Terminate: read_conf %d\n", status);
        exit(0);
    }
    rename("dhcpd.conf.tmp", "dhcpd.conf");
    /* This is the place to HUP the daemon */

    while (1) {
        static int poll_fail_counter=0;
        struct pollfd set[1];

        set[0].fd = subsock;
        set[0].events = POLLIN;
        set[0].revents = 0;

        if (poll(&set[0], 1, -1) < 0) {
            perror("Poll failed:");
            if(++poll_fail_counter < 10)
                continue;
            fprintf(stderr, "Too many poll failures, terminating\n");
            exit(1);
        }

        poll_fail_counter = 0;
        if (set[0].revents & POLLIN) {
            int sub_points[1];
            int reslen;

            if ((status = cdb_read_subscription_socket(subsock,
                                                       &sub_points[0],
                                                       &reslen)) != CONFD_OK) {
                fprintf(stderr, "terminate sub_read: %d\n", status);
                exit(1);
            }
            if (reslen > 0) {
                if ((status = read_conf(&addr)) != CONFD_OK) {
                    fprintf(stderr, "Terminate: read_conf %d\n", status);
                    exit(1);
                }
            }

            fprintf(stderr, "\n*** Reading new config from CDB --> updating dhcpd config \n");

            /* rename file */
            rename("dhcpd.conf.tmp", "dhcpd.conf");

            // /* move the file to /etc/dhcp/  */
            // pid_t pid = fork();
            // if (pid==0) {
            //     // child
            //     if (execv("/bin/mv", mv_args) == -1) {
            //         fprintf(stderr, "failed to move the dhcpd.conf file!\n");
            //         exit(EXIT_FAILURE);  // make sure to kill the child
            //     }
            //     /* (exec only returns if an error has occurred) */
            // }
            // else if (pid==-1) {
            //     fprintf(stderr, "failed to fork!\n");
            // }
            // /* Put the parent to sleep for 1 second -- to be sure the move is finished */
            // sleep(1);
            //
            // /* HUP the DHCP daemon */
            // pid_t pid2 = fork();
            // if (pid2==0) {
            //     // child
            //     if (execv("/etc/init.d/isc-dhcp-server", dhcp_args) == -1) {
            //         fprintf(stderr, "failed to restart isc-dhcp-server service!\n");
            //         exit(EXIT_FAILURE);  // make sure to kill the child
            //     }
            // }
            // else if (pid==-1) {
            //     fprintf(stderr, "failed to fork!\n");
            // }

            if ((status = cdb_sync_subscription_socket(
                            subsock, CDB_DONE_PRIORITY)) != CONFD_OK) {
                fprintf(stderr, "failed to sync subscription: %d\n", status);
                exit(1);
            }
        }
    }
}

/********************************************************************/
