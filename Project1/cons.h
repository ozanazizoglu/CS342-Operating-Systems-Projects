#define FIRSTMQ "/afirstmq"
#define SECONDMQ "/asecondmq"
#define TFIRSTMQ "/atfirstmq"
#define TSECONDMQ "/atsecondmq"
struct item{
	char request[64];
	int	params[3];
	int  param_count;
};	

struct result{
	int size;
	double nums[1000];
};
