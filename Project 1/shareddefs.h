struct item {
	int id;
	char astr[64];
	int arguments[3];
};

struct responseItem{
	int value;
};

struct childToParentItem{
	int childValue;
	int isSent;
};

struct parentToChildItem{
	int order;
};

#define MQNAME "/justaname"
