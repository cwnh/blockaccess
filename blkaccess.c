
/* DGUX  -  gcc -D_MULTIPLE_TARGETS=1 -I/usr/src/uts/aviion -o blkaccess blkaccess.c  */


#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


#if DGUX
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include "/usr/src/uts/aviion/ii/i_dev.h"
#endif

#ifndef linux
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>
#endif

/*#define SCSI 1*/  /* For sgi scsi access */

#if sun
#include <sys/scsi/scsi.h>
#include <sys/dklabel.h>
#elif (defined(sgi) && defined(SCSI)) 
#include <dslib.h>
#elif _AIX
#include <sys/scsi.h>
#endif

#include <errno.h>
#include <dirent.h>

#if __osf__
#define LONGLONG long
#else
#define LONGLONG long long
#endif

#if linux
long long lseek64(int, long long, int);
#endif

#undef MYTRUE
#define MYTRUE 1
#undef MYFALSE
#define MYFALSE !MYTRUE

#define READ 0
#define WRITE 1

#define BUFSIZE (64*1024)

#define ROUNDUP(x,y) ((long)y + ((long)x & ~((long)y-1L)))

#if DGUX
int dgscsi_release(int fd);
#endif


int debugFlag = 0;
int scsiFlag = 0;
int g_BlockSize = 512 ;

struct devList {
  int index;
  char dev[256];
};

#ifdef DGUX
char * strtok_r(char *s1, const char *s2, char **last)
{
  return((char *)strtok(s1,s2));
}
#endif

int isValid(char *buf) 
{
  int i;
  for (i=0;i<strlen(buf);i++) {

    if (isdigit(buf[i]) == 0) {
      perror("bad entry\n");
      return -1;
    }
  }
  return 0;
}

long getNumberInput(char *str) 
{
  char input[32];
  long number = -1;

  printf("%s",str);
  gets(input);
  if (input != NULL) {
    if (input[0] == '0' && (input[1] == 'x' || input[1] == 'X')) {
      char temp[32];
      strcpy(temp,&input[2]);
      if (isValid(temp) == 0) {
	number = strtol(input,(char **)NULL,16);
      }
    }
    else {
      if (isValid(input) == 0) {
	number = strtol(input,(char **)NULL,10);
      }
    }
  }
  return number;
}


void displayOwner()
{
  int i = 0, count =0, fd, res, numFound = 0, numErrors = 0, numOwned = 0;
  DIR *dirp;
  char in[64], dirStr[64], tempstr[256], *buf, *orig, str[128];
  struct dirent *dp;  
  struct devList list[512];
  buf = orig = (char *)malloc(g_BlockSize * 2);
  buf = (char *)ROUNDUP(buf,(long)g_BlockSize);

  in[0] = '\0';
  
#if  sun
  count = 0;
#else
  count = 1;
#endif
  for(;count < 3; count++) {
    if (scsiFlag) {
#if (defined(sgi) && defined(SCSI)) 
      sprintf(dirStr,"/hw/scsi");
#else
      printf("Not supported for this OS!!!\n");
      exit(0);
#endif
    }
    else {
      if (count == 1)
#if defined sun
	sprintf(dirStr,"/dev/md/rdsk");
      else if (count == 0) 
	sprintf(dirStr,"/dev/rdsk");
#elif defined sgi  
        sprintf(dirStr,"/dev/rdsk");	
#elif defined DGUX  
        sprintf(dirStr,"/dev/rpdsk");	
#elif defined hpux
	sprintf(dirStr,"/dev/rdsk");
#elif __osf__ 
# ifdef V4
        sprintf(dirStr,"/dev");
# else
        sprintf(dirStr,"/dev/rdisk");
# endif
#else
        sprintf(dirStr,"/dev");
#endif
      else 
	sprintf(dirStr,"/dev/SANergyCDev");
    }
    dirp = opendir(dirStr);
    if (dirp == NULL) {
      continue;
    }
    while ((dp = readdir(dirp)) != NULL) {
#if _AIX
      if (dp->d_name[0] != '.' && strstr(dp->d_name,"rhdisk")) 
#elif sgi
      if (strstr(dp->d_name,"vol") && !strstr(dp->d_name,"volume")) 
#elif linux
      if (!strstr(dp->d_name,"isd") && (strstr(dp->d_name,"raw") || strstr(dp->d_name,"sd") || strstr(dp->d_name,"hd"))) 
#elif __osf__ 
# ifdef V4
      if ((dp->d_name[0] == 'r') && (dp->d_name[0] == 'r') && (dp->d_name[0] == 'z')) 
# else
      if (strstr(dp->d_name,"dsk"))
# endif
#elif sun	
      if (strstr(dp->d_name,"s2") || strstr(dp->d_name,"p0"))
#else
      if (dp->d_name[0] != '.') 
#endif
      {
	sprintf(tempstr,"%s/%s",dirStr, dp->d_name);

	if (accessDevice(READ,tempstr, buf, 1, 1, MYFALSE)!=-1) {
	  if (strstr(buf,"MNGD"))  {
	    strncpy(str,buf,128);
	    printf("%s %s\n",dp->d_name,str);	  
	    numOwned++;
	  }
	  else {
	    printf("%s NO INFO  \n",dp->d_name);
	  }
	  numFound++;
	}
	else {
	  /*printf("%s Read Failed %s\n",dp->d_name,strerror(errno));*/
	  numErrors++;
	}
      }
    }
    closedir(dirp);
  }
  free(orig);
  printf("Num Devices found Total: %d Readable: %d  NonReadable: %d  Owned: %d\n\n",
	 numFound+numErrors,numFound,numErrors,numOwned);
}  


void tryDevices()
{
  int i = 0, count =0, fd, res;
  DIR *dirp;
  char in[64], dirStr[64], tempstr[256], *buf, *orig;
  struct dirent *dp;  
  struct devList list[512];
  buf = orig = (char *)malloc(g_BlockSize * 2);
  buf = (char *)ROUNDUP(buf,(long)g_BlockSize);

  in[0] = '\0';
  
#if  sun
  count = 0;
#else
  count = 1;
#endif
  for(;count < 2; count++) {
    if (scsiFlag) {
#if (defined(sgi) && defined(SCSI)) 
      sprintf(dirStr,"/hw/scsi");
#else
      printf("Not supported for this OS!!!\n");
      exit(0);
#endif
    }
    else {
#if defined sun
      if (count == 0)
	sprintf(dirStr,"/dev/md/rdsk");
      else
	sprintf(dirStr,"/dev/rdsk");
#elif defined sgi  
      sprintf(dirStr,"/dev/rdsk");	
#elif defined DGUX  
      sprintf(dirStr,"/dev/rpdsk");
#elif defined hpux
      sprintf(dirStr,"/dev/rdsk");
#elif __osf__ 
# ifdef V4
      sprintf(dirStr,"/dev");
# else
      sprintf(dirStr,"/dev/rdisk");
# endif
#endif
    }
    dirp = opendir(dirStr);
    if (dirp == NULL) {
      continue;
    }
    while ((dp = readdir(dirp)) != NULL) {
#if _AIX
      if (dp->d_name[0] != '.' && strstr(dp->d_name,"rhdisk")) 
#elif sgi
      if (strstr(dp->d_name,"vol") && !strstr(dp->d_name,"volume")) 
#elif linux
      if (!strstr(dp->d_name,"isd") && (strstr(dp->d_name,"raw") || strstr(dp->d_name,"sd") || strstr(dp->d_name,"hd"))) 
#elif __osf__ 
# ifdef V4
      if ((dp->d_name[0] == 'r') && (dp->d_name[0] == 'r') && (dp->d_name[0] == 'z')) 
# else
      if (strstr(dp->d_name,"dsk"))
# endif
#else
      if (dp->d_name[0] != '.') 
#endif
      {
	sprintf(tempstr,"%s/%s",dirStr, dp->d_name);
#if _AIX
	if (strstr(dp->d_name,"vpath"))
	  fd = open(tempstr, O_RDWR | O_NDELAY, 777);
	else
	  fd = openx(tempstr, O_RDWR | O_NDELAY, 777, SC_NO_RESERVE);
#else
	fd = open(tempstr, O_RDWR|O_NDELAY);
#endif

#if DGUX
	dgscsi_release(fd);
#endif
	
	if (fd) {
#if sun
	  res = pread64(fd, buf, g_BlockSize, 0); 
#else
	  res = pread(fd, buf, g_BlockSize, 0); 
#endif
	  close(fd);
	  if (res != -1) {
	    list[i].index = i;
	    strcpy(list[i].dev,tempstr);
	    printf("%3d  %s\t",list[i].index,dp->d_name);
	    if (i!=0 && (i+1)%4 == 0) printf("\n");
	    i++;
	  }
	}
      }
    }
    closedir(dirp);
  }
  free(orig);
  printf("\n");
}  

#define LIST_LENGTH 512

int chooseDev(char *dev) 
{
  int res = -1, i = 0, count =0;
  DIR *dirp;
  char in[64], dirStr[64], tempstr[LIST_LENGTH];
  struct dirent *dp;  
  struct devList list[512];
  in[0] = '\0';
 
  do {
#if  sun
    count = 0;
#else
    count = 1;
#endif
    for(;count < 2; count++){
      if (scsiFlag) {
#if (defined(sgi) && defined(SCSI)) 
	sprintf(dirStr,"/hw/scsi");
#else
	printf("Not supported for this OS!!!\n");
	exit(0);
#endif
      }
      else {
#if defined sun
	if (count == 0)
	  sprintf(dirStr,"/dev/md/rdsk");
	else
	  sprintf(dirStr,"/dev/rdsk");
#elif defined sgi  
	sprintf(dirStr,"/dev/rdsk");	
#elif defined DGUX  
	sprintf(dirStr,"/dev/rpdsk");	
#elif __osf__ 
# ifdef V4
        sprintf(dirStr,"/dev");
# else
        sprintf(dirStr,"/dev/rdisk");
# endif
#else
	sprintf(dirStr,"/dev");
#endif
      }
      
      dirp = opendir(dirStr);
      if (dirp == NULL) {
	continue;
      }
      while ((dp = readdir(dirp)) != NULL) {
#if _AIX
	if (dp->d_name[0] != '.' && strstr(dp->d_name,"rhdisk")) 
#elif sgi
	if (strstr(dp->d_name,"vol") && !strstr(dp->d_name,"volume")) 
#elif linux
	if (!strstr(dp->d_name,"isd")  && ((strstr(dp->d_name,"raw") || strstr(dp->d_name,"sd") || strstr(dp->d_name,"hd"))))
#elif __osf__ 
# ifdef V4
	if ((dp->d_name[0] == 'r') && (dp->d_name[0] == 'r') && (dp->d_name[0] == 'z')) 
# else
	if (strstr(dp->d_name,"dsk"))
# endif
#else
        if (dp->d_name[0] != '.') 
#endif
	{
	  list[i].index = i;
	  sprintf(tempstr,"%s/%s",dirStr, dp->d_name);
	  strcpy(list[i].dev,tempstr);
	  printf("%3d  %s\t",list[i].index,dp->d_name);
	  if (i!=0 && (i+1)%4 == 0) printf("\n");
	  i++;
	}
	if (i>LIST_LENGTH) break;
      }
      closedir(dirp);
    }  
    printf("\nChoose a device (0 -> %d):",i-1);
    gets(in);
    
    if (in[0] == 'q') exit(0);
    if (isValid(in) == 0) {
      if (atoi(in) < i) {
	res = 0;
	strcpy(dev,list[atoi(in)].dev);
	printf("Device : %s\n\n",dev);
      }
      else {
	printf("%s is out of range\n",in);
      }
    }
    else {
      printf("%s",strerror(errno));
      exit(1);
    }
  }
  while(res != 0);
  return res;
  
}

#if sun
int accessDevice(int mode, char *dev, char *buf, long block, long nblocks, int print)
{
  struct uscsi_cmd ucmd;
  union scsi_cdb cdb;
  int status, fd, res = -1;
  int sccCmd = SCMD_READ;
  int uscsiFlags = USCSI_READ;

  fd = open64(dev, O_RDWR|O_NDELAY);
  if (fd) {
    if (mode == READ) {
      res = pread64(fd, buf, nblocks*g_BlockSize, (off64_t)block*g_BlockSize); 
      if (res == -1 && print) fprintf(stderr,"read %s fd:%d block:%d failed: %s\n",dev,fd,block,strerror(errno));
    }
    else {
      res = pwrite64(fd, buf, nblocks*g_BlockSize, (off64_t)block*g_BlockSize); 
      if (res == -1 && print) fprintf(stderr,"write %s fd:%d block:%d failed: %s\n",dev,fd,block,strerror(errno));
    }
  }

  if (res == -1) {  /* try uscsi maybe nt */
    if (mode == READ) {
      sccCmd = SCMD_READ;
      uscsiFlags = USCSI_READ;
    }
    else {
      sccCmd = SCMD_WRITE;
      uscsiFlags = USCSI_WRITE;
    }

    memset((char *)&ucmd,0,sizeof(ucmd));
    memset((char *)&cdb,0,sizeof(union scsi_cdb));
    
    cdb.scc_cmd = sccCmd;
    FORMG0ADDR(&cdb, block);
    FORMG0COUNT(&cdb, nblocks);
    
    ucmd.uscsi_cdb = (caddr_t) &cdb;
    ucmd.uscsi_cdblen = CDB_GROUP0;
    ucmd.uscsi_flags |= uscsiFlags;
    
    ucmd.uscsi_bufaddr = (caddr_t) buf;
    ucmd.uscsi_buflen = nblocks*g_BlockSize;
    
    status = ioctl(fd, USCSICMD, &ucmd);
    
    if (status < 0 && print) printf("ioctl failed on %s block:%d : %s\n",dev,block,strerror(errno));
    
    if (status == 0 && ucmd.uscsi_status == 0) {
      if (debugFlag)
	printf("uscisCommand [%s] \n",buf);
      res = 0;
    }
    else {
      if (print) perror("USCSI ioctl error");
      res = -1;
    }  
  }
  close(fd);
  return(res);
}

#else

int accessDevice(int mode, char *dev, char *buf, long block, long nblocks, int print)
{
  int status, fd, res = -1;

  if (!scsiFlag) {
#if _AIX
      fd = openx(dev, O_RDWR, 0777, SC_NO_RESERVE);
#elif sun
      fd = open64(dev, O_RDWR|O_NDELAY);
#elif liunx
# ifndef O_DIRECT
#  ifdef __ppc64__
#   define O_DIRECT       0400000        /* direct disk access hint */
#  else
#   define O_DIRECT       00040000        /* direct disk access hint */
#  endif
# endif
      fd = open(dev, O_RDWR|O_DIRECT);
#else
      fd = open(dev, O_RDWR|O_NDELAY);
#endif
      if (fd) {

#if DGUX
	  dgscsi_release(fd);
#endif

	if (mode == READ) {
#if (defined sun) 
	  res = pread64(fd, buf, nblocks*g_BlockSize, (off64_t)block*g_BlockSize); 
#elif (defined linux) || (defined _AIX)
	  long long go = (long long)((long long)block*(long long)g_BlockSize);
          long long seekres = 0;
	  seekres = lseek64(fd, go, SEEK_SET);
	  /*printf("fd:%d go:%lld SEEK_SET:%d seekres:%lld\n",fd,go,SEEK_SET,seekres);*/
	  if (seekres == -1 && print) printf("lseek64 failed [%s] at block %d : %s\n",dev, block, strerror(errno));	  
	  res = read(fd, buf, nblocks*g_BlockSize); 
	  /*printf("readres %d\n",res);*/
#else
	  /*printf("pread block %d * g_BlockSize %d = %lld\n",block,g_BlockSize,(off64_t)block*g_BlockSize);*/
	  res = pread(fd, buf, nblocks*g_BlockSize, (off_t)block*g_BlockSize); 
#endif
	  if (res == -1 && print) printf("read failed [%s] at block %d : %s\n",dev, block, strerror(errno));	  
	}
	else if (mode == WRITE) {
#if (defined sun) || (defined _AIX)
	  res = pwrite64(fd, buf, nblocks*g_BlockSize, (off64_t)block*g_BlockSize);
#elif linux
	  long long go = (long long)((long long)block*(long long)g_BlockSize);
          long long seekres = 0;
	  seekres = lseek64(fd, go, SEEK_SET);
	  if (seekres ==-1 && print) perror("lseek64 failed");
	  res = write(fd, buf, nblocks*g_BlockSize); 
#else
	  res = pwrite(fd, buf, nblocks*g_BlockSize, (off_t)block*g_BlockSize); 
#endif
	  if (res == -1 && print) perror("read failed");	  
	}
	close(fd);
      }
  }
  else {
#if (defined(sgi) && defined(SCSI)) /* scsi stuff */
      struct dsreq *dsp = NULL;
      char vu = 0;

      memset(buf,0,g_BlockSize);
      dsp = dsopen(dev, O_RDWR);
      if (dsp) {
#if 0
	  fillg0cmd(dsp, (uchar_t *)CMDBUF(dsp), 0x08, 0, 0, 0, B1(512), B1(vu<<6));
	  filldsreq(dsp, (uchar_t *)buf, 512, DSRQ_READ|DSRQ_SENSE);
	  dsp->ds_time = 1000 * 30; /* 90 seconds */
	  res = doscsireq(getfd(dsp), dsp);
#endif
	  res = readextended28(dsp, (caddr_t)buf, nblocks*g_BlockSize, block, 0);   /* past 2 gig */
	  /*res = read08(dsp, (caddr_t)buf, nblocks*g_BlockSize, block, 0); */
	  if (res == -1) perror("read failed");
	  dsclose(dsp);
      }
      else {
	  perror("dsopen failed");
      }
#endif
  }
  return res;
}	 

#endif

void printBlock(char *buf) 
{
  int i;
  char str[32];
  memset(str,0,32);

  printf("000  ");

  for (i=0; i<g_BlockSize; i++){
    if ((unsigned char)buf[i] <= 0xf) 
      printf(" 0%X",(unsigned char)buf[i]);
    else 
      printf(" %X",(unsigned char)buf[i]);

    if ((int)buf[i] >= 32 && (int)buf[i] <= 126)
      sprintf(&str[strlen(str)],"%c",buf[i]);
    else
      sprintf(&str[strlen(str)],".");

    if ((i != 0 && ((i+1)%16) == 0)) {
      printf("  %s\n",str);
      if ((i != g_BlockSize-1) && i < 0xff) 
	printf("0%X  ",i+1);
      else if (i != g_BlockSize-1) /*(i != 0x1ff) */
	printf("%X  ",i+1);
      memset(str,0,32);
      str[0] = '\0';
    }
  }
}


int readBlock(char *dev, long block, char *buf, int display)
{
  if (block >= 0) {
    if (display) printf("Reading %s Block %d  0X%X\n",dev,block,block);
    if (accessDevice(READ,dev, buf, block, 1, MYTRUE)!=-1) {
      if (display == MYTRUE) printBlock(buf);
    }
    else {
      if (display == MYTRUE) perror("Read failed\n");
      return -1;
    }
  }
  else {
    perror("Invalid block number");
  }
  return 0;
}    

int writeBlock(char *dev, long block, char *buf, int display)
{
  if (block >= 0) {
    if (display) printf("Writing %s Block %d  0X%X\n",dev,block,block);
    if (accessDevice(WRITE,dev, buf, block, 1, MYTRUE)==-1) {
      perror("Write failed\n");
      return -1;
    }
  }
  else {
    perror("Invalid block number");
  }
  return 0;
}    

int writeBlockTest(char *dev, char *buf, int display)
{
  long block = 1;
  char in[BUFSIZE];
  strcpy(buf,"Hello World");
  printf("Write to Block(1):\n");
  gets(in);
  if (in[0] != '\0') block = atol(in);
  printf("Enter data(Hello World):\n");
  gets(in);
  if (in[0] != '\0') strcpy(buf,in);
     
  if (block >= 0) {
    if (display) printf("Reading %s Block %d  0X%X\n",dev,block,block);
    if (accessDevice(WRITE,dev, buf, block, 1, MYTRUE)!=-1) {
      memset(buf,0,g_BlockSize);
      if (accessDevice(READ,dev, buf, block, 1, MYTRUE)!=-1) {
	if (display) printBlock(buf);
      }
    }
    else {
      perror("Read failed\n");
      return -1;
    }
  }
  else {
    perror("Invalid block number");
  }
  return 0;
}

void copyBlock(char *dev, char *buf) 
{
  long block = 0;
  if ((block = getNumberInput("Enter src Block #: ")) != -1) {
    if (readBlock(dev, block, buf, 0) == 0) {
      if ((block = getNumberInput("Enter des Block #: ")) != -1) {
	writeBlock(dev, block, buf, 0);
      }
    }
  }
}

void modifyBlock(char *dev, char *buf) 
{
  long block, offset = 0;
  char data[BUFSIZE];

  if ((block = getNumberInput("Enter Block #: ")) != -1) {
    if (readBlock(dev, block, buf, 0) == 0) {
      if ((offset = getNumberInput("Enter offset: ")) != -1) {
	printf("Enter data: ");
	gets(data);
	memcpy(&buf[offset],data,strlen(data-1));
	writeBlock(dev, block, buf, 0);
      }
    }
  }
}

void modifyBlockBytes(char *dev, char *buf) 
{
  long block, offset = 0;
  char data[BUFSIZE];
  long bytes = 0;

  if ((block = getNumberInput("Enter Block #: ")) != -1) {
    if (readBlock(dev, block, buf, 0) == 0) {
      if ((offset = getNumberInput("Enter offset: ")) != -1) {
	printf("Enter data: ");
	gets(data);
	bytes = atol(data);
	memcpy(&buf[offset],&bytes, sizeof(bytes));
	writeBlock(dev, block, buf, 0);
      }
    }
  }
}

int holdOpenDevice(char *dev)
{
  long delay = 3;
  char in[16];
  int fd = 0;
  printf("Enter hold delay(3):\n");
  gets(in);
  if (in[0] != '\0') delay = atol(in);
#if sun
  fd = open64(dev, O_RDWR|O_NDELAY);
#elif _AIX  
  fd = openx(dev, O_RDWR | O_NDELAY, 777, SC_NO_RESERVE);
#else
  fd = open(dev, O_RDWR|O_NDELAY);
# if DGUX
  if (fd != -1) dgscsi_release(fd);
# endif
#endif
  sleep(delay);
  if (fd >= 0) 
    close(fd);
  return fd;
}
    
#define NUMBLKS 100

void searchDev(char *dev, char *buf) 
{
  int j = 0, done = MYFALSE;
  char *token, *text, *blkbuf, *orig, *temp;
  long start = 0, end = 17767890, i = 0;
  blkbuf = orig = (char *)malloc((g_BlockSize * NUMBLKS) + g_BlockSize);
  blkbuf = (char *)ROUNDUP(blkbuf,(long)g_BlockSize);

  token = (char *)strtok_r(buf,"|",&temp); /* cmd */
  if (token != NULL) {
    text = (char *)strtok_r(NULL,"|",&temp);  /* search string */
    if (text != NULL) {
      token = (char *)strtok_r(NULL,"|",&temp); /* start */
      if (token != NULL) {
	start = atol(token);
	token = (char *)strtok_r(NULL,"|",&temp); /* end */
	if (token != NULL) {
	  end = atol(token);
	}
      }
    }

    if (text != NULL) {
      printf("Searching for [%s] on [%s] from %d to %d\n",text,dev,start,end);
      for (i=start; i<end+1; i+=NUMBLKS) {
	if (accessDevice(READ,dev, blkbuf, i, NUMBLKS, MYTRUE)!=-1) {
	  
	  for (j=0; j<(g_BlockSize * NUMBLKS); j++) {
	    if (blkbuf[j] == text[0] && strstr(&blkbuf[j],text)) {
	      printf("\nFOUND %s at block %d\n",text,i+(j/g_BlockSize));
	      printBlock(&blkbuf[(j/g_BlockSize)*g_BlockSize]);	  
	      done = MYTRUE;
	      break;
	    }
	  }
	  /*
	    for (j=0; j<NUMBLKS; j++) {
	    if (strstr(&blkbuf[j*g_BlockSize],text)) {
	    printf("\nFOUND %s at block %d\n",text,i+j);
	    printBlock(&blkbuf[j*g_BlockSize]);
	    done = MYTRUE;
	    break;
	    }
	    }
	  */
	  if (done) break;
	}
	else {
	  printf("search ended at %d\n",i);
	  break;
	}
	/*if (i%256) printf(".");*/
      }
    }
    else {
      printf("syntax s|search string|begin block|end block\n");
    }
  }
  else {
    printf("syntax s|search string|begin block|end block\n");
  }
  free(orig);
}



void help() { 
  printf("device\t(d)\tList all devices and choose by number\n");
  printf("search\t(s)\ts|string|start blk|end blk \n");
  printf("copy\t(c)\tcopy one block to another\n");
  printf("modify\t(m)\tmodify data in a block\n");
  printf("bytes\t(b)\tmodify bytes in a block\n");
  printf("write\t(w)\twrite to the dev specified\n");
  printf("open\t(o)\thold open the raw device.\n");
  printf("list\t(l)\ttry all the raw device.\n");
  printf("owner\t(n)\tshow owner information.\n");
  printf("quit\t(q)\tQuit\n\n");
}

static char Usage[] = "Usage: %s [-d -s -b<BlockSize>] <raw device path> [block number]\n";
extern 		int optind;
  extern char *optarg;

main(int argc, char **argv) 
{
  int c = 0;
  char buf[256], dev[256];
  char *blkbuf, *orig;
  int i,res;
  long blockAddr=0;
 
  if (argc > 1 && argv[1] != NULL) {
    while ((c = getopt(argc, argv, "dsb:h")) != -1){
      switch (c) {
      case 'd':
	debugFlag++;
	break;
      case 's':
	scsiFlag++;
	break;
      case 'b':
         g_BlockSize = atoi(optarg);
	break;
      case 'h':
      case '?':
	fprintf(stderr, Usage, argv[0]);
	exit (1);
      }
    }
  }
  printf("Blocksize is %d\n",g_BlockSize);

  buf[0] = '\0';
  if (argc-optind < 2 ) {
#if sgi
    printf("Try %s -s for low level scsi reads on sgi.\n",argv[0]);
#endif
    help();
    /*chooseDev(dev);*/
  } 
  else if (argc-optind == 3) {
    if (argv[optind+1] != NULL) 
      strcpy(buf,argv[optind+1]);
    strcpy(dev,argv[optind]);
  }
  else {
    strcpy(dev,argv[optind]);
  }


  blkbuf = orig = (char *)malloc(g_BlockSize * 2);
  blkbuf = (char *)ROUNDUP(blkbuf,(long)g_BlockSize);

  while (MYTRUE) {
    res = 0;
    if (buf[0]=='\0') {
      printf("Enter Block # or Command: ");
      gets(buf);
    }

    if (!strcasecmp(buf,"EXIT") || !strcasecmp(buf,"QUIT") || 
	buf[0] == 'e' || buf[0] == 'E' || buf[0] == 'q' || buf[0] == 'Q') {
      /*printf("exiting...\n");*/
      break;
    }
    else if (buf[0] == 'S' || buf[0] == 's' || strstr(buf,"SEARCH")) {
      searchDev(dev, buf);
    }
    else if (buf[0] == 'D' || buf[0] == 'd' || !strcasecmp(buf,"DEVICE")) {
      chooseDev(dev);
    }
    else if (buf[0] == 'H' || buf[0] == 'h' || !strcasecmp(buf,"HELP")) {
      help();
    }
    else if (buf[0] == 'W' || buf[0] == 'w' || !strcasecmp(buf,"WRITE")) {
      writeBlockTest(dev,blkbuf,MYTRUE);
    }
    else if (buf[0] == 'O' || buf[0] == 'o' || !strcasecmp(buf,"OPEN")) {
      holdOpenDevice(dev);
    }    
    else if (buf[0] == 'L' || buf[0] == 'l' || !strcasecmp(buf,"LIST")) {
      tryDevices();
    }    
    else if (buf[0] == 'C' || buf[0] == 'c' || !strcasecmp(buf,"COPY")) {
      copyBlock(dev, blkbuf);
    }
    else if (buf[0] == 'B' || buf[0] == 'b' || !strcasecmp(buf,"BYTES")) {
      modifyBlockBytes(dev, blkbuf);
    }
    else if (buf[0] == 'M' || buf[0] == 'm' || !strcasecmp(buf,"MODIFY")) {
      modifyBlock(dev, blkbuf);
    }
    else if (buf[0] == 'N' || buf[0] == 'n' || !strcasecmp(buf,"OWNER")) {
      displayOwner();
    }
    else if (buf[0] == '+' || buf[0] == '-') {
      char temp[32];       
      sprintf(temp,"%s",&buf[1]);
      if (isValid(temp)==0) {
	if (buf[0] == '+') 
	  blockAddr += atol(temp);
	else 
	  blockAddr -= atol(temp);
	readBlock(dev,blockAddr,blkbuf,MYTRUE);
      }
    }
    else if (!strcmp(buf,">") || !strcmp(buf,".")) {
      blockAddr++;
      readBlock(dev,blockAddr,blkbuf,MYTRUE);
    }
    else if (!strcmp(buf,"<") || !strcmp(buf,",")) {
      blockAddr--;
      readBlock(dev,blockAddr,blkbuf,MYTRUE);
    }
    else if (buf[0] != '#' && buf[0] != '\0') {
      if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
	char temp[32];
	strcpy(temp,&buf[2]);
	if (isValid(temp) == 0) {
	  blockAddr = strtol(buf,(char **)NULL,16);
	  readBlock(dev,blockAddr,blkbuf,MYTRUE);
	}
      }
      else {
	if (isValid(buf) == 0) {
	  blockAddr = strtol(buf,(char **)NULL,10);
	  readBlock(dev,blockAddr,blkbuf,MYTRUE);
	}
      }
    }
    buf[0] = '\0';
  }
  free(orig);
  exit(0);
}


#if DGUX


/*=======================================*/

static void g_print_sense_data(dev_scsi_generic_parm_blk_type * scsi_param_blk_ptr )

/*=======================================*/
{
  printf("ESC=%x\n",scsi_param_blk_ptr->status.status);
  printf("SK =%x\n",scsi_param_blk_ptr->status.sense_key);
  printf("ASK=%x ASB= %x %x %x %x %x %x %x \n",
    scsi_param_blk_ptr->status.ad_sense_key,
    scsi_param_blk_ptr->status.pads[0],
    scsi_param_blk_ptr->status.pads[1],
    scsi_param_blk_ptr->status.pads[2],
    scsi_param_blk_ptr->status.pads[3],
    scsi_param_blk_ptr->status.pads[4],
    scsi_param_blk_ptr->status.pads[5],
    scsi_param_blk_ptr->status.pads[6]);

}


int dgscsi_release(int fd) 
{
  int  status;
  char * byte_ptr;
  dev_scsi_generic_parm_blk_type  	param_blk;
  dev_scsi_generic_parm_ptr_type  	param_blk_ptr;

  dev_scsi_inquiry_buffer_type		inquiry_buffer;
  dev_scsi_inquiry_buffer_ptr_type      inq_buffer_ptr;

  dev_scsi_generic_cmd_ptr_type 	cdb_ptr;

  /**** build the cdb ******/

  param_blk_ptr = &param_blk;
  cdb_ptr = &param_blk_ptr->cmd;
  inq_buffer_ptr = &inquiry_buffer;

  byte_ptr = (char *)param_blk_ptr;

  memset (param_blk_ptr, 0, sizeof(dev_scsi_generic_parm_blk_type));

  cdb_ptr->OpCode = DEV_SCSI_CMD_RELEASE;

  /* Note: Some adapters only allow the standard 32 byte inquiry.  Do not
   use an allocation length greater than 32. */

  cdb_ptr->AllocationLength= 0 ; 

  param_blk_ptr->cmd_size = USER_SCSI_CDB_GROUP0_SIZE;
  param_blk_ptr->data_ptr = inq_buffer_ptr;

  /* Make the data_size negative for reading in data */
  param_blk_ptr->data_size = 0 ; 
  param_blk_ptr->status_size = sizeof(dev_scsi_ioctl_status_blk_type);


  /**** do the ioctl *****/
  status = ioctl(fd, GSCSI_GENERIC_SCSI, param_blk_ptr);
#if 0
  if (status < 0) {
    printf("genio :ioctl %s",strerror(errno));
    g_print_sense_data(param_blk_ptr); 
  }
  else {
    if ( param_blk_ptr->status.status == 0 ) {
      /*printf("RELEASE completed with good status.\n");*/
    }
    else {
      printf("Check condition on the RELEASE.\n");
      g_print_sense_data(param_blk_ptr);
    }
  }  
#endif
  return status;
}
#endif
