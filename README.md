# LFU-Cache
based on implementation described in [Ketan Shah 2010](http://dhruvbird.com/lfu.pdf)

We implemented (unoptimized) two version of LFU Replacements described in [STEFAN PODLIPNIG 2003](https://dl-acm-org.services.lib.mtu.edu/doi/pdf/10.1145/954339.954341)

> ## 1. Perfect-LFU   
> "Perfect LFU counts all requests to an object i. Request counts
persist across replacements. On the one
hand, this assures that the request
counts represent all requests from the
past. On the other hand, these statistics have to be kept for all objects seen
in the past (space overhead)."

> ## 2. In-cache-LFU
> "With in-cache LFU, the
counts are defined for cache objects only.
Although this does not represent all requests in the past, it assures a simpler
management (less space overhead)."
