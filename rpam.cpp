#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <random>

#define N 100

#define R //number of experiments

int new_online_size;

int *alt_path;//corresponds to an alternate path of max size 2N

int find_alt_path(int *on_match, int *off_match, int **graph, int side, int node, int path_index);//recursive function that performs BFS to try and find an alternating path, returns path size or -1 if none exists

int is_in_path(int side, int index, int path_size);//helper function

void update_matching(int *on_match, int *off_match, int path_size);//function that updates current matching given the alternating path



int uniform_randInt(int M, int L){
    return floorl(((double)M + ((double)rand() / (RAND_MAX + 1.0)) * (double)(L-M+1)));
}

int main(){
    int **type_graph;//row correspond to an ONLINE node
    int **offline_graph;//instantiated graph after online phase
    int *matching;//matching[i] is node matched to off-node 'i', else -1
    int *on_matching;//on_matching[i] is node matched to on-node i', else -1
    int *instances;//number of nodes of each type generated by the Poisson process
    int *starting_index;//helper array for creating updated "offline" arrays
    int i, j, k, l, c, neighbor, greedy_size, on_node, path_size, opt_off;
    bool flag;
    long curtime;

    c = 5;

    curtime = time(NULL);
    srand((unsigned int) curtime);

    std::default_random_engine generator;
    std::poisson_distribution<int> distribution(1);

    /** allocate space **/
    type_graph = (int**)malloc(N*sizeof(int *));
    for(i=0;i<N;i++)
        type_graph[i] = (int*)malloc(N*sizeof(int));

    matching = (int*)malloc(N*sizeof(int));
    for(i=0;i<N;i++)
        matching[i] = -1;

    instances = (int*)malloc(N*sizeof(int));
    starting_index = (int*)malloc(N*sizeof(int));

    alt_path = (int*)malloc(2*N*sizeof(int));
    /**************************************/

    /** Generate type graph **/
    for(i=0; i<N; i++){
        for(j=0; j<N; j++){
            if(uniform_randInt(1,N) <= c)
                type_graph[i][j] = 1;
            else
                type_graph[i][j] = 0;
        }
    }

    /*************************/
    /** Online phase **/
    greedy_size = 0;
    new_online_size = 0;
    for(i=0;i<N;i++){ //iterate over each type
        instances[i] = distribution(generator);//number of appearances of a type ~Poisson(1)
        starting_index[i] = new_online_size;
        new_online_size += instances[i];
        //printf("%d\n",instances);
        neighbor = 0;
        for(k=0; k<instances[i]; k++){
            for(j=neighbor;j<N;j++){
                if((type_graph[i][j] == 1) && (matching[j] == (-1))){//if there's an available neighbor, match it
                    matching[j] = i;//offline node 'j' is matched to type 'i'
                    greedy_size++;
                    neighbor = j+1;
                    break;
                }
                if(j == (N-1))
                    neighbor = N; //no more available neighbors (avoid checking for future instances)
            }
        }
    }
    /********************************/
    /** create (new) offline graph **/
    offline_graph = (int**)malloc(new_online_size*sizeof(int *));
    /*allocate row space and input data at the same time*/
    j = 0;
    for(i=0;i<N;i++){
        for(k=0;k<instances[i];k++){
            offline_graph[j] = (int*)malloc(N*sizeof(int));
            for(l=0;l<N;l++)
                offline_graph[j][l] = type_graph[i][l];
            j++;
        }
    }

    on_matching = (int*)malloc(new_online_size*sizeof(int));
    for(i=0;i<new_online_size;i++)
        on_matching[i] = -1;

    /*update matching arrays*/
    for(i=0;i<N;i++){//iterate over offline nodes
        on_node = matching[i];
        if(on_node == (-1))
            continue;
        for(j=starting_index[on_node]; j<starting_index[on_node]+instances[on_node]; j++){
            if(on_matching[j] == (-1)){
                on_matching[j] = i;
                matching[i] = j;
                break;
            }
        }
    }
    /**************************/
    /** Max offline matching **/
    /* alternating paths algorithm on greedy solution*/
    opt_off = greedy_size;
    flag = true;
    while(flag){
        flag = false;
        for(i=0;i<N;i++){//try to find an alternating path starting from some offline node
            if(matching[i] != (-1))//must start from an unmatched offline node
                continue;
            alt_path[0] = i;
            path_size = find_alt_path(on_matching,matching,offline_graph,0,i,0);
            if(path_size != (-1)){//if an alt-path was found, update matching
                update_matching(on_matching,matching,path_size);
                opt_off++; //increase OPT matching size by 1
                flag = true;
                break;
            }
        }
    }
    /************************************/

    printf("Greedy matching size: %d\nOPT size is : %d\n",greedy_size,opt_off);
    ////////////////////////////////////////////////////////////////////////////
    return 0;
}


int find_alt_path(int *on_match, int *off_match, int **graph, int side, int node, int path_index){
    int i, j, found_path;

    found_path = -1;
    ////////////////////////////
    if(side == 0){//we are currently at an offline node
        //try to find unmatched node on online side
        for(j=0;j<new_online_size;j++){//iterate over all adjacent edges
            if((graph[j][node] == 1) && (on_match[j] == (-1))){
                //if(is_in_path(1,j,path_index+1) == 0) //redundant to check if online node is already on path (impossible, would've already stopped)
                alt_path[path_index+1] = j;//complete path
                return (path_index+2);//return size of path (# of nodes)
            }
        }

        //else try to follow an edge not in the matching and not currently on the path to get to a MATCHED online node
        for(i=0;i<new_online_size;i++){
            if((graph[i][node] == 1) && (on_match[i] != (-1)) && (on_match[i] != node) && (is_in_path(1,i,path_index+1) == 0)){
                alt_path[path_index+1] = i;
                found_path = find_alt_path(on_match,off_match,graph,1,i,path_index+1);
                if(found_path != (-1))//if a path was found, return its size
                    return found_path;
                else //if no path was found, check remaining adjacent edges
                    continue;
            }
        }
        return (-1);//if we couldn't expand path, return -1

    } else{//we are at an (matched) online node, only one way forward by following matched edge adjacent to it
        alt_path[path_index+1] = on_match[node];
        return find_alt_path(on_match,off_match,graph,0,on_match[node],path_index+1);
    }
}


int is_in_path(int side, int index, int path_size){
    int i;
    if(side == 0){//offline side
        for(i=0;i<path_size; i += 2){
            if(alt_path[i] == index)
                return 1;
        }
    } else{
        for(i=1;i<path_size; i += 2){
            if(alt_path[i] == index)
                return 1;
        }
    }
    return 0;
}

void update_matching(int *on_match, int *off_match, int path_size){
    int i;
    for(i=0;i<path_size; i+=2 ){
        off_match[alt_path[i]] = alt_path[i+1];
        on_match[alt_path[i+1]] = alt_path[i];
    }
}


