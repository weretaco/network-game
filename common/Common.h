#ifndef _COMMON_H
#define _COMMON_H

void set_nonblock(int sock)
{
    int flags;
    flags = fcntl(sock, F_GETFL,0);
    assert(flags != -1);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

#endif
