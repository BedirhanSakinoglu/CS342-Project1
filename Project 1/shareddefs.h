struct item {
	//These will be deleted xDDDD
	int id;
	char astr[64];

	int argumentArray[3];
};

struct itemResponse {
	float responseVal;
	int rangeItemCount;	
	int rangeResp[3000];
};

#define MQNAME "/justaname"
