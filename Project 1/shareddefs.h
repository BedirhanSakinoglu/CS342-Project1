struct item {
	int id;
	char astr[64];
	int arguments[3];
};

struct responseItem{
	int value[1000];
};

struct childToParentItem{
	int childValue[1000];
	int childOrder;
	int isSent;
};

struct parentToChildItem{
	int order;
	int arguments[3];
};

#define MQNAME "/justaname"
