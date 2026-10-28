# distributed_denial_of_service -> rate_limiting  (concept)

**Attack (concept).** Many machines flood one service until its CPU or
network can no longer serve real users. It needs a network and many
attackers - UzOS has neither, so it cannot be a target and nothing is
launched.

**Defence (concept).** On a real server: rate limiting per source, load
balancing, and traffic filtering upstream, so the flood is dropped
before it reaches the CPU. The single-machine version (one process
hogging the CPU) is the `denial_of_service` pair, which is real here.
