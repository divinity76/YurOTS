//Precompiled Headers by Huczu

#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/timeb.h>

//libxml
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


//boost
#include <boost/pool/pool.hpp>
#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <boost/thread.hpp>

//linux
#ifdef __LINUX__
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#endif // linux
