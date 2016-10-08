#ifndef LOAD_DATA_H_
#define LOAD_DATA_H_

#include <fstream>
#include <iostream>
#include <vector>
#include <set>
#include "mpi.h"

#define MASTER_ID (0)
#define FEA_DIM_FLAG (99)

struct sparse_feature{
    int group;
    long int idx;
    float val;
};

class Load_Data {
public:
    std::ifstream fin_;
    std::vector<std::vector<sparse_feature> > fea_matrix;
    std::vector<sparse_feature> key_val;
    sparse_feature sf;
    std::vector<int> label;
    std::string line;
    int y, nchar;
    int group;
    long int index;
    float value;
    long int loc_fea_dim = 0;
    long int glo_fea_dim = 0;
    int factor = 10;
    int field = 16;
    std::vector<std::set<int> > cross_field;

    Load_Data(const char *file_name){
        fin_.open(file_name, std::ios::in);
        if(!fin_.is_open()){
            std::cout<<"open file error: "<<file_name << std::endl;
            exit(1);
        } 
        std::cout<<file_name<<std::endl;
        for(int i = 0; i < field; i++){
            for(int j = 0; j < field; j += 1){
                if(j == i) continue;
                cross_field[i].insert(j);
            }
        }
    }

    ~Load_Data(){
        fin_.close();
    }

    void load_data_batch(int nproc, int rank){
        MPI_Status status;
        fea_matrix.clear();
        while(!fin_.eof()){
            std::getline(fin_, line);
            if(fin_.eof()) break;
            key_val.clear();
            const char *pline = line.c_str();
            if(sscanf(pline, "%d%n", &y, &nchar) >= 1){
                pline += nchar;
                label.push_back(y);
                while(sscanf(pline, "%d:%ld:%f%n", &group, &index, &value, &nchar) >= 2){
                    pline += nchar;
                    sf.group = group;
                    sf.idx = index;
                    if(index > loc_fea_dim) loc_fea_dim = index;
                    sf.val = value;
                    key_val.push_back(sf);
                    //if(rank == 0)std::cout<<sf.field<<" "<<sf.idx<<" "<<sf.val<<"\t";
                }
                //std::cout<<std::endl;
            }
            fea_matrix.push_back(key_val);
        }
        if(rank != 0) {
            MPI_Send(&loc_fea_dim, 1, MPI_LONG, 0, 90, MPI_COMM_WORLD);
        }
        else if(rank == 0){ 
            for(int i = 1; i < nproc; i++){
                MPI_Recv(&loc_fea_dim, 1, MPI_LONG, i, 90, MPI_COMM_WORLD, &status);
                if(loc_fea_dim >= glo_fea_dim) glo_fea_dim = loc_fea_dim + 1;
            }
        }
        MPI_Bcast(&glo_fea_dim, 1, MPI_LONG, 0, MPI_COMM_WORLD);//must be in all processes code;
    }
private:
};
#endif
