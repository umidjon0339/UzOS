#ifndef DEF_RATE_LIMITING_H
#define DEF_RATE_LIMITING_H

// 1 if UzOS has no remote attack surface to flood: it has no network
// stack, so a *distributed* denial of service (many machines flooding a
// service) has nowhere to land. On a real server the defence is rate
// limiting, load balancing and traffic filtering at the network edge.
int def_no_network_surface(void);

#endif
