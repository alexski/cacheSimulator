#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

//./first 1<cache size> 2<associativity> 3<cache policy> 4<block size> 5<trace file>
// Cache size = Sets * Associativity * Block size

struct Block{
    unsigned long int tag;
    int blockOffset;
    unsigned int metadata;
};

int format(int, char**);

int main(int argc, char** argv){
    FILE* fp;
    int setNum, assocNum, h, i, j, lowest, found, pfound, full;
    long hit, miss, phit, pmiss, read, write, pread, pwrite;
    unsigned long int tag, ptag;
    hit = miss = phit = pmiss = read = write = pread = pwrite = 0;
    char assoc, wr;
    unsigned int pmetadata, metadata, min;
    char counter[100];
    unsigned long long int address, paddress;
    int index = 0;
    int pindex = 0;
    metadata = 0;
    pmetadata = 0;
    min = 0;
    lowest = h = i = j = full = 0;

    if(format(argc, argv) != 0 || !(fp = fopen(argv[5], "r"))){
        printf("error\n");
        return 0;
    }
    int cacheSize = atoi(argv[1]);
    int blockSize = atoi(argv[4]);
    struct Block**** cache = (struct Block****) malloc(2 * sizeof(struct Block***));

    //BUILD DIRECT MAPPED CACHE
    if(strcmp(argv[2], "direct") == 0){ // number of sets = number of total blocks
        assoc = 'd';
        assocNum = 1;
        setNum = cacheSize / blockSize;

        for(h = 0; h < 2; h++){
            cache[h] = (struct Block***) malloc(setNum * sizeof(struct Block**));
            for(i = 0; i < setNum; i++){
                cache[h][i] = (struct Block**) malloc(sizeof(struct Block*));
                cache[h][i][0] = (struct Block*) malloc(sizeof(struct Block));
                cache[h][i][0]->tag = -1;
                cache[h][i][0]->blockOffset = -1;
                cache[h][i][0]->metadata = 0;
            }
        }
    }else if(strcmp(argv[2], "assoc") == 0){ // only 1 set so C = B*A BUILD FULLY ASSOCIATIVE CACHE
        assoc = 'f';
        setNum = 1;
        assocNum = cacheSize / blockSize;
        for(h = 0; h < 2; h++){
            cache[h] = (struct Block***) malloc(sizeof(struct Block**));
            cache[h][0] = (struct Block**) malloc(assocNum * sizeof(struct Block*));
            for(i = 0; i < assocNum; i++){
                cache[h][0][i] = (struct Block*) malloc(sizeof(struct Block));
                cache[h][0][i]->tag = -1;
                cache[h][0][i]->blockOffset = -1;
                cache[h][0][i]->metadata = 0;
            }
        }
    }else if(strlen(argv[2]) > 6){ //n = assocNum = A    C = S*A*B BUILD N WAY ASSOCIATIVE CACHE
        assoc = 'n';
        int len = ((int)(strlen(argv[2]) - 6));
        //CHECK HOW MANY SETS THE COMMAND CALLS FOR AND ASSIGN TO setNum
        if(argv[2][5] == ':'){
            char nWays[len];
            for(i = 0; i+6 <= ((int)strlen(argv[2])-1); i++){
                nWays[i] = argv[2][i+6];
            }
            assocNum = atoi(nWays);
        }else{
            for(i = 0; i < 2; i++){
                free(cache[i]);
            }
            free(cache);
            printf("error\n");
            return 0;
        }
        setNum = cacheSize / (blockSize * assocNum);

        for(h = 0; h < 2; h++){
            cache[h] = (struct Block***) malloc(setNum * sizeof(struct Block**));
            for(i = 0; i < setNum; i++){
                cache[h][i] = (struct Block**) malloc(assocNum * sizeof(struct Block*));
                for(j = 0; j < assocNum; j++){
                    cache[h][i][j] = (struct Block*) malloc(sizeof(struct Block));
                    cache[h][i][j]->tag = -1;
                    cache[h][i][j]->blockOffset = -1;
                    cache[h][i][j]->metadata = 0;
                }
            }
        }
    }else{
        printf("error\n");
        return 0;
    }

    fp = fopen(argv[5], "r");

    int b_bit = (int) (log(blockSize) / log(2));
    double s_bit = (double) (log(setNum) / log(2));

    while(fscanf(fp, "%s %c %llx", counter, &wr, &address) != EOF){
        if(strcmp(counter, "#eof") == 0){break;}

        if(wr == 'R'){
            //READ
            if(assoc == 'd'){ //ASSOC IS DIRECT
                index = (address >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                tag = (unsigned long int)(address >> (unsigned)(s_bit + b_bit));
                if(cache[0][index][0]->tag == tag){ //HIT
                    hit++;
                }else{ //MISS
                    read++;
                    miss++;
                    cache[0][index][0]->tag = tag;
                    cache[0][index][0]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                }
                //PREFETCH CACHE DIRECT READ
                if(cache[1][index][0]->tag == tag){ //PREFETCH HIT
                    phit++;
                }else{ //PREFETCH MISS
                    pmiss++;
                    pread++;
                    cache[1][index][0]->tag = tag;
                    cache[1][index][0]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);

                    paddress = address + blockSize;
                    pindex = (paddress >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                    ptag = (unsigned long int)(paddress >> (unsigned)(s_bit + b_bit));
                    if(cache[1][pindex][0]->tag != ptag){
                        pread++;
                        cache[1][pindex][0]->tag = ptag;
                        cache[1][pindex][0]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                    }
                }
            }
            if(assoc == 'f'){ //ASSOC IS FULL
                tag = (unsigned long int)(address >> (unsigned)(s_bit + b_bit));
                found = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[0][0][j]->tag == tag){
                        found = 1;
                        break;
                    }
                }
                if(found == 1){ //HIT
                    hit++;
                    if(strcmp(argv[3], "lru") == 0){
                        metadata++;
                        cache[0][0][j]->metadata = metadata;
                    }
                }else{ //MISS
                    read++;
                    miss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[0][0][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        metadata++;
                        cache[0][0][j]->tag = tag;
                        cache[0][0][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][0][j]->metadata = metadata;
                    }else{
                        min = metadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[0][0][j]->metadata <= min){
                                min = cache[0][0][j]->metadata;
                                lowest = j;
                            }
                        }
                        metadata++;
                        cache[0][0][lowest]->tag = tag;
                        cache[0][0][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][0][lowest]->metadata = metadata;
                    }

                }
                //PREFETCH CACHE FULL READ
                pfound = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[1][0][j]->tag == tag){
                        pfound = 1;
                        break;
                    }
                }
                if(pfound == 1){ //PREFETCHED HIT
                    phit++;
                    if(strcmp(argv[3], "lru") == 0){
                        pmetadata++;
                        cache[1][0][j]->metadata = pmetadata;
                    }
                }else{ //PREFETCHED MISS
                    pread++;
                    pmiss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][0][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        pmetadata++;
                        cache[1][0][j]->tag = tag;
                        cache[1][0][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][0][j]->metadata = pmetadata;
                    }else{
                        min = pmetadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[1][0][j]->metadata <= min){
                                min = cache[1][0][j]->metadata;
                                lowest = j;
                            }
                        }
                        pmetadata++;
                        cache[1][0][lowest]->tag = tag;
                        cache[1][0][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][0][lowest]->metadata = pmetadata;
                    }
                    paddress = address + blockSize;
                    ptag = (unsigned long int)(paddress >> (unsigned)(s_bit + b_bit));
                    full = 1;
                    pfound = 0;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][0][j]->tag == ptag){
                            pfound = 1;
                            break;
                        }
                    }
                    if(pfound == 0){
                        pread++;
                        full = 1;
                        for(j = 0; j < assocNum; j++){
                            if(cache[1][0][j]->tag == -1){
                                full = 0;
                                break;
                            }
                        }
                        if(full == 0){
                            pmetadata++;
                            cache[1][0][j]->tag = ptag;
                            cache[1][0][j]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][0][j]->metadata = pmetadata;
                        }else{
                            min = pmetadata;
                            for(j = 0; j < assocNum; j++){
                                if(cache[1][0][j]->metadata <= min){
                                    min = cache[1][0][j]->metadata;
                                    lowest = j;
                                }
                            }
                            pmetadata++;
                            cache[1][0][lowest]->tag = ptag;
                            cache[1][0][lowest]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][0][lowest]->metadata = pmetadata;
                        }
                    }
                }
            }
            if(assoc == 'n'){ //ASSOC IS N WAYS
                index = (address >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                tag = (unsigned long int)(address >> (unsigned)(s_bit + b_bit));
                found = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[0][index][j]->tag == tag){
                        found = 1;
                        break;
                    }
                }
                if(found == 1){ //HIT
                    hit++;
                    if(strcmp(argv[3], "lru") == 0){
                        metadata++;
                        cache[0][index][j]->metadata = metadata;
                    }
                }else{ //MISS
                    read++;
                    miss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[0][index][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        metadata++;
                        cache[0][index][j]->tag = tag;
                        cache[0][index][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][index][j]->metadata = metadata;
                    }else{
                        min = metadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[0][index][j]->metadata <= min){
                                min = cache[0][index][j]->metadata;
                                lowest = j;
                            }
                        }
                        metadata++;
                        cache[0][index][lowest]->tag = tag;
                        cache[0][index][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][index][lowest]->metadata = metadata;
                    }
                }
                //PREFETCH CACHE N WAY READ
                pfound = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[1][index][j]->tag == tag){
                        pfound = 1;
                        break;
                    }
                }
                if(pfound == 1){ //PREFETCHED HIT
                    phit++;
                    if(strcmp(argv[3], "lru") == 0){
                        pmetadata++;
                        cache[1][index][j]->metadata = pmetadata;
                    }
                }else{ //PREFETCHED MISS
                    pread++;
                    pmiss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][index][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        pmetadata++;
                        cache[1][index][j]->tag = tag;
                        cache[1][index][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][index][j]->metadata = pmetadata;
                    }else{
                        min = pmetadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[1][index][j]->metadata <= min){
                                min = cache[1][index][j]->metadata;
                                lowest = j;
                            }
                        }
                        pmetadata++;
                        cache[1][index][lowest]->tag = tag;
                        cache[1][index][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][index][lowest]->metadata = pmetadata;
                    }

                    paddress = address + blockSize;
                    pindex = (paddress >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                    ptag = (unsigned long int)(paddress >> (unsigned)(s_bit + b_bit));
                    full = 1;
                    pfound = 0;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][pindex][j]->tag == ptag){
                            pfound = 1;
                        }
                        if(cache[1][pindex][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(pfound == 0){
                        pread++;
                        if(full == 0){
                            pmetadata++;
                            cache[1][pindex][j]->tag = ptag;
                            cache[1][pindex][j]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][pindex][j]->metadata = pmetadata;
                        }else{
                            min = pmetadata;
                            for(j = 0; j < assocNum; j++){
                                if(min >= cache[1][pindex][j]->metadata){
                                    min = cache[1][pindex][j]->metadata;
                                    lowest = j;
                                }
                            }
                            pmetadata++;
                            cache[1][pindex][lowest]->tag = ptag;
                            cache[1][pindex][lowest]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][pindex][lowest]->metadata = pmetadata;
                        }
                    }
                }
            }
        }
        if(wr == 'W'){
            //WRITE
            if(assoc == 'd'){ //ASSOC IS DIRECT
                index = (address >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                tag = (unsigned long int)(address >> (unsigned)(s_bit + b_bit));
                if(cache[0][index][0]->tag == tag){ //HIT
                    hit++;
                    write++;
                }else{ //MISS
                    write++;
                    read++;
                    miss++;
                    cache[0][index][0]->tag = tag;
                    cache[0][index][0]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                }
                //PREFETCH CACHE DIRECT WRITE
                if(cache[1][index][0]->tag == tag){ //PREFETCH HIT
                    phit++;
                    pwrite++;
                }else{ //PREFETCH MISS
                    pmiss++;
                    pwrite++;
                    pread++;
                    cache[1][index][0]->tag = tag;
                    cache[1][index][0]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                    paddress = address + blockSize;
                    pindex = (paddress >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                    ptag = (unsigned long int)(paddress >> (unsigned)(s_bit + b_bit));
                    if(cache[1][pindex][0]->tag != ptag){
                        pread++;
                        cache[1][pindex][0]->tag = ptag;
                        cache[1][pindex][0]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                    }
                }
            }
            if(assoc == 'f'){ //ASSOC IS FULL
                tag = (unsigned long int)(address >> (unsigned)(b_bit));
                found = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[0][0][j]->tag == tag){
                        found = 1;
                        break;
                    }
                }
                if(found == 1){ //HIT
                    hit++;
                    write++;
                    if(strcmp(argv[3], "lru") == 0){
                        metadata++;
                        cache[0][0][j]->metadata = metadata;
                    }
                }else{ //MISS
                    read++;
                    write++;
                    miss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[0][0][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        metadata++;
                        cache[0][0][j]->tag = tag;
                        cache[0][0][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][0][j]->metadata = metadata;
                    }else{
                        min = metadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[0][0][j]->metadata <= min){
                                min = cache[0][0][j]->metadata;
                                lowest = j;
                            }
                        }
                        metadata++;
                        cache[0][0][lowest]->tag = tag;
                        cache[0][0][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][0][lowest]->metadata = metadata;
                    }

                }
                //PREFETCH CACHE FULL WRITE
                pfound = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[1][0][j]->tag == tag){
                        pfound = 1;
                        break;
                    }
                }
                if(pfound == 1){ //PREFETCHED HIT
                    phit++;
                    pwrite++;
                    if(strcmp(argv[3], "lru") == 0){
                        pmetadata++;
                        cache[1][0][j]->metadata = pmetadata;
                    }
                }else{ //PREFETCHED MISS
                    pread++;
                    pwrite++;
                    pmiss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][0][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        pmetadata++;
                        cache[1][0][j]->tag = tag;
                        cache[1][0][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][0][j]->metadata = pmetadata;
                    }else{
                        min = pmetadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[1][0][j]->metadata <= min){
                                min = cache[1][0][j]->metadata;
                                lowest = j;
                            }
                        }
                        pmetadata++;
                        cache[1][0][lowest]->tag = tag;
                        cache[1][0][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][0][lowest]->metadata = pmetadata;
                    }
                    paddress = address + blockSize;
                    ptag = (unsigned long int)(paddress >> (unsigned)(s_bit + b_bit));
                    full = 1;
                    pfound = 0;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][0][j]->tag == ptag){
                            pfound = 1;
                        }
                        if(cache[1][0][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(pfound == 0){
                        pread++;
                        if(full == 0){
                            pmetadata++;
                            cache[1][0][j]->tag = ptag;
                            cache[1][0][j]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][0][j]->metadata = pmetadata;
                        }else{
                            min = pmetadata;
                            for(j = 0; j < assocNum; j++){
                                if(min >= cache[1][0][j]->metadata){
                                    min = cache[1][0][j]->metadata;
                                    lowest = j;
                                }
                            }
                            pmetadata++;
                            cache[1][0][lowest]->tag = ptag;
                            cache[1][0][lowest]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][0][lowest]->metadata = pmetadata;
                        }
                    }
                }
            }
            if(assoc == 'n'){ //ASSOC IS N WAYS
                index = (address >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                tag = (unsigned long int)(address >> (unsigned)(s_bit + b_bit));
                found = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[0][index][j]->tag == tag){
                        found = 1;
                        break;
                    }
                }
                if(found == 1){ //HIT
                    hit++;
                    write++;
                    if(strcmp(argv[3], "lru") == 0){
                        metadata++;
                        cache[0][index][j]->metadata = metadata;
                    }
                }else{ //MISS
                    read++;
                    write++;
                    miss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[0][index][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        metadata++;
                        cache[0][index][j]->tag = tag;
                        cache[0][index][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][index][j]->metadata = metadata;
                    }else{
                        min = metadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[0][index][j]->metadata <= min){
                                min = cache[0][index][j]->metadata;
                                lowest = j;
                            }
                        }
                        metadata++;
                        cache[0][index][lowest]->tag = tag;
                        cache[0][index][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[0][index][lowest]->metadata = metadata;
                    }
                }
                //PREFETCH CACHE N WAY WRITE
                pfound = 0;
                for(j = 0; j < assocNum; j++){
                    if(cache[1][index][j]->tag == tag){
                        pfound = 1;
                        break;
                    }
                }
                if(pfound == 1){ //PREFETCHED HIT
                    phit++;
                    pwrite++;
                    if(strcmp(argv[3], "lru") == 0){
                        pmetadata++;
                        cache[1][index][j]->metadata = pmetadata;
                    }
                }else{ //PREFETCHED MISS
                    pread++;
                    pwrite++;
                    pmiss++;
                    full = 1;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][index][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(full == 0){
                        pmetadata++;
                        cache[1][index][j]->tag = tag;
                        cache[1][index][j]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][index][j]->metadata = pmetadata;
                    }else{
                        min = pmetadata;
                        for(j = 0; j < assocNum; j++){
                            if(cache[1][index][j]->metadata <= min){
                                min = cache[1][index][j]->metadata;
                                lowest = j;
                            }
                        }
                        pmetadata++;
                        cache[1][index][lowest]->tag = tag;
                        cache[1][index][lowest]->blockOffset = (address >> 0) & (unsigned)(pow(2, b_bit) - 1);
                        cache[1][index][lowest]->metadata = pmetadata;
                    }
                    paddress = address + blockSize;
                    pindex = (paddress >> b_bit) & (unsigned)(pow(2, s_bit) - 1);
                    ptag = (unsigned long int)(paddress >> (unsigned)(s_bit + b_bit));
                    full = 1;
                    pfound = 0;
                    for(j = 0; j < assocNum; j++){
                        if(cache[1][pindex][j]->tag == ptag){
                            pfound = 1;
                        }
                        if(cache[1][pindex][j]->tag == -1){
                            full = 0;
                            break;
                        }
                    }
                    if(pfound == 0){
                        pread++;
                        if(full == 0){
                            pmetadata++;
                            cache[1][pindex][j]->tag = ptag;
                            cache[1][pindex][j]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][pindex][j]->metadata = pmetadata;
                        }else{
                            min = pmetadata;
                            for(j = 0; j < assocNum; j++){
                                if(min >= cache[1][pindex][j]->metadata){
                                    min = cache[1][pindex][j]->metadata;
                                    lowest = j;
                                }
                            }
                            pmetadata++;
                            cache[1][pindex][lowest]->tag = ptag;
                            cache[1][pindex][lowest]->blockOffset = (paddress >> 0) & (unsigned)(pow(2, b_bit) - 1);
                            cache[1][pindex][lowest]->metadata = pmetadata;
                        }
                    }
                }
            }
        }
    }

    fclose(fp);

    for(h = 0; h < 2; h++){
        for(i = 0; i < setNum; i++){
            for(j = 0; j < assocNum; j++){
                free(cache[h][i][j]);
            }
            free(cache[h][i]);
        }
        free(cache[h]);
    }
    free(cache);

    printf("no-prefetch\n");
    printf("Memory reads: %ld\n", read);
    printf("Memory writes: %ld\n", write);
    printf("Cache hits: %ld\n", hit);
    printf("Cache misses: %ld\n", miss);
    printf("with-prefetch\n");
    printf("Memory reads: %ld\n", pread);
    printf("Memory writes: %ld\n", pwrite);
    printf("Cache hits: %ld\n", phit);
    printf("Cache misses: %ld\n", pmiss);

    return 0;
}

int format(int argc, char** argv){
    int sum = 0;
    char* ptr;
    if(argc != 6){return 1;}
    if((strtol(argv[1], &ptr, 10) % 2) != 0){
        sum = 1;
    }
    if(strcmp(argv[3], "fifo") != 0 && strcmp(argv[3], "lru") != 0){
        sum = 2;
    }
    if((atoi(argv[4]) % 2) != 0 && (atoi(argv[4]) % 2) != 1){
        sum = 3;
    }
    //printf("%d\n", sum);
    return sum;
}
