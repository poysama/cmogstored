/* used for Debian GNU/kFreeBSD */
#ifdef SETPROCTITLE_NOOP
void setproctitle(const char *fmt, ...)
{
}

void spt_init(int argc, char *argv[], char *envp[])
{
}
#endif
