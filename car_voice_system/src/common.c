#include "common.h"

/********************* 通用函数列表 *******************/

int Open(const char *pathname, int flag)
{
	int fd;
	while((fd=open(pathname, flag)) == -1 && errno == EINTR);

	if(fd == -1)
	{
		perror("open() failed");
		exit(0);
	}

	return fd;
}

ssize_t Write(int fildes, const void *buf, size_t nbyte)
{
	ssize_t n;
	size_t total = 0;

	char *tmp = (char *)buf;
	while(nbyte > 0)
	{
		while((n=write(fildes, tmp, nbyte)) == -1 &&
				errno == EINTR);

		if(n == -1)
		{
			perror("write() error");
			exit(0);
		}

		nbyte -= n;
		tmp += n;
		total += n;
	}

	return total;
}

ssize_t Read(int fildes, void *buf, size_t nbyte)
{
	ssize_t n;
	while((n=read(fildes, buf, nbyte)) == -1
			&& errno == EINTR);

	if(n == -1)
	{
		perror("read() failed");
		exit(0);
	}

	return n;
}


/********************* 网络函数列表 *******************/

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret = bind(sockfd, addr, addrlen);
	if(ret == -1) { perror("bind() failed"); exit(0); }
	return ret;
}

int Listen(int sockfd, int backlog)
{
	int ret = listen(sockfd, backlog);
	if(ret == -1) { perror("listen() failed"); exit(0); }
	return ret;
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int connfd = accept(sockfd, addr, addrlen);
	if(connfd == -1) { perror("accept() failed"); exit(0); }
	return connfd;
}

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret = connect(sockfd, addr, addrlen);
	if(ret == -1) { perror("connect() failed"); exit(0); }
	return ret;
}

int Socket(int domain, int type, int protocol)
{
	int sockfd = socket(domain, type, protocol);
	if(sockfd == -1) { perror("socket() error"); }
	return sockfd;
}

int Setsockopt(int sockfd, int level, int optname,
	       const void *optval, socklen_t optlen)
{
	int retval = setsockopt(sockfd, level, optname, optval, optlen);
	if(retval == -1) { perror("setsockopt() error"); }
	return retval;
}

int Select(int nfds, fd_set *readfds, fd_set *writefds,
	   fd_set *exceptfds, struct timeval *timeout)
{
	int n = select(nfds, readfds, writefds, exceptfds, timeout);
	if(n == -1) { perror("select() failed"); exit(0); }
	return n;
}

int init_sock(const char *ubuntu_ip)
{
	struct sockaddr_in sin;
	int sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	int on = 1;
	Setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	bzero((char *) &sin, sizeof(sin));
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, ubuntu_ip, &sin.sin_addr);
	sin.sin_port = htons(DEF_PORT);

	Connect(sockfd, (struct sockaddr *)&sin, sizeof(sin));
	printf("connected.\n");

	return sockfd;
}


void send_pcm(int sockfd, char *pcmfile)
{
	int fd = Open(pcmfile, O_RDONLY);

	off_t pcm_size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	char *pcm = calloc(1, pcm_size);
	Read(fd, pcm, pcm_size);

	int m = Write(sockfd, pcm, pcm_size);
	printf("%d bytes has been write into socket!\n", m);

	free(pcm);
	close(fd);
}

xmlChar *wait4id(int sockfd)
{
	char *xml = calloc(1, XMLSIZE);

	int n = Read(sockfd, xml, XMLSIZE);
	printf("%d bytes has been recv from ubuntu.\n", n);

	/* 对端关闭或空响应：直接返回 NULL */
	if (n <= 0)
	{
		free(xml);
		return NULL;
	}

	FILE *fp = fopen(XMLFILE, "w");
	if(fp == NULL)
	{
		perror("fopen() failed");
		free(xml);
		exit(0);
	}

	size_t m = fwrite(xml, 1, n, fp);
	if(m != (size_t)n)
	{
		perror("fwrite() failed");
		fclose(fp);
		free(xml);
		exit(0);
	}

	fflush(fp);
	fclose(fp);
	free(xml);

	return parse_xml(XMLFILE);
}


/********************* 触摸屏函数列表 *******************/

void wait4touch(void)
{
	int ts = open("/dev/input/event0", O_RDONLY);
	if(ts == -1)
	{
		perror("open() /dev/input/event0 failed");
		exit(0);
	}

	struct input_event buf;
	static int count = 0;
	while(1)
	{
		bzero(&buf, sizeof(buf));
		read(ts, &buf, sizeof(buf));

		if(buf.type == EV_ABS &&
		   buf.code == ABS_PRESSURE &&
		   buf.value == 0)
		{
			printf("clicked[%d]\n", ++count);
			break;
		}
	}
	close(ts);
}


void wait_key_input(void)
{
	int a;
	scanf("%d",&a);
}


/********************* XML函数列表 *******************/

/* 注意：本函数不再释放 doc，由调用者 parse_xml() 统一释放 */
xmlChar *__get_cmd_id(xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar *key, *id;
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"cmd")))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			printf("cmd: %s\n", key);
			xmlFree(key);

			id = xmlGetProp(cur, (const xmlChar *)"id");
			printf("id: %s\n", id);
			return id;   /* 由 parse_xml 统一释放 doc */
		}
		cur = cur->next;
	}
	return NULL;
}

xmlChar *parse_xml(char *xmlfile)
{
	xmlDocPtr doc;
	xmlNodePtr cur1, cur2;

	doc = xmlParseFile(xmlfile);
	if (doc == NULL)
	{
		fprintf(stderr,"Document not parsed successfully. \n");
		return NULL;
	}

	cur1 = xmlDocGetRootElement(doc);
	if(cur1 == NULL)
	{
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return NULL;
	}

	if(xmlStrcmp(cur1->name, (const xmlChar *)"nlp"))
	{
		fprintf(stderr,"document of the wrong type, root node != nlp\n");
		xmlFreeDoc(doc);
		return NULL;
	}

	cur1 = cur1->xmlChildrenNode;

	while (cur1 != NULL)
	{
		if ((!xmlStrcmp(cur1->name, (const xmlChar *)"result")))
		{
			cur2 = cur1->xmlChildrenNode;
			while(cur2 != NULL)
			{
				if((!xmlStrcmp(cur2->name, (const xmlChar *)"confidence")))
				{
					xmlChar *key = xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
					int conf = atoi((char *)key);
					xmlFree(key);
					if(conf < 30)
					{
						xmlFreeDoc(doc);
						fprintf(stderr, "sorry, I'm NOT sure what you say.\n");
						return NULL;
					}
				}

				if((!xmlStrcmp(cur2->name, (const xmlChar *)"object")))
				{
					xmlChar *id = __get_cmd_id(doc, cur2);
					xmlFreeDoc(doc);      /* 统一释放，避免 double-free */
					return id;
				}
				cur2 = cur2->next;
			}
		}
		cur1 = cur1->next;
	}

	xmlFreeDoc(doc);
	return NULL;
}


/********************* 显示函数列表 *******************/

char *init_lcd(void)
{
	int lcd = Open("/dev/fb0", O_RDWR);

	struct fb_var_screeninfo vinfo;
	ioctl(lcd, FBIOGET_VSCREENINFO, &vinfo);

	unsigned long bpp = vinfo.bits_per_pixel;
	char *FB = mmap(NULL, vinfo.xres * vinfo.yres * bpp/8,
			PROT_READ|PROT_WRITE, MAP_SHARED, lcd, 0);
	if(FB == MAP_FAILED)
	{
		perror("mmap() failed");
		exit(0);
	}
	close(lcd);
	return FB;
}

void show_someone(char *FB, int id)
{
	static int firsttime = 1;
	int namelen = 16;
	static char *oldfile = NULL;
	static char *newfile = NULL;

	if(firsttime == 1)
	{
		oldfile = calloc(1, namelen);
		newfile = calloc(1, namelen);

		snprintf(oldfile, namelen, "./jpeg/%d.jpg", id);
		snprintf(newfile, namelen, "./jpeg/%d.jpg", id);

		printf("[%d]: %s\n", __LINE__, newfile);
		show_in(FB, newfile);

		firsttime = 0;
		return;
	}

	snprintf(newfile, namelen, "./jpeg/%d.jpg", id);
	fade_out(FB, oldfile);
	show_in (FB, newfile);
}

void show_in(char *FB, char *filename)
{
	char *rgb_buf = get_rgb(filename);
	blind_window_in(FB, (stride)rgb_buf);
	free(rgb_buf);
}

void fade_out(char *FB, char *filename)
{
	char *rgb_buf = get_rgb(filename);
	blind_window_out(FB, (stride)rgb_buf);
	free(rgb_buf);
}


unsigned long read_image_from_file(int fd,
				   unsigned char *jpg_buffer,
				   unsigned long jpg_size)
{
	unsigned long nread = 0;
	unsigned long total = 0;

	while(jpg_size > 0)
	{
		nread = read(fd, jpg_buffer, jpg_size);
		if(nread == -1)
		{
			perror("read jpeg-file failed");
			exit(1);
		}
		if(nread == 0) break;

		jpg_size -= nread;
		jpg_buffer += nread;
		total += nread;
	}
	close(fd);

	return total;
}

char *transfor(unsigned char *rgb_buffer)
{
	char *rgb_buf = calloc(1, 800*480*4);
	unsigned char *tmp = rgb_buffer + 1;

	int i, j;
	for(i=0; i<480; i++)
	{
		for(j=0; j<800; j++)
		{
			/* BGR -> BGRA，LCD 帧缓冲为 [B][G][R][A] */
			rgb_buf[(j+800*i)*4+0] = tmp[(j+800*i)*3+0];
			rgb_buf[(j+800*i)*4+1] = tmp[(j+800*i)*3+1];
			rgb_buf[(j+800*i)*4+2] = tmp[(j+800*i)*3+2];
			rgb_buf[(j+800*i)*4+3] = 0;
		}
	}
	free(rgb_buffer);
	return rgb_buf;
}

char *get_rgb(char *filename)
{
	struct stat file_info;
	stat(filename, &file_info);
	int fd = Open(filename, O_RDONLY);

	unsigned char *jpg_buffer;
	jpg_buffer = (unsigned char *)calloc(1, file_info.st_size);
	read_image_from_file(fd, jpg_buffer, file_info.st_size);

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, jpg_buffer, file_info.st_size);

	int ret = jpeg_read_header(&cinfo, 1);
	if(ret != 1)
	{
		fprintf(stderr, "[%d]: jpeg_read_header failed: %s\n",
			__LINE__, strerror(errno));
		exit(1);
	}

	jpeg_start_decompress(&cinfo);
	int row_stride = cinfo.output_width * cinfo.output_components;

	unsigned long rgb_size = row_stride * cinfo.output_height;
	unsigned char *rgb_buffer = (unsigned char *)calloc(1, rgb_size);

	while(cinfo.output_scanline < cinfo.output_height)
	{
		unsigned char *buffer_array[1];
		buffer_array[0] = rgb_buffer +
				(cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(jpg_buffer);

	return transfor(rgb_buffer);
}


void *__routine(void *p)
{
	struct argument *arg = (struct argument *)p;
	int i;

	if(arg->flag == IN)
	{
		for(i=HEIGHT/BLIND-1; i>=0; i--)
		{
			memcpy(&(arg->FB)[arg->offset+i][0],
			       &(arg->image)[arg->offset+i][0],
			       WIDTH*4);
			usleep(10000);
		}
	}

	if(arg->flag == OUT)
	{
		for(i=0; i<HEIGHT/BLIND; i++)
		{
			memset(&(arg->FB)[arg->offset+i][0], 0, WIDTH*4);
			usleep(10000);
		}
	}

	free(arg);
	pthread_exit(NULL);
}

void __write_lcd(char *FB, unsigned long (*image)[WIDTH], int flag)
{
	int i;
	pthread_t tid[BLIND];
	for(i=0; i<BLIND; i++)
	{
		struct argument *arg = malloc(sizeof(struct argument));
		arg->FB = (stride)FB;
		arg->image = image;
		arg->offset = i*(HEIGHT/BLIND);
		arg->flag = flag;

		pthread_create(&tid[i], NULL, __routine, (void *)arg);
	}

	for(i=0; i<BLIND; i++)
	{
		pthread_join(tid[i], NULL);
	}
}

void blind_window_in(char *FB, unsigned long (*image)[WIDTH])
{
	__write_lcd(FB, image, IN);
}

void blind_window_out(char *FB, unsigned long (*image)[WIDTH])
{
	__write_lcd(FB, image, OUT);
}

void blind_window(char *FB, unsigned long (*image)[WIDTH])
{
	blind_window_in(FB, image);
	blind_window_out(FB, image);
}

/*************************串口相关函数*****************************/
void init_tty(int fd)
{
	struct termios termios_new;
	bzero( &termios_new, sizeof(termios_new));
	cfmakeraw(&termios_new);
	cfsetispeed(&termios_new, B9600);
	cfsetospeed(&termios_new, B9600);
	termios_new.c_cflag |= CLOCAL | CREAD;
	termios_new.c_cflag &= ~CSIZE;
	termios_new.c_cflag |= CS8;
	termios_new.c_cflag &= ~PARENB;
	termios_new.c_cflag &= ~CSTOPB;
	tcflush(fd,TCIFLUSH);
	termios_new.c_cc[VTIME] = 10;
	termios_new.c_cc[VMIN] = 1;
	tcflush (fd, TCIFLUSH);
	if(tcsetattr(fd,TCSANOW,&termios_new) )
		printf("Setting the serial1 failed!\n");
}
void uart_send_data(int fd,int id_num)
{
	switch(id_num)
	{
		case 1:Write(fd,"open windows",20); break;
		case 2:Write(fd,"open door",20); break;
		case 3:Write(fd,"open led",20); break;
		case 4:Write(fd,"close windows",20); break;
		case 5:Write(fd,"close door",20); break;
		case 6:Write(fd,"close led",20); break;
		default: break;
	}
}

/**************************UDP通信相关**********************************/

void send_data_to_8266(int id_num)
{
	int sockfd;
	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) ==-1)
	{
		printf("socket error\n");
		exit(1);
	}

	struct sockaddr_in r_addr;
	bzero(&r_addr,sizeof(struct sockaddr_in));
	r_addr.sin_family = AF_INET;
	r_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	r_addr.sin_port = htons(10086);

	int on=1;
	setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on));

	switch(id_num)
	{
		case 1: sendto(sockfd,"open windows",20,0,(struct sockaddr *)&r_addr,sizeof(struct sockaddr_in)); printf("open windows\n"); break;
		case 2: sendto(sockfd,"open door",20,0,(struct sockaddr *)&r_addr,sizeof(struct sockaddr_in));    printf("open door\n");    break;
		case 3: sendto(sockfd,"open led",20,0,(struct sockaddr *)&r_addr,sizeof(struct sockaddr_in));     printf("open led\n");     break;
		case 4: sendto(sockfd,"close windows",20,0,(struct sockaddr *)&r_addr,sizeof(struct sockaddr_in));printf("close windows\n");break;
		case 5: sendto(sockfd,"close door",20,0,(struct sockaddr *)&r_addr,sizeof(struct sockaddr_in));   printf("close door\n");   break;
		case 6: sendto(sockfd,"close led",20,0,(struct sockaddr *)&r_addr,sizeof(struct sockaddr_in));    printf("close led\n");    break;
		default: break;
	}

	close(sockfd);
}