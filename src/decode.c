struct jpeg {

};

struct decode_config{

};

int decode2yuv(struct jpeg*, char* y, char* u, char* v);
int split();
int dct();
int run_length();
int quantilize();
int huffman();
